#include "lorawan_bridge.h"
#include "esphome/core/log.h"

namespace esphome {
namespace lorawan_bridge {

static const char *const TAG = "lorawan_bridge";

// ---- Lifecycle ----

void LoRaWANBridgeComponent::setup() {
  if (this->power_pin_ != nullptr) {
    this->power_pin_->setup();
    this->power_pin_->digital_write(false);
  }

  if (this->enable_on_boot_) {
    this->power_on();
  } else {
    this->state_ = State::OFF;
    ESP_LOGI(TAG, "LoRaWAN bridge OFF. Call power_on() to start.");
  }
}

void LoRaWANBridgeComponent::loop() {
  uint32_t now = millis();

  switch (this->state_) {
    case State::OFF:
      break;

    case State::POWERING_ON:
      if (now - this->state_start_ > this->power_on_delay_) {
        ESP_LOGD(TAG, "Power-on delay elapsed, waiting for ready...");
        this->state_ = State::WAITING_READY;
        this->state_start_ = now;
      }
      break;

    case State::WAITING_READY:
      this->process_serial_();
      if (this->state_ == State::WAITING_READY &&
          now - this->state_start_ > this->boot_timeout_) {
        ESP_LOGW(TAG, "Boot timeout — XIAO did not send ready");
        // Stay in WAITING_READY, keep trying (XIAO may be slow)
      }
      break;

    case State::READY:
      this->process_serial_();
      break;

    case State::SENDING:
      this->process_serial_();
      if (this->state_ == State::SENDING &&
          now - this->state_start_ > this->ack_timeout_) {
        ESP_LOGW(TAG, "ACK timeout");
        this->state_ = State::READY;
        this->on_send_failed_callbacks_.call();
      }
      break;
  }
}

void LoRaWANBridgeComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LoRaWAN Bridge:");
  if (this->power_pin_ != nullptr) {
    LOG_PIN("  Power Pin: ", this->power_pin_);
  }
  ESP_LOGCONFIG(TAG, "  Power-On Delay: %ums", this->power_on_delay_);
  ESP_LOGCONFIG(TAG, "  Boot Timeout: %ums", this->boot_timeout_);
  ESP_LOGCONFIG(TAG, "  ACK Timeout: %ums", this->ack_timeout_);
}

// ---- Public API ----

void LoRaWANBridgeComponent::power_on() {
  if (this->power_pin_ == nullptr) {
    ESP_LOGW(TAG, "No power pin configured");
    this->state_ = State::WAITING_READY;
    this->state_start_ = millis();
    return;
  }
  ESP_LOGI(TAG, "Powering ON LoRaWAN bridge");
  this->power_pin_->digital_write(true);
  this->state_ = State::POWERING_ON;
  this->state_start_ = millis();
  this->rx_buffer_.clear();
}

void LoRaWANBridgeComponent::power_off() {
  if (this->power_pin_ == nullptr) {
    ESP_LOGW(TAG, "No power pin configured");
    return;
  }
  ESP_LOGI(TAG, "Powering OFF LoRaWAN bridge");
  this->power_pin_->digital_write(false);
  this->state_ = State::OFF;
}

bool LoRaWANBridgeComponent::send_text(const std::string &message) {
  if (this->state_ != State::READY) {
    ESP_LOGW(TAG, "Cannot send, not ready (state=%d)", static_cast<int>(this->state_));
    return false;
  }

  ESP_LOGI(TAG, "Sending (%u bytes): %.64s%s", message.length(), message.c_str(),
           message.length() > 64 ? "..." : "");

  // Write payload + newline to UART
  this->write_array(reinterpret_cast<const uint8_t *>(message.c_str()), message.size());
  uint8_t nl = '\n';
  this->write_array(&nl, 1);
  this->flush();

  this->state_ = State::SENDING;
  this->state_start_ = millis();
  return true;
}

// ---- Serial processing ----

void LoRaWANBridgeComponent::process_serial_() {
  while (this->available()) {
    uint8_t b;
    this->read_byte(&b);
    if (b == '\n') {
      if (!this->rx_buffer_.empty()) {
        this->handle_line_(this->rx_buffer_);
        this->rx_buffer_.clear();
      }
    } else if (b >= 0x20) {
      if (this->rx_buffer_.length() < 512) {
        this->rx_buffer_ += static_cast<char>(b);
      } else {
        this->rx_buffer_.clear();
      }
    }
  }
}

void LoRaWANBridgeComponent::handle_line_(const std::string &line) {
  ESP_LOGD(TAG, "RX: %s", line.c_str());

  // {"status":"ready"}
  if (line.find("\"status\":\"ready\"") != std::string::npos) {
    if (this->state_ == State::WAITING_READY || this->state_ == State::POWERING_ON) {
      ESP_LOGI(TAG, "LoRaWAN bridge ready");
      this->state_ = State::READY;
      this->on_ready_callbacks_.call();
    }
    return;
  }

  // {"ack":true} or {"ack":true,"dl":"..."} or {"ack":false}
  if (line.find("\"ack\":") != std::string::npos) {
    bool success = line.find("\"ack\":true") != std::string::npos;

    if (this->state_ == State::SENDING) {
      this->state_ = State::READY;

      if (success) {
        // Check for downlink data
        auto dl_pos = line.find("\"dl\":\"");
        if (dl_pos != std::string::npos) {
          dl_pos += 6;  // skip past "dl":"
          auto dl_end = line.rfind('"');
          if (dl_end != std::string::npos && dl_end > dl_pos) {
            std::string dl_escaped = line.substr(dl_pos, dl_end - dl_pos);
            std::string dl = this->unescape_json_string_(dl_escaped);
            ESP_LOGI(TAG, "Downlink: %s", dl.c_str());
            this->on_message_callbacks_.call(std::move(dl));
          }
        }
        this->on_send_success_callbacks_.call();
      } else {
        this->on_send_failed_callbacks_.call();
      }
    }
    return;
  }
}

std::string LoRaWANBridgeComponent::unescape_json_string_(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      char next = s[i + 1];
      if (next == '"' || next == '\\' || next == '/') {
        out += next;
        i++;
      } else if (next == 'n') {
        out += '\n';
        i++;
      } else {
        out += s[i];
      }
    } else {
      out += s[i];
    }
  }
  return out;
}

}  // namespace lorawan_bridge
}  // namespace esphome
