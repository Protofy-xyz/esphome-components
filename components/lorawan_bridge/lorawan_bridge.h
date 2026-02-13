#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include <string>

namespace esphome {
namespace lorawan_bridge {

// Orchestrator-side component that talks to a XIAO LoRaWAN bridge via UART.
// Provides the same API as the meshtastic component (power_on, send_text,
// on_ready, on_message, on_send_success, on_send_failed).
//
// UART protocol (115200, newline-delimited JSON):
//   RX (from XIAO):
//     {"status":"ready"}          — XIAO radio initialized
//     {"ack":true}                — uplink sent, no downlink
//     {"ack":true,"dl":"<data>"}  — uplink sent, downlink received
//     {"ack":false}               — uplink failed
//   TX (to XIAO):
//     <payload>\n                 — raw text to send as LoRaWAN uplink

enum class State : uint8_t {
  OFF,
  POWERING_ON,
  WAITING_READY,
  READY,
  SENDING,
};

class LoRaWANBridgeComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return 250.0f; }

  // Configuration setters
  void set_power_pin(GPIOPin *pin) { power_pin_ = pin; }
  void set_power_on_delay(uint32_t ms) { power_on_delay_ = ms; }
  void set_boot_timeout(uint32_t ms) { boot_timeout_ = ms; }
  void set_ack_timeout(uint32_t ms) { ack_timeout_ = ms; }
  void set_enable_on_boot(bool en) { enable_on_boot_ = en; }

  // Public API (matches meshtastic component interface)
  void power_on();
  void power_off();
  bool send_text(const std::string &message);
  bool is_ready() const { return state_ == State::READY; }
  State get_state() const { return state_; }

  // Callback registration
  void add_on_ready_callback(std::function<void()> &&cb) { on_ready_callbacks_.add(std::move(cb)); }
  void add_on_message_callback(std::function<void(std::string)> &&cb) { on_message_callbacks_.add(std::move(cb)); }
  void add_on_send_success_callback(std::function<void()> &&cb) { on_send_success_callbacks_.add(std::move(cb)); }
  void add_on_send_failed_callback(std::function<void()> &&cb) { on_send_failed_callbacks_.add(std::move(cb)); }

 protected:
  void process_serial_();
  void handle_line_(const std::string &line);
  std::string unescape_json_string_(const std::string &s);

  GPIOPin *power_pin_{nullptr};
  uint32_t power_on_delay_{2000};
  uint32_t boot_timeout_{15000};
  uint32_t ack_timeout_{10000};
  bool enable_on_boot_{true};

  State state_{State::OFF};
  uint32_t state_start_{0};

  // UART line buffer
  std::string rx_buffer_;

  CallbackManager<void()> on_ready_callbacks_;
  CallbackManager<void(std::string)> on_message_callbacks_;
  CallbackManager<void()> on_send_success_callbacks_;
  CallbackManager<void()> on_send_failed_callbacks_;
};

// --- Triggers ---

class ReadyTrigger : public Trigger<> {
 public:
  explicit ReadyTrigger(LoRaWANBridgeComponent *parent) {
    parent->add_on_ready_callback([this]() { this->trigger(); });
  }
};

class MessageTrigger : public Trigger<std::string> {
 public:
  explicit MessageTrigger(LoRaWANBridgeComponent *parent) {
    parent->add_on_message_callback([this](const std::string &msg) { this->trigger(msg); });
  }
};

class SendSuccessTrigger : public Trigger<> {
 public:
  explicit SendSuccessTrigger(LoRaWANBridgeComponent *parent) {
    parent->add_on_send_success_callback([this]() { this->trigger(); });
  }
};

class SendFailedTrigger : public Trigger<> {
 public:
  explicit SendFailedTrigger(LoRaWANBridgeComponent *parent) {
    parent->add_on_send_failed_callback([this]() { this->trigger(); });
  }
};

// --- Actions ---

template<typename... Ts> class SendTextAction : public Action<Ts...> {
 public:
  explicit SendTextAction(LoRaWANBridgeComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(std::string, message)

  void play(Ts... x) override {
    auto msg = this->message_.value(x...);
    parent_->send_text(msg);
  }

 protected:
  LoRaWANBridgeComponent *parent_;
};

template<typename... Ts> class PowerOnAction : public Action<Ts...> {
 public:
  explicit PowerOnAction(LoRaWANBridgeComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { parent_->power_on(); }

 protected:
  LoRaWANBridgeComponent *parent_;
};

template<typename... Ts> class PowerOffAction : public Action<Ts...> {
 public:
  explicit PowerOffAction(LoRaWANBridgeComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { parent_->power_off(); }

 protected:
  LoRaWANBridgeComponent *parent_;
};

}  // namespace lorawan_bridge
}  // namespace esphome
