#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <RadioLib.h>
#include <string>

namespace esphome {
namespace lorawan {

// LoRaWAN bridge for XIAO ESP32-S3 + SX1262
//
// Receives payloads on UART from the orchestrator, sends them as
// LoRaWAN Class A uplinks via SX1262, and returns ACK + any downlink.
//
// UART protocol (115200, newline-delimited):
//   TX (to orchestrator):
//     {"status":"ready"}          — radio initialized
//     {"ack":true}                — uplink sent, no downlink
//     {"ack":true,"dl":"<data>"}  — uplink sent, downlink received
//     {"ack":false}               — uplink failed
//   RX (from orchestrator):
//     <payload>\n                 — raw text to send as uplink

class LoRaWANComponent : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  // Configuration setters (called from Python codegen)
  void set_cs_pin(int pin) { cs_pin_ = pin; }
  void set_dio1_pin(int pin) { dio1_pin_ = pin; }
  void set_reset_pin(int pin) { reset_pin_ = pin; }
  void set_busy_pin(int pin) { busy_pin_ = pin; }
  void set_sck_pin(int pin) { sck_pin_ = pin; }
  void set_mosi_pin(int pin) { mosi_pin_ = pin; }
  void set_miso_pin(int pin) { miso_pin_ = pin; }

  void set_dev_addr_str(const std::string &s) { dev_addr_str_ = s; }
  void set_nwk_s_key_str(const std::string &s) { nwk_s_key_str_ = s; }
  void set_app_s_key_str(const std::string &s) { app_s_key_str_ = s; }

  void set_band(const std::string &s) { band_ = s; }
  void set_tx_power(int p) { tx_power_ = p; }
  void set_port(uint8_t p) { port_ = p; }
  void set_dio2_as_rf_switch(bool v) { dio2_as_rf_switch_ = v; }
  void set_tcxo_voltage(float v) { tcxo_voltage_ = v; }

  // Public send (for testing / direct use from lambdas)
  void send(const std::string &payload) { this->send_uplink_(payload); }
  bool is_ready() const { return ready_; }

 protected:
  void process_uart_();
  void send_uplink_(const std::string &payload);
  void send_uart_line_(const std::string &line);

  static bool parse_hex_key_(const std::string &hex, uint8_t *out, size_t len);
  static uint32_t parse_hex_u32_(const std::string &hex);

  // SX1262 SPI pins
  int cs_pin_{-1};
  int dio1_pin_{-1};
  int reset_pin_{-1};
  int busy_pin_{-1};
  int sck_pin_{-1};
  int mosi_pin_{-1};
  int miso_pin_{-1};

  // LoRaWAN ABP credentials (hex strings, parsed in setup)
  std::string dev_addr_str_;
  std::string nwk_s_key_str_;
  std::string app_s_key_str_;

  // Radio parameters
  std::string band_{"EU868"};
  int tx_power_{14};
  uint8_t port_{1};
  bool dio2_as_rf_switch_{false};
  float tcxo_voltage_{0.0f};

  // RadioLib objects (heap-allocated in setup, live forever)
  SX1262 *radio_{nullptr};
  LoRaWANNode *node_{nullptr};

  // LoRaWAN session persistence buffers (required by RadioLib v7)
  uint8_t nonces_buf_[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
  uint8_t session_buf_[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

  // State
  bool ready_{false};
  std::string uart_buffer_;
};

}  // namespace lorawan
}  // namespace esphome
