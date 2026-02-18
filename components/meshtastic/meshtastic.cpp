#include "meshtastic.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF
#include <esp_random.h>
#endif

namespace esphome {
namespace meshtastic {

static const char *const TAG = "meshtastic";

// Serial framing constants
static const uint8_t START1 = 0x94;
static const uint8_t START2 = 0xC3;

// Protobuf wire types
static const uint8_t WT_VARINT = 0;
static const uint8_t WT_64BIT = 1;
static const uint8_t WT_LENGTH = 2;
static const uint8_t WT_32BIT = 5;

// Meshtastic PortNum values
static const uint32_t PORTNUM_TEXT_MESSAGE = 1;
static const uint32_t PORTNUM_NODEINFO_APP = 4;
static const uint32_t PORTNUM_ROUTING = 5;
static const uint32_t PORTNUM_ADMIN = 6;

// Routing::Error
static const uint32_t ROUTING_ERROR_NONE = 0;

// ---- Component lifecycle ----

void MeshtasticComponent::setup() {
  if (this->power_pin_ != nullptr) {
    this->power_pin_->setup();
    this->power_pin_->digital_write(false);
  }

  if (this->enable_on_boot_) {
    this->power_on();
  } else {
    this->state_ = State::OFF;
    ESP_LOGI(TAG, "Device OFF. Call power_on() to start.");
  }
}

void MeshtasticComponent::loop() {
  uint32_t now = millis();

  if (this->state_ != this->last_published_state_) {
    this->last_published_state_ = this->state_;
    this->publish_state_();
  }

  switch (this->state_) {
    case State::OFF:
      break;

    case State::POWERING_ON:
      // Wait 3s for the Meshtastic device to boot after power-on
      if (now - this->state_start_ > 3000) {
        ESP_LOGD(TAG, "Boot delay elapsed, sending wake sequence");
        this->state_ = State::INITIALIZING;
        this->state_start_ = now;
        this->send_wake_bytes_();
      }
      break;

    case State::INITIALIZING:
      // Wait 200ms after wake bytes, then request config
      if (now - this->state_start_ > 200) {
        this->send_want_config_();
        this->state_ = State::CONFIGURING;
        this->state_start_ = now;
      }
      break;

    case State::CONFIGURING:
      this->process_serial_();
      // process_serial_ may have changed state (config complete -> APPLYING_CONFIG or READY)
      if (this->state_ != State::CONFIGURING)
        break;
      if (now - this->boot_start_ > this->boot_timeout_) {
        ESP_LOGW(TAG, "Boot timeout after %us - device not responding. Restarting.",
                 this->boot_timeout_ / 1000);
        this->boot_start_ = now;
      }
      // Retry wake + config every 5 seconds until device responds
      if (now - this->state_start_ > 5000) {
        ESP_LOGD(TAG, "No config response, retrying wake + config...");
        this->state_ = State::INITIALIZING;
        this->state_start_ = now;
        this->send_wake_bytes_();
      }
      break;

    case State::APPLYING_CONFIG:
      this->process_serial_();
      if (this->state_ != State::APPLYING_CONFIG)
        break;

      // Phase 0: sending settings batch (begin_edit + set_config/set_module + commit_edit)
      if (this->config_phase_ == 0) {
        // Send messages with 200ms spacing
        if (now - this->state_start_ > 200) {
          if (this->config_msg_index_ < this->config_admin_msgs_.size()) {
            this->send_next_config_msg_();
            this->state_start_ = now;
          } else {
            // All settings sent (including commit_edit), wait for flash write
            ESP_LOGI(TAG, "All settings sent, waiting for commit to complete...");
            this->config_phase_ = 1;
            this->state_start_ = now;
          }
        }
      }
      // Phase 1: wait for commit_edit_settings to finish writing to flash
      else if (this->config_phase_ == 1) {
        // commit_edit saves settings to flash without needing a reboot.
        // If the device does reboot (some firmware versions), the reboot
        // handler will detect it and re-init. Otherwise, proceed after 2s.
        if (now - this->state_start_ > 2000) {
          if (this->has_channel_config_) {
            ESP_LOGI(TAG, "Settings committed, sending channel config...");
            this->config_phase_ = 2;
            this->state_start_ = now;
          } else {
            ESP_LOGI(TAG, "Settings committed, sending reboot...");
            this->config_phase_ = 3;
            this->state_start_ = now;
          }
        }
      }
      // Phase 2: sending channel config
      else if (this->config_phase_ == 2) {
        if (now - this->state_start_ > 500) {
          if (this->has_channel_config_) {
            ESP_LOGI(TAG, "Sending channel configuration...");
            this->send_admin_message_(this->channel_admin_msg_.data, this->channel_admin_msg_.len);
          }
          this->config_phase_ = 3;
          this->state_start_ = now;
        }
      }
      // Phase 3: send reboot to persist all config changes
      else if (this->config_phase_ == 3) {
        if (now - this->state_start_ > 500) {
          ESP_LOGI(TAG, "Sending reboot to apply configuration...");
          // AdminMessage { reboot_seconds = 2 } -> field 97, varint
          uint8_t reboot_msg[4];
          size_t rlen = this->encode_field_varint_(reboot_msg, 97, 2);
          this->send_admin_message_(reboot_msg, rlen);
          this->config_phase_ = 4;
          this->state_start_ = now;
        }
      }
      // Phase 4: wait for reboot, then ready
      else if (this->config_phase_ == 4) {
        // Reboot handler (case 8) or config_complete handler (case 7)
        // will detect the reboot and transition us.
        // If no reboot after 15s, go ready anyway.
        if (now - this->state_start_ > 15000) {
          ESP_LOGW(TAG, "No reboot detected after config, proceeding...");
          this->config_applied_ = true;
          this->state_ = State::READY;
          this->on_ready_callbacks_.call();
        }
      }
      break;

    case State::READY:
      this->process_serial_();
      break;

    case State::SENDING:
      this->process_serial_();
      // process_serial_ may have changed state (ACK received -> READY)
      if (this->state_ != State::SENDING)
        break;
      if (now - this->state_start_ > this->ack_timeout_) {
        ESP_LOGW(TAG, "ACK timeout for packet 0x%08X", this->pending_packet_id_);
        this->pending_packet_id_ = 0;
        this->state_ = State::READY;
        this->on_send_failed_callbacks_.call();
      }
      break;
  }
}

void MeshtasticComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Meshtastic:");
  if (this->power_pin_ != nullptr) {
    LOG_PIN("  Power Pin: ", this->power_pin_);
  }
  ESP_LOGCONFIG(TAG, "  Boot Timeout: %ums", this->boot_timeout_);
  ESP_LOGCONFIG(TAG, "  ACK Timeout: %ums", this->ack_timeout_);
  ESP_LOGCONFIG(TAG, "  Default Destination: 0x%08X", this->default_destination_);
  ESP_LOGCONFIG(TAG, "  Default Channel: %u", this->default_channel_);
  if (!this->config_admin_msgs_.empty()) {
    ESP_LOGCONFIG(TAG, "  Admin config messages: %u settings + %s (on_boot=%s)",
                  this->config_admin_msgs_.size(),
                  this->has_channel_config_ ? "channel" : "no channel",
                  this->configure_on_boot_ ? "yes" : "no");
  }
}

// ---- Public API ----

void MeshtasticComponent::power_on() {
  if (this->power_pin_ == nullptr) {
    ESP_LOGW(TAG, "No power pin configured, cannot power_on()");
    // If no power pin, just re-init in case user wants to reconnect
    this->state_ = State::INITIALIZING;
    this->state_start_ = millis();
    this->boot_start_ = this->state_start_;
    this->send_wake_bytes_();
    return;
  }
  ESP_LOGI(TAG, "Powering ON Meshtastic device");
  this->power_pin_->digital_write(true);
  this->state_ = State::POWERING_ON;
  this->state_start_ = millis();
  this->boot_start_ = this->state_start_;
  this->rx_state_ = RX_WAIT_START1;
  this->rx_pos_ = 0;
  this->my_node_num_ = 0;
  this->my_long_name_.clear();
  this->my_short_name_.clear();
  this->pending_packet_id_ = 0;
  // Reset config state
  this->config_msg_index_ = 0;
  this->config_phase_ = 0;
}

void MeshtasticComponent::power_off() {
  if (this->power_pin_ == nullptr) {
    ESP_LOGW(TAG, "No power pin configured, cannot power_off()");
    return;
  }
  ESP_LOGI(TAG, "Powering OFF Meshtastic device");
  this->power_pin_->digital_write(false);
  this->state_ = State::OFF;
  this->pending_packet_id_ = 0;
  this->my_node_num_ = 0;
  this->my_long_name_.clear();
  this->my_short_name_.clear();
}

bool MeshtasticComponent::send_text(const std::string &message, uint32_t destination, uint8_t channel) {
  if (this->state_ != State::READY && this->state_ != State::SENDING) {
    ESP_LOGW(TAG, "Cannot send text, not ready (state=%d)", static_cast<int>(this->state_));
    return false;
  }

  if (message.length() > 233) {
    ESP_LOGW(TAG, "Message too long (%u bytes, max 233)", message.length());
    return false;
  }

  uint32_t packet_id = this->generate_packet_id_();

  // Encode Data sub-message: portnum=TEXT_MESSAGE(1), payload=message bytes
  uint8_t data_buf[256];
  size_t data_len = 0;
  data_len += this->encode_field_varint_(data_buf + data_len, 1, PORTNUM_TEXT_MESSAGE);
  data_len += this->encode_field_bytes_(data_buf + data_len, 2,
                                        reinterpret_cast<const uint8_t *>(message.c_str()), message.length());

  // Encode MeshPacket sub-message
  uint8_t pkt_buf[300];
  size_t pkt_len = 0;
  pkt_len += this->encode_field_fixed32_(pkt_buf + pkt_len, 2, destination);   // to
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 3, channel);        // channel
  pkt_len += this->encode_field_bytes_(pkt_buf + pkt_len, 4, data_buf, data_len);  // decoded (Data)
  pkt_len += this->encode_field_fixed32_(pkt_buf + pkt_len, 6, packet_id);     // id
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 9, 3);             // hop_limit
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 10, 1);            // want_ack = true
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 11, 70);           // priority = RELIABLE

  // Encode ToRadio: packet (field 1, length-delimited)
  uint8_t toradio_buf[320];
  size_t toradio_len = 0;
  toradio_len += this->encode_field_bytes_(toradio_buf + toradio_len, 1, pkt_buf, pkt_len);

  this->send_framed_(toradio_buf, toradio_len);

  ESP_LOGI(TAG, "Sent text (id=0x%08X dest=0x%08X ch=%u len=%u): %s",
           packet_id, destination, channel, message.length(), message.c_str());

  this->pending_packet_id_ = packet_id;
  this->pending_destination_ = destination;
  this->state_ = State::SENDING;
  this->state_start_ = millis();
  return true;
}

bool MeshtasticComponent::send_nodeinfo() {
  if (this->state_ != State::READY) {
    ESP_LOGW(TAG, "Cannot send nodeinfo, not ready (state=%d)", static_cast<int>(this->state_));
    return false;
  }
  if (this->my_node_num_ == 0) {
    ESP_LOGW(TAG, "Cannot send nodeinfo, node number not known");
    return false;
  }

  // Build User protobuf
  // field 1: id (string) "!XXXXXXXX", field 2: long_name, field 3: short_name
  char id_str[12];
  snprintf(id_str, sizeof(id_str), "!%08x", this->my_node_num_);

  uint8_t user_buf[128];
  size_t user_len = 0;
  user_len += this->encode_field_bytes_(user_buf + user_len, 1,
    reinterpret_cast<const uint8_t *>(id_str), strlen(id_str));
  if (!this->my_long_name_.empty()) {
    user_len += this->encode_field_bytes_(user_buf + user_len, 2,
      reinterpret_cast<const uint8_t *>(this->my_long_name_.c_str()), this->my_long_name_.length());
  }
  if (!this->my_short_name_.empty()) {
    user_len += this->encode_field_bytes_(user_buf + user_len, 3,
      reinterpret_cast<const uint8_t *>(this->my_short_name_.c_str()), this->my_short_name_.length());
  }

  // Encode Data: portnum=NODEINFO_APP(4), payload=User bytes
  uint8_t data_buf[192];
  size_t data_len = 0;
  data_len += this->encode_field_varint_(data_buf + data_len, 1, PORTNUM_NODEINFO_APP);
  data_len += this->encode_field_bytes_(data_buf + data_len, 2, user_buf, user_len);

  // Encode MeshPacket: broadcast
  uint32_t packet_id = this->generate_packet_id_();
  uint8_t pkt_buf[256];
  size_t pkt_len = 0;
  pkt_len += this->encode_field_fixed32_(pkt_buf + pkt_len, 2, 0xFFFFFFFF);  // to (broadcast)
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 3, 0);             // channel 0
  pkt_len += this->encode_field_bytes_(pkt_buf + pkt_len, 4, data_buf, data_len);
  pkt_len += this->encode_field_fixed32_(pkt_buf + pkt_len, 6, packet_id);
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 9, 3);             // hop_limit
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 11, 70);           // priority = RELIABLE

  // ToRadio: packet (field 1)
  uint8_t toradio_buf[280];
  size_t toradio_len = 0;
  toradio_len += this->encode_field_bytes_(toradio_buf + toradio_len, 1, pkt_buf, pkt_len);

  this->send_framed_(toradio_buf, toradio_len);

  ESP_LOGI(TAG, "Sent nodeinfo broadcast (id=0x%08X, long=%s, short=%s)",
           packet_id, this->my_long_name_.c_str(), this->my_short_name_.c_str());
  return true;
}

// ---- Admin config helpers ----

void MeshtasticComponent::send_admin_message_(const uint8_t *admin_payload, size_t admin_len) {
  // Wrap admin payload in: Data { portnum=ADMIN(6), payload=admin_payload }
  uint8_t data_buf[256];
  size_t data_len = 0;
  data_len += this->encode_field_varint_(data_buf + data_len, 1, PORTNUM_ADMIN);
  data_len += this->encode_field_bytes_(data_buf + data_len, 2, admin_payload, admin_len);

  // MeshPacket { to=my_node_num, channel=0, decoded=data, id=..., hop_limit=3, want_ack=true }
  uint8_t pkt_buf[300];
  size_t pkt_len = 0;
  pkt_len += this->encode_field_fixed32_(pkt_buf + pkt_len, 2, this->my_node_num_);  // to self
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 3, 0);                     // channel 0
  pkt_len += this->encode_field_bytes_(pkt_buf + pkt_len, 4, data_buf, data_len);
  uint32_t pkt_id = this->generate_packet_id_();
  pkt_len += this->encode_field_fixed32_(pkt_buf + pkt_len, 6, pkt_id);
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 9, 3);   // hop_limit
  pkt_len += this->encode_field_varint_(pkt_buf + pkt_len, 10, 1);  // want_ack

  // ToRadio { packet (field 1) = pkt }
  uint8_t toradio_buf[320];
  size_t toradio_len = 0;
  toradio_len += this->encode_field_bytes_(toradio_buf + toradio_len, 1, pkt_buf, pkt_len);

  this->send_framed_(toradio_buf, toradio_len);
  ESP_LOGD(TAG, "Sent admin message (%u bytes, pkt_id=0x%08X)", admin_len, pkt_id);
}

void MeshtasticComponent::send_next_config_msg_() {
  if (this->config_msg_index_ >= this->config_admin_msgs_.size())
    return;

  const auto &msg = this->config_admin_msgs_[this->config_msg_index_];
  ESP_LOGI(TAG, "Sending config message %u/%u...", this->config_msg_index_ + 1,
           this->config_admin_msgs_.size());
  this->send_admin_message_(msg.data, msg.len);
  this->config_msg_index_++;
}

void MeshtasticComponent::apply_config() {
  if (this->config_admin_msgs_.empty()) {
    ESP_LOGW(TAG, "No admin config messages to apply");
    return;
  }
  if (this->my_node_num_ == 0) {
    ESP_LOGW(TAG, "Cannot apply config - node number not known yet");
    return;
  }
  ESP_LOGI(TAG, "Applying admin configuration on demand...");
  this->config_msg_index_ = 0;
  this->config_phase_ = 0;
  this->config_applied_ = false;
  this->state_ = State::APPLYING_CONFIG;
  this->state_start_ = millis();
}

// ---- Debug config logging ----

// Field name lookup for human-readable output
static const char *field_name_(const char *section, uint32_t num) {
  if (strcmp(section, "lora") == 0) {
    switch (num) {
      case 1: return "use_preset"; case 2: return "modem_preset"; case 3: return "bandwidth";
      case 4: return "spread_factor"; case 5: return "coding_rate"; case 7: return "region";
      case 8: return "hop_limit"; case 9: return "tx_enabled"; case 10: return "tx_power";
      case 11: return "channel_num"; case 12: return "override_duty_cycle";
      case 104: return "ignore_mqtt"; case 105: return "config_ok_to_mqtt";
    }
  } else if (strcmp(section, "bluetooth") == 0) {
    switch (num) { case 1: return "enabled"; case 2: return "mode"; case 3: return "fixed_pin"; }
  } else if (strcmp(section, "network") == 0) {
    switch (num) {
      case 1: return "wifi_enabled"; case 3: return "wifi_ssid"; case 4: return "wifi_psk";
      case 5: return "ntp_server"; case 6: return "eth_enabled";
    }
  } else if (strcmp(section, "mqtt") == 0) {
    switch (num) {
      case 1: return "enabled"; case 2: return "address"; case 3: return "username";
      case 4: return "password"; case 5: return "encryption_enabled";
      case 6: return "json_enabled"; case 7: return "tls_enabled"; case 8: return "root";
    }
  } else if (strcmp(section, "serial") == 0) {
    switch (num) {
      case 1: return "enabled"; case 2: return "echo"; case 3: return "rxd"; case 4: return "txd";
      case 5: return "baud"; case 7: return "mode";
    }
  } else if (strcmp(section, "ch_settings") == 0) {
    switch (num) {
      case 2: return "psk"; case 3: return "name"; case 5: return "uplink_enabled";
      case 6: return "downlink_enabled";
    }
  } else if (strcmp(section, "device") == 0) {
    switch (num) {
      case 1: return "role"; case 6: return "rebroadcast_mode";
      case 7: return "node_info_broadcast_secs"; case 11: return "tzdef";
    }
  } else if (strcmp(section, "power") == 0) {
    switch (num) {
      case 4: return "sds_secs"; case 6: return "min_wake_secs";
      case 7: return "ls_secs"; case 8: return "wait_bluetooth_secs";
    }
  }
  return nullptr;
}

void MeshtasticComponent::log_proto_fields_(const char *label, const uint8_t *buf, size_t len) {
  size_t pos = 0;
  while (pos < len) {
    uint32_t tag = this->decode_varint_(buf, &pos, len);
    uint8_t wire_type = tag & 0x07;
    uint32_t field_num = tag >> 3;
    const char *name = field_name_(label, field_num);

    if (wire_type == WT_VARINT) {
      uint32_t val = this->decode_varint_(buf, &pos, len);
      if (name)
        ESP_LOGI(TAG, "  %s = %u", name, val);
      else
        ESP_LOGI(TAG, "  %s.%u = %u", label, field_num, val);
    } else if (wire_type == WT_LENGTH) {
      uint32_t sub_len = this->decode_varint_(buf, &pos, len);
      if (pos + sub_len > len) break;
      bool is_text = sub_len > 0 && sub_len < 128;
      for (size_t i = 0; i < sub_len && is_text; i++) {
        if (buf[pos + i] < 0x20 || buf[pos + i] > 0x7E) is_text = false;
      }
      if (is_text) {
        std::string s(reinterpret_cast<const char *>(buf + pos), sub_len);
        if (name)
          ESP_LOGI(TAG, "  %s = \"%s\"", name, s.c_str());
        else
          ESP_LOGI(TAG, "  %s.%u = \"%s\"", label, field_num, s.c_str());
      } else {
        if (name)
          ESP_LOGI(TAG, "  %s = (%u bytes)", name, sub_len);
        else
          ESP_LOGI(TAG, "  %s.%u = (%u bytes)", label, field_num, sub_len);
      }
      pos += sub_len;
    } else if (wire_type == WT_32BIT) {
      uint32_t val = this->decode_fixed32_(buf, &pos);
      if (name)
        ESP_LOGI(TAG, "  %s = 0x%08X", name, val);
      else
        ESP_LOGI(TAG, "  %s.%u = 0x%08X", label, field_num, val);
    } else {
      this->skip_field_(buf, &pos, len, wire_type);
    }
  }
}

void MeshtasticComponent::dump_radio_config() {
  if (this->state_ == State::OFF || this->state_ == State::POWERING_ON) {
    ESP_LOGW(TAG, "Cannot dump config - device is off");
    return;
  }
  // Send want_config_id to trigger a full config dump from the device.
  // The device will respond with all Config, ModuleConfig, and Channel messages.
  // Our FromRadio handlers (cases 5, 6, 9) will log everything.
  // Using nonce=0 so config_complete_id won't match and won't trigger state changes.
  ESP_LOGI(TAG, "=== Requesting radio config dump ===");
  this->dump_config_active_ = true;
  uint8_t buf[10];
  size_t len = this->encode_field_varint_(buf, 3, 0xDEAD);  // dummy nonce
  this->send_framed_(buf, len);
}

// ---- Internal methods ----

void MeshtasticComponent::publish_state_() {
#ifdef USE_BINARY_SENSOR
  if (this->ready_sensor_)
    this->ready_sensor_->publish_state(this->state_ == State::READY);
#endif
#ifdef USE_TEXT_SENSOR
  if (this->state_sensor_) {
    const char *s;
    switch (this->state_) {
      case State::OFF: s = "Off"; break;
      case State::POWERING_ON: s = "Booting"; break;
      case State::INITIALIZING: s = "Initializing"; break;
      case State::CONFIGURING: s = "Configuring"; break;
      case State::APPLYING_CONFIG: s = "Applying Config"; break;
      case State::READY: s = "Ready"; break;
      case State::SENDING: s = "Sending"; break;
      default: s = "Unknown"; break;
    }
    this->state_sensor_->publish_state(s);
  }
#endif
}

void MeshtasticComponent::send_wake_bytes_() {
  ESP_LOGD(TAG, "Sending 32 wake bytes (0xC3)");
  uint8_t wake[32];
  memset(wake, START2, sizeof(wake));
  this->write_array(wake, sizeof(wake));
  this->flush();
}

void MeshtasticComponent::send_want_config_() {
  // Generate a random nonzero nonce
  this->config_nonce_ = static_cast<uint32_t>(esp_random()) | 1;
  ESP_LOGD(TAG, "Requesting config (nonce=0x%08X)", this->config_nonce_);

  // ToRadio { want_config_id = nonce }  =>  field 3, varint
  uint8_t buf[10];
  size_t len = this->encode_field_varint_(buf, 3, this->config_nonce_);
  this->send_framed_(buf, len);
}

void MeshtasticComponent::process_serial_() {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);

    switch (this->rx_state_) {
      case RX_WAIT_START1:
        if (byte == START1) {
          this->rx_state_ = RX_WAIT_START2;
        }
        break;

      case RX_WAIT_START2:
        if (byte == START2) {
          this->rx_state_ = RX_WAIT_LEN_MSB;
        } else {
          this->rx_state_ = RX_WAIT_START1;
        }
        break;

      case RX_WAIT_LEN_MSB:
        this->rx_payload_len_ = static_cast<uint16_t>(byte) << 8;
        this->rx_state_ = RX_WAIT_LEN_LSB;
        break;

      case RX_WAIT_LEN_LSB:
        this->rx_payload_len_ |= byte;
        if (this->rx_payload_len_ > MAX_PAYLOAD || this->rx_payload_len_ == 0) {
          if (this->rx_payload_len_ > MAX_PAYLOAD) {
            ESP_LOGW(TAG, "Frame too large (%u bytes), discarding", this->rx_payload_len_);
          }
          this->rx_state_ = RX_WAIT_START1;
        } else {
          this->rx_pos_ = 0;
          this->rx_state_ = RX_WAIT_PAYLOAD;
        }
        break;

      case RX_WAIT_PAYLOAD:
        this->rx_buf_[this->rx_pos_++] = byte;
        if (this->rx_pos_ >= this->rx_payload_len_) {
          this->handle_from_radio_(this->rx_buf_, this->rx_payload_len_);
          this->rx_state_ = RX_WAIT_START1;
        }
        break;
    }
  }
}

void MeshtasticComponent::handle_from_radio_(const uint8_t *buf, size_t len) {
  // Parse FromRadio protobuf
  size_t pos = 0;

  while (pos < len) {
    if (pos >= len) break;
    uint32_t tag = this->decode_varint_(buf, &pos, len);
    uint8_t wire_type = tag & 0x07;
    uint32_t field_num = tag >> 3;

    ESP_LOGD(TAG, "FromRadio field=%u wire_type=%u (len=%u)", field_num, wire_type, len);

    switch (field_num) {
      case 1:  // id (varint)
        if (wire_type == WT_VARINT) {
          uint32_t fr_id = this->decode_varint_(buf, &pos, len);
          ESP_LOGD(TAG, "  FromRadio.id = %u", fr_id);
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;

      case 2: {  // packet (MeshPacket, length-delimited)
        if (wire_type == WT_LENGTH) {
          uint32_t sub_len = this->decode_varint_(buf, &pos, len);
          if (pos + sub_len <= len) {
            this->handle_mesh_packet_(buf + pos, sub_len);
          }
          pos += sub_len;
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;
      }

      case 3: {  // my_info (MyNodeInfo, length-delimited)
        if (wire_type == WT_LENGTH) {
          uint32_t sub_len = this->decode_varint_(buf, &pos, len);
          size_t end = pos + sub_len;
          // Extract my_node_num (field 1, varint)
          while (pos < end) {
            uint32_t sub_tag = this->decode_varint_(buf, &pos, end);
            uint8_t sub_wt = sub_tag & 0x07;
            uint32_t sub_fn = sub_tag >> 3;
            if (sub_fn == 1 && sub_wt == WT_VARINT) {
              this->my_node_num_ = this->decode_varint_(buf, &pos, end);
              ESP_LOGI(TAG, "My node number: 0x%08X", this->my_node_num_);
            } else {
              this->skip_field_(buf, &pos, end, sub_wt);
            }
          }
          pos = end;
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;
      }

      case 4: {  // node_info (NodeInfo, length-delimited)
        if (wire_type == WT_LENGTH) {
          uint32_t sub_len = this->decode_varint_(buf, &pos, len);
          size_t end = pos + sub_len;
          uint32_t node_num = 0;
          const uint8_t *user_data = nullptr;
          size_t user_len = 0;

          // Parse NodeInfo: num (field 1, varint), user (field 2, length-delimited)
          while (pos < end) {
            uint32_t sub_tag = this->decode_varint_(buf, &pos, end);
            uint8_t sub_wt = sub_tag & 0x07;
            uint32_t sub_fn = sub_tag >> 3;
            if (sub_fn == 1 && sub_wt == WT_VARINT) {
              node_num = this->decode_varint_(buf, &pos, end);
            } else if (sub_fn == 2 && sub_wt == WT_LENGTH) {
              user_len = this->decode_varint_(buf, &pos, end);
              user_data = buf + pos;
              pos += user_len;
            } else {
              this->skip_field_(buf, &pos, end, sub_wt);
            }
          }

          // If this is our own node, extract long_name and short_name from User
          if (node_num == this->my_node_num_ && this->my_node_num_ != 0 && user_data != nullptr) {
            size_t upos = 0;
            while (upos < user_len) {
              uint32_t utag = this->decode_varint_(user_data, &upos, user_len);
              uint8_t uwt = utag & 0x07;
              uint32_t ufn = utag >> 3;
              if (ufn == 2 && uwt == WT_LENGTH) {  // long_name
                uint32_t slen = this->decode_varint_(user_data, &upos, user_len);
                this->my_long_name_ = std::string(reinterpret_cast<const char *>(user_data + upos), slen);
                upos += slen;
                ESP_LOGI(TAG, "My long name: %s", this->my_long_name_.c_str());
              } else if (ufn == 3 && uwt == WT_LENGTH) {  // short_name
                uint32_t slen = this->decode_varint_(user_data, &upos, user_len);
                this->my_short_name_ = std::string(reinterpret_cast<const char *>(user_data + upos), slen);
                upos += slen;
                ESP_LOGI(TAG, "My short name: %s", this->my_short_name_.c_str());
              } else {
                this->skip_field_(user_data, &upos, user_len, uwt);
              }
            }
          }

          pos = end;
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;
      }

      case 5: {  // config (Config, length-delimited)
        if (wire_type == WT_LENGTH) {
          uint32_t sub_len = this->decode_varint_(buf, &pos, len);
          if (this->dump_config_active_) {
            size_t end = pos + sub_len;
            static const char *const CFG_NAMES[] = {
              "?", "device", "position", "power", "network",
              "display", "lora", "bluetooth", "security", "sessionkey", "device_ui"
            };
            while (pos < end) {
              uint32_t sub_tag = this->decode_varint_(buf, &pos, end);
              uint8_t sub_wt = sub_tag & 0x07;
              uint32_t sub_fn = sub_tag >> 3;
              if (sub_wt == WT_LENGTH) {
                uint32_t cfg_len = this->decode_varint_(buf, &pos, end);
                const char *name = sub_fn <= 10 ? CFG_NAMES[sub_fn] : "unknown";
                ESP_LOGI(TAG, "Config.%s:", name);
                if (pos + cfg_len <= end) {
                  this->log_proto_fields_(name, buf + pos, cfg_len);
                }
                pos += cfg_len;
              } else {
                this->skip_field_(buf, &pos, end, sub_wt);
              }
            }
            pos = end;
          } else {
            pos += sub_len;
          }
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;
      }

      case 6: {  // module_config (ModuleConfig, length-delimited)
        if (wire_type == WT_LENGTH) {
          uint32_t sub_len = this->decode_varint_(buf, &pos, len);
          if (this->dump_config_active_) {
            size_t end = pos + sub_len;
            static const char *const MOD_NAMES[] = {
              "?", "mqtt", "serial", "ext_notif", "store_fwd",
              "range_test", "telemetry", "canned_msg", "audio",
              "remote_hw", "neighbor_info", "ambient_light", "detection_sensor",
              "paxcounter", "statusmessage", "traffic_mgmt"
            };
            while (pos < end) {
              uint32_t sub_tag = this->decode_varint_(buf, &pos, end);
              uint8_t sub_wt = sub_tag & 0x07;
              uint32_t sub_fn = sub_tag >> 3;
              if (sub_wt == WT_LENGTH) {
                uint32_t cfg_len = this->decode_varint_(buf, &pos, end);
                const char *name = sub_fn <= 15 ? MOD_NAMES[sub_fn] : "unknown";
                ESP_LOGI(TAG, "ModuleConfig.%s:", name);
                if (pos + cfg_len <= end) {
                  this->log_proto_fields_(name, buf + pos, cfg_len);
                }
                pos += cfg_len;
              } else {
                this->skip_field_(buf, &pos, end, sub_wt);
              }
            }
            pos = end;
          } else {
            pos += sub_len;
          }
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;
      }

      case 7:  // config_complete_id (varint)
        if (wire_type == WT_VARINT) {
          uint32_t complete_id = this->decode_varint_(buf, &pos, len);
          // End of dump_radio_config output
          if (this->dump_config_active_) {
            this->dump_config_active_ = false;
            ESP_LOGI(TAG, "=== End radio config dump ===");
          }
          if (complete_id == this->config_nonce_ && this->state_ == State::CONFIGURING) {
            // If we have admin config to apply and haven't applied it yet, go to APPLYING_CONFIG
            if (!this->config_admin_msgs_.empty() && !this->config_applied_ && this->configure_on_boot_) {
              ESP_LOGI(TAG, "Config complete - applying admin configuration...");
              this->state_ = State::APPLYING_CONFIG;
              this->state_start_ = millis();
              this->config_msg_index_ = 0;
              this->config_phase_ = 0;
            } else {
              ESP_LOGI(TAG, "Config complete - device ready");
              this->state_ = State::READY;
              this->on_ready_callbacks_.call();
            }
          }
          // Handle config_complete after reboot during APPLYING_CONFIG
          else if (complete_id != 0 && this->state_ == State::APPLYING_CONFIG) {
            if (this->config_phase_ == 1) {
              ESP_LOGI(TAG, "Device rebooted and ready after settings commit");
              if (this->has_channel_config_) {
                this->config_phase_ = 2;
                this->state_start_ = millis();
              } else {
                // No channel config — send reboot to apply settings
                ESP_LOGI(TAG, "No channel config, sending reboot to apply...");
                this->config_phase_ = 3;
                this->state_start_ = millis();
              }
            } else if (this->config_phase_ == 4) {
              ESP_LOGI(TAG, "Device rebooted - all config applied");
              this->config_applied_ = true;
              this->state_ = State::READY;
              this->on_ready_callbacks_.call();
            }
          }
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;

      case 8:  // rebooted (varint/bool)
        if (wire_type == WT_VARINT) {
          uint32_t rebooted = this->decode_varint_(buf, &pos, len);
          if (rebooted) {
            if (this->state_ == State::APPLYING_CONFIG &&
                (this->config_phase_ == 1 || this->config_phase_ == 4)) {
              // Expected reboot after settings commit or channel config — re-init
              ESP_LOGI(TAG, "Device rebooted (phase %u), re-initializing...", this->config_phase_);
              // Stay in APPLYING_CONFIG at current phase
              // Re-send wake + want_config to get config_complete_id
              this->send_wake_bytes_();
              this->config_nonce_ = static_cast<uint32_t>(esp_random()) | 1;
              uint8_t cfg_buf[10];
              size_t cfg_len = this->encode_field_varint_(cfg_buf, 3, this->config_nonce_);
              this->send_framed_(cfg_buf, cfg_len);
            } else {
              ESP_LOGW(TAG, "Device reboot detected, re-initializing");
              this->state_ = State::INITIALIZING;
              this->state_start_ = millis();
            }
          }
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;

      case 9: {  // channel (Channel, length-delimited)
        if (wire_type == WT_LENGTH) {
          uint32_t sub_len = this->decode_varint_(buf, &pos, len);
          if (this->dump_config_active_) {
            size_t end = pos + sub_len;
            uint32_t ch_index = 0, ch_role = 0;
            const uint8_t *settings_data = nullptr;
            size_t settings_len = 0;
            while (pos < end) {
              uint32_t sub_tag = this->decode_varint_(buf, &pos, end);
              uint8_t sub_wt = sub_tag & 0x07;
              uint32_t sub_fn = sub_tag >> 3;
              if (sub_fn == 1 && sub_wt == WT_VARINT) {
                ch_index = this->decode_varint_(buf, &pos, end);
              } else if (sub_fn == 2 && sub_wt == WT_LENGTH) {
                settings_len = this->decode_varint_(buf, &pos, end);
                settings_data = buf + pos;
                pos += settings_len;
              } else if (sub_fn == 3 && sub_wt == WT_VARINT) {
                ch_role = this->decode_varint_(buf, &pos, end);
              } else {
                this->skip_field_(buf, &pos, end, sub_wt);
              }
            }
            static const char *const ROLE_NAMES[] = {"DISABLED", "PRIMARY", "SECONDARY"};
            const char *role_name = ch_role <= 2 ? ROLE_NAMES[ch_role] : "?";
            ESP_LOGI(TAG, "Channel %u (%s):", ch_index, role_name);
            if (settings_data != nullptr && settings_len > 0) {
              this->log_proto_fields_("ch_settings", settings_data, settings_len);
            }
            pos = end;
          } else {
            pos += sub_len;
          }
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;
      }

      case 11: {  // queueStatus (QueueStatus, length-delimited)
        if (wire_type == WT_LENGTH) {
          uint32_t sub_len = this->decode_varint_(buf, &pos, len);
          size_t end = pos + sub_len;
          int32_t res = 0;
          uint32_t mesh_packet_id = 0;
          while (pos < end) {
            uint32_t sub_tag = this->decode_varint_(buf, &pos, end);
            uint8_t sub_wt = sub_tag & 0x07;
            uint32_t sub_fn = sub_tag >> 3;
            if (sub_fn == 1 && sub_wt == WT_VARINT) {
              res = static_cast<int32_t>(this->decode_varint_(buf, &pos, end));
            } else if (sub_fn == 4 && sub_wt == WT_VARINT) {
              mesh_packet_id = this->decode_varint_(buf, &pos, end);
            } else {
              this->skip_field_(buf, &pos, end, sub_wt);
            }
          }
          ESP_LOGD(TAG, "QueueStatus: res=%d packet_id=0x%08X (pending=0x%08X)", res, mesh_packet_id, this->pending_packet_id_);
          if (mesh_packet_id != 0 && mesh_packet_id == this->pending_packet_id_) {
            if (res != 0) {
              ESP_LOGW(TAG, "Message queue failed (res=%d)", res);
              this->pending_packet_id_ = 0;
              this->state_ = State::READY;
              this->on_send_failed_callbacks_.call();
            } else {
              ESP_LOGD(TAG, "Message queued, waiting for routing confirmation...");
            }
          }
          pos = end;
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;
      }

      default:
        this->skip_field_(buf, &pos, len, wire_type);
        break;
    }
  }
}

void MeshtasticComponent::handle_mesh_packet_(const uint8_t *buf, size_t len) {
  size_t pos = 0;
  uint32_t from = 0;
  uint32_t to = 0;
  uint8_t channel = 0;
  const uint8_t *decoded_data = nullptr;
  size_t decoded_len = 0;

  // First pass: extract MeshPacket fields
  while (pos < len) {
    uint32_t tag = this->decode_varint_(buf, &pos, len);
    uint8_t wire_type = tag & 0x07;
    uint32_t field_num = tag >> 3;

    switch (field_num) {
      case 1:  // from (fixed32)
        if (wire_type == WT_32BIT) {
          from = this->decode_fixed32_(buf, &pos);
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;

      case 2:  // to (fixed32)
        if (wire_type == WT_32BIT) {
          to = this->decode_fixed32_(buf, &pos);
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;

      case 3:  // channel (varint)
        if (wire_type == WT_VARINT) {
          channel = static_cast<uint8_t>(this->decode_varint_(buf, &pos, len));
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;

      case 4:  // decoded (Data, length-delimited)
        if (wire_type == WT_LENGTH) {
          decoded_len = this->decode_varint_(buf, &pos, len);
          decoded_data = buf + pos;
          pos += decoded_len;
        } else {
          this->skip_field_(buf, &pos, len, wire_type);
        }
        break;

      default:
        this->skip_field_(buf, &pos, len, wire_type);
        break;
    }
  }

  ESP_LOGD(TAG, "MeshPacket from=0x%08X to=0x%08X ch=%u decoded=%s (len=%u)",
           from, to, channel, decoded_data ? "yes" : "no/encrypted", decoded_len);

  if (decoded_data == nullptr) {
    return;  // Encrypted packet or no decoded data
  }

  // Parse the Data sub-message
  uint32_t portnum = 0;
  const uint8_t *payload = nullptr;
  size_t payload_len = 0;
  uint32_t request_id = 0;

  pos = 0;
  while (pos < decoded_len) {
    uint32_t tag = this->decode_varint_(decoded_data, &pos, decoded_len);
    uint8_t wire_type = tag & 0x07;
    uint32_t field_num = tag >> 3;

    switch (field_num) {
      case 1:  // portnum (varint)
        if (wire_type == WT_VARINT) {
          portnum = this->decode_varint_(decoded_data, &pos, decoded_len);
        } else {
          this->skip_field_(decoded_data, &pos, decoded_len, wire_type);
        }
        break;

      case 2:  // payload (bytes, length-delimited)
        if (wire_type == WT_LENGTH) {
          payload_len = this->decode_varint_(decoded_data, &pos, decoded_len);
          payload = decoded_data + pos;
          pos += payload_len;
        } else {
          this->skip_field_(decoded_data, &pos, decoded_len, wire_type);
        }
        break;

      case 6:  // request_id (fixed32)
        if (wire_type == WT_32BIT) {
          request_id = this->decode_fixed32_(decoded_data, &pos);
        } else {
          this->skip_field_(decoded_data, &pos, decoded_len, wire_type);
        }
        break;

      default:
        this->skip_field_(decoded_data, &pos, decoded_len, wire_type);
        break;
    }
  }

  ESP_LOGD(TAG, "  Data: portnum=%u request_id=0x%08X payload_len=%u (pending=0x%08X)",
           portnum, request_id, payload_len, this->pending_packet_id_);

  // Handle TEXT_MESSAGE_APP
  if (portnum == PORTNUM_TEXT_MESSAGE && payload != nullptr && payload_len > 0) {
    std::string text(reinterpret_cast<const char *>(payload), payload_len);
    ESP_LOGI(TAG, "Received text from 0x%08X (ch=%u): %s", from, channel, text.c_str());
    this->last_from_node_ = from;
    this->last_channel_ = channel;
    this->on_message_callbacks_.call(std::move(text));
    return;
  }

  // Handle ROUTING_APP (ACK/NAK)
  if (portnum == PORTNUM_ROUTING && request_id != 0 && request_id == this->pending_packet_id_) {
    // Parse Routing message to get error_reason
    uint32_t error_reason = 0;
    if (payload != nullptr && payload_len > 0) {
      size_t rpos = 0;
      while (rpos < payload_len) {
        uint32_t rtag = this->decode_varint_(payload, &rpos, payload_len);
        uint8_t rwt = rtag & 0x07;
        uint32_t rfn = rtag >> 3;
        if (rfn == 3 && rwt == WT_VARINT) {  // error_reason = field 3
          error_reason = this->decode_varint_(payload, &rpos, payload_len);
        } else {
          this->skip_field_(payload, &rpos, payload_len, rwt);
        }
      }
    }

    this->pending_packet_id_ = 0;
    this->state_ = State::READY;

    if (error_reason == ROUTING_ERROR_NONE) {
      ESP_LOGI(TAG, "ACK received - message delivered successfully");
      this->on_send_success_callbacks_.call();
    } else {
      ESP_LOGW(TAG, "NAK received (error=%u) - message delivery failed", error_reason);
      this->on_send_failed_callbacks_.call();
    }
  }
}

void MeshtasticComponent::send_framed_(const uint8_t *data, size_t len) {
  uint8_t header[4];
  header[0] = START1;
  header[1] = START2;
  header[2] = (len >> 8) & 0xFF;
  header[3] = len & 0xFF;
  this->write_array(header, 4);
  this->write_array(data, len);
  this->flush();
}

uint32_t MeshtasticComponent::generate_packet_id_() {
  this->packet_id_counter_++;
  uint32_t random_part = esp_random() & 0x003FFFFF;  // 22 bits random
  return (random_part << 10) | (this->packet_id_counter_ & 0x3FF);
}

// ---- Protobuf encoding ----

size_t MeshtasticComponent::encode_varint_(uint8_t *buf, uint32_t value) {
  size_t i = 0;
  while (value > 0x7F) {
    buf[i++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  buf[i++] = value & 0x7F;
  return i;
}

size_t MeshtasticComponent::encode_field_varint_(uint8_t *buf, uint32_t field_num, uint32_t value) {
  size_t len = 0;
  len += this->encode_varint_(buf + len, (field_num << 3) | WT_VARINT);
  len += this->encode_varint_(buf + len, value);
  return len;
}

size_t MeshtasticComponent::encode_field_fixed32_(uint8_t *buf, uint32_t field_num, uint32_t value) {
  size_t len = 0;
  len += this->encode_varint_(buf + len, (field_num << 3) | WT_32BIT);
  buf[len++] = value & 0xFF;
  buf[len++] = (value >> 8) & 0xFF;
  buf[len++] = (value >> 16) & 0xFF;
  buf[len++] = (value >> 24) & 0xFF;
  return len;
}

size_t MeshtasticComponent::encode_field_bytes_(uint8_t *buf, uint32_t field_num, const uint8_t *data,
                                                size_t data_len) {
  size_t len = 0;
  len += this->encode_varint_(buf + len, (field_num << 3) | WT_LENGTH);
  len += this->encode_varint_(buf + len, data_len);
  memcpy(buf + len, data, data_len);
  len += data_len;
  return len;
}

// ---- Protobuf decoding ----

uint32_t MeshtasticComponent::decode_varint_(const uint8_t *buf, size_t *pos, size_t len) {
  uint32_t result = 0;
  uint32_t shift = 0;
  while (*pos < len) {
    uint8_t byte = buf[(*pos)++];
    result |= static_cast<uint32_t>(byte & 0x7F) << shift;
    if (!(byte & 0x80))
      break;
    shift += 7;
    if (shift >= 35)
      break;  // overflow guard
  }
  return result;
}

uint32_t MeshtasticComponent::decode_fixed32_(const uint8_t *buf, size_t *pos) {
  uint32_t v = buf[*pos] |
               (static_cast<uint32_t>(buf[*pos + 1]) << 8) |
               (static_cast<uint32_t>(buf[*pos + 2]) << 16) |
               (static_cast<uint32_t>(buf[*pos + 3]) << 24);
  *pos += 4;
  return v;
}

bool MeshtasticComponent::skip_field_(const uint8_t *buf, size_t *pos, size_t len, uint8_t wire_type) {
  switch (wire_type) {
    case WT_VARINT:
      this->decode_varint_(buf, pos, len);
      return true;
    case WT_64BIT:
      *pos += 8;
      return *pos <= len;
    case WT_LENGTH: {
      uint32_t sub_len = this->decode_varint_(buf, pos, len);
      *pos += sub_len;
      return *pos <= len;
    }
    case WT_32BIT:
      *pos += 4;
      return *pos <= len;
    default:
      return false;
  }
}

}  // namespace meshtastic
}  // namespace esphome
