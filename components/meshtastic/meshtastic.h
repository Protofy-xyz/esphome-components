#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#include <string>
#include <cstring>

namespace esphome {
namespace meshtastic {

enum class State : uint8_t {
  OFF,
  POWERING_ON,
  INITIALIZING,
  CONFIGURING,
  READY,
  SENDING,
};

class MeshtasticComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  // Configuration setters
  void set_power_pin(GPIOPin *pin) { power_pin_ = pin; }
  void set_boot_timeout(uint32_t ms) { boot_timeout_ = ms; }
  void set_ack_timeout(uint32_t ms) { ack_timeout_ = ms; }
  void set_default_destination(uint32_t dest) { default_destination_ = dest; }
  void set_default_channel(uint8_t ch) { default_channel_ = ch; }
  void set_enable_on_boot(bool en) { enable_on_boot_ = en; }
#ifdef USE_BINARY_SENSOR
  void set_ready_sensor(binary_sensor::BinarySensor *s) { ready_sensor_ = s; }
#endif
#ifdef USE_TEXT_SENSOR
  void set_state_sensor(text_sensor::TextSensor *s) { state_sensor_ = s; }
#endif

  // Public API
  void power_on();
  void power_off();
  bool send_text(const std::string &message, uint32_t destination, uint8_t channel);

  // Getters
  State get_state() const { return state_; }
  bool is_ready() const { return state_ == State::READY; }
  uint32_t get_my_node_num() const { return my_node_num_; }
  uint32_t get_last_from_node() const { return last_from_node_; }
  uint8_t get_last_channel() const { return last_channel_; }
  uint32_t get_default_destination() const { return default_destination_; }
  uint8_t get_default_channel() const { return default_channel_; }

  // Callback registration
  void add_on_ready_callback(std::function<void()> &&cb) { on_ready_callbacks_.add(std::move(cb)); }
  void add_on_message_callback(std::function<void(std::string)> &&cb) { on_message_callbacks_.add(std::move(cb)); }
  void add_on_send_success_callback(std::function<void()> &&cb) { on_send_success_callbacks_.add(std::move(cb)); }
  void add_on_send_failed_callback(std::function<void()> &&cb) { on_send_failed_callbacks_.add(std::move(cb)); }

 protected:
  void send_wake_bytes_();
  void send_want_config_();
  void process_serial_();
  void handle_from_radio_(const uint8_t *buf, size_t len);
  void handle_mesh_packet_(const uint8_t *buf, size_t len);
  void send_framed_(const uint8_t *data, size_t len);
  uint32_t generate_packet_id_();
  void publish_state_();

  // Protobuf encoding helpers
  size_t encode_varint_(uint8_t *buf, uint32_t value);
  size_t encode_field_varint_(uint8_t *buf, uint32_t field_num, uint32_t value);
  size_t encode_field_fixed32_(uint8_t *buf, uint32_t field_num, uint32_t value);
  size_t encode_field_bytes_(uint8_t *buf, uint32_t field_num, const uint8_t *data, size_t data_len);

  // Protobuf decoding helpers
  uint32_t decode_varint_(const uint8_t *buf, size_t *pos, size_t len);
  uint32_t decode_fixed32_(const uint8_t *buf, size_t *pos);
  bool skip_field_(const uint8_t *buf, size_t *pos, size_t len, uint8_t wire_type);

  GPIOPin *power_pin_{nullptr};
  uint32_t boot_timeout_{30000};
  uint32_t ack_timeout_{30000};
  uint32_t default_destination_{0xFFFFFFFF};
  uint8_t default_channel_{0};
  bool enable_on_boot_{true};

  State state_{State::OFF};
  uint32_t state_start_{0};
  uint32_t boot_start_{0};
  uint32_t config_nonce_{0};
  uint32_t my_node_num_{0};
  uint32_t pending_packet_id_{0};
  uint32_t pending_destination_{0};
  uint32_t last_from_node_{0};
  uint8_t last_channel_{0};
  uint16_t packet_id_counter_{0};

  // Serial receive state machine
  static const uint16_t MAX_PAYLOAD = 512;
  uint8_t rx_buf_[MAX_PAYLOAD];
  size_t rx_pos_{0};
  enum RxState : uint8_t {
    RX_WAIT_START1,
    RX_WAIT_START2,
    RX_WAIT_LEN_MSB,
    RX_WAIT_LEN_LSB,
    RX_WAIT_PAYLOAD,
  } rx_state_{RX_WAIT_START1};
  uint16_t rx_payload_len_{0};

  State last_published_state_{State::OFF};

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *ready_sensor_{nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *state_sensor_{nullptr};
#endif

  CallbackManager<void()> on_ready_callbacks_;
  CallbackManager<void(std::string)> on_message_callbacks_;
  CallbackManager<void()> on_send_success_callbacks_;
  CallbackManager<void()> on_send_failed_callbacks_;
};

// --- Triggers ---

class ReadyTrigger : public Trigger<> {
 public:
  explicit ReadyTrigger(MeshtasticComponent *parent) {
    parent->add_on_ready_callback([this]() { this->trigger(); });
  }
};

class MessageTrigger : public Trigger<std::string> {
 public:
  explicit MessageTrigger(MeshtasticComponent *parent) {
    parent->add_on_message_callback([this](const std::string &msg) { this->trigger(msg); });
  }
};

class SendSuccessTrigger : public Trigger<> {
 public:
  explicit SendSuccessTrigger(MeshtasticComponent *parent) {
    parent->add_on_send_success_callback([this]() { this->trigger(); });
  }
};

class SendFailedTrigger : public Trigger<> {
 public:
  explicit SendFailedTrigger(MeshtasticComponent *parent) {
    parent->add_on_send_failed_callback([this]() { this->trigger(); });
  }
};

// --- Actions ---

template<typename... Ts> class SendTextAction : public Action<Ts...> {
 public:
  explicit SendTextAction(MeshtasticComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(std::string, message)
  TEMPLATABLE_VALUE(uint32_t, destination)
  TEMPLATABLE_VALUE(uint8_t, channel)

  void play(Ts... x) override {
    auto msg = this->message_.value(x...);
    uint32_t dest = this->destination_.has_value() ? this->destination_.value(x...) : parent_->get_default_destination();
    uint8_t ch = this->channel_.has_value() ? this->channel_.value(x...) : parent_->get_default_channel();
    parent_->send_text(msg, dest, ch);
  }

 protected:
  MeshtasticComponent *parent_;
};

template<typename... Ts> class PowerOnAction : public Action<Ts...> {
 public:
  explicit PowerOnAction(MeshtasticComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { parent_->power_on(); }

 protected:
  MeshtasticComponent *parent_;
};

template<typename... Ts> class PowerOffAction : public Action<Ts...> {
 public:
  explicit PowerOffAction(MeshtasticComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { parent_->power_off(); }

 protected:
  MeshtasticComponent *parent_;
};

}  // namespace meshtastic
}  // namespace esphome
