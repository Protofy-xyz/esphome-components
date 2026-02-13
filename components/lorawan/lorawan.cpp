#include "lorawan.h"
#include "esphome/core/log.h"
#include <SPI.h>

namespace esphome {
namespace lorawan {

static const char *const TAG = "lorawan";

// ---- Lifecycle ----

void LoRaWANComponent::setup() {
  ESP_LOGI(TAG, "Initializing SX1262 + LoRaWAN...");

  // Configure SPI with custom pins (uses default FSPI bus on ESP32-S3)
  SPI.begin(this->sck_pin_, this->miso_pin_, this->mosi_pin_);

  // Create RadioLib module + radio
  // The 6-arg Module constructor uses our SPI instance
  this->radio_ = new SX1262(new Module(
      this->cs_pin_, this->dio1_pin_, this->reset_pin_, this->busy_pin_,
      SPI, SPISettings(2000000, MSBFIRST, SPI_MODE0)));

  int16_t state = this->radio_->begin();
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "SX1262 init failed, code: %d", state);
    this->mark_failed();
    return;
  }

  // Configure TCXO voltage via DIO3 (must be set before any RF operation)
  if (this->tcxo_voltage_ > 0.0f) {
    state = this->radio_->setTCXO(this->tcxo_voltage_);
    if (state != RADIOLIB_ERR_NONE) {
      ESP_LOGE(TAG, "setTCXO(%.1f) failed: %d", this->tcxo_voltage_, state);
      this->mark_failed();
      return;
    }
    ESP_LOGI(TAG, "TCXO voltage set to %.1fV via DIO3", this->tcxo_voltage_);
  }

  // Configure DIO2 as RF switch control
  if (this->dio2_as_rf_switch_) {
    state = this->radio_->setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE) {
      ESP_LOGE(TAG, "setDio2AsRfSwitch failed: %d", state);
      this->mark_failed();
      return;
    }
    ESP_LOGI(TAG, "DIO2 configured as RF switch");
  }

  // Set TX power
  state = this->radio_->setOutputPower(this->tx_power_);
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGW(TAG, "setOutputPower(%d) failed: %d, continuing with default", this->tx_power_, state);
  }

  // Parse LoRaWAN ABP credentials
  uint32_t dev_addr = parse_hex_u32_(this->dev_addr_str_);
  uint8_t nwk_key[16], app_key[16];
  if (!parse_hex_key_(this->nwk_s_key_str_, nwk_key, 16)) {
    ESP_LOGE(TAG, "Invalid nwk_s_key (need 32 hex chars)");
    this->mark_failed();
    return;
  }
  if (!parse_hex_key_(this->app_s_key_str_, app_key, 16)) {
    ESP_LOGE(TAG, "Invalid app_s_key (need 32 hex chars)");
    this->mark_failed();
    return;
  }

  // Select LoRaWAN band
  const LoRaWANBand_t *band = &EU868;
  if (this->band_ == "US915") band = &US915;
  else if (this->band_ == "AU915") band = &AU915;
  else if (this->band_ == "AS923") band = &AS923;
  else if (this->band_ == "IN865") band = &IN865;
  else if (this->band_ == "KR920") band = &KR920;

  // Create LoRaWAN node
  this->node_ = new LoRaWANNode(this->radio_, band);

  // RadioLib v7 requires nonces + session buffers before begin
  memset(this->nonces_buf_, 0, sizeof(this->nonces_buf_));
  memset(this->session_buf_, 0, sizeof(this->session_buf_));
  this->node_->setBufferNonces(this->nonces_buf_);
  this->node_->setBufferSession(this->session_buf_);

  // ABP activation (LoRaWAN 1.0.x: pass NULL for fNwkSIntKey and sNwkSIntKey
  // so RadioLib uses 1.0 MIC format, not 1.1 composite MIC)
  // NOTE: Frame counters start at 0 on each boot.
  //       Disable frame counter validation in ChirpStack device profile
  //       or persist counters in NVS for production use.
  state = this->node_->beginABP(dev_addr, NULL, NULL, nwk_key, app_key);
  if (state != RADIOLIB_ERR_NONE) {
    ESP_LOGE(TAG, "LoRaWAN beginABP failed, code: %d", state);
    this->mark_failed();
    return;
  }

  // activateABP() actually creates the session — without this, sendReceive
  // fails with -1101 (RADIOLIB_ERR_NETWORK_NOT_JOINED)
  state = this->node_->activateABP();
  if (state != RADIOLIB_ERR_NONE && state != RADIOLIB_LORAWAN_NEW_SESSION) {
    ESP_LOGE(TAG, "LoRaWAN activateABP failed, code: %d", state);
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "ABP session %s", state == RADIOLIB_LORAWAN_NEW_SESSION ? "created" : "restored");

  this->ready_ = true;
  this->send_uart_line_("{\"status\":\"ready\"}");
  ESP_LOGI(TAG, "LoRaWAN ready (ABP, dev_addr=0x%08X, band=%s, port=%u)",
           dev_addr, this->band_.c_str(), this->port_);
}

void LoRaWANComponent::loop() {
  if (!this->ready_) return;
  this->process_uart_();
}

void LoRaWANComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LoRaWAN (SX1262):");
  ESP_LOGCONFIG(TAG, "  SPI: SCK=%d MOSI=%d MISO=%d CS=%d", sck_pin_, mosi_pin_, miso_pin_, cs_pin_);
  ESP_LOGCONFIG(TAG, "  DIO1=%d BUSY=%d RESET=%d", dio1_pin_, busy_pin_, reset_pin_);
  ESP_LOGCONFIG(TAG, "  Band: %s  TX Power: %ddBm  Port: %u", band_.c_str(), tx_power_, port_);
  ESP_LOGCONFIG(TAG, "  DIO2 RF Switch: %s  TCXO: %.1fV", dio2_as_rf_switch_ ? "yes" : "no", tcxo_voltage_);
  ESP_LOGCONFIG(TAG, "  DevAddr: %s", dev_addr_str_.c_str());
}

// ---- UART processing ----

void LoRaWANComponent::process_uart_() {
  while (this->available()) {
    uint8_t b;
    this->read_byte(&b);
    if (b == '\n') {
      if (!this->uart_buffer_.empty()) {
        this->send_uplink_(this->uart_buffer_);
        this->uart_buffer_.clear();
      }
    } else if (b >= 0x20) {
      // Guard against buffer overflow
      if (this->uart_buffer_.length() < 300) {
        this->uart_buffer_ += static_cast<char>(b);
      }
    }
  }
}

// ---- LoRaWAN uplink ----

void LoRaWANComponent::send_uplink_(const std::string &payload) {
  ESP_LOGI(TAG, "TX uplink (%u bytes, port %u)", payload.size(), this->port_);

  uint8_t downlink[256];
  size_t downlink_len = 0;

  // sendReceive is blocking (~3-4s for TX + RX1 + RX2 windows)
  int16_t state = this->node_->sendReceive(
      const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(payload.c_str())),
      payload.size(),
      this->port_,
      downlink, &downlink_len);

  // RADIOLIB_ERR_NONE = success + downlink received
  // RADIOLIB_ERR_RX_TIMEOUT (-6) = uplink sent, no downlink (normal for Class A)
  if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_ERR_RX_TIMEOUT) {
    if (state == RADIOLIB_ERR_NONE && downlink_len > 0) {
      // Uplink OK + downlink received — escape downlink for JSON string
      std::string dl_escaped;
      dl_escaped.reserve(downlink_len + 16);
      for (size_t i = 0; i < downlink_len; i++) {
        char c = static_cast<char>(downlink[i]);
        if (c == '"') dl_escaped += "\\\"";
        else if (c == '\\') dl_escaped += "\\\\";
        else if (c >= 0x20) dl_escaped += c;
      }
      this->send_uart_line_("{\"ack\":true,\"dl\":\"" + dl_escaped + "\"}");
      ESP_LOGI(TAG, "TX OK, downlink %u bytes", downlink_len);
    } else {
      // Uplink OK, no downlink
      this->send_uart_line_("{\"ack\":true}");
      ESP_LOGI(TAG, "TX OK, no downlink");
    }
  } else {
    this->send_uart_line_("{\"ack\":false}");
    ESP_LOGW(TAG, "TX failed, RadioLib code: %d", state);
  }
}

// ---- Helpers ----

void LoRaWANComponent::send_uart_line_(const std::string &line) {
  this->write_array(reinterpret_cast<const uint8_t *>(line.c_str()), line.size());
  uint8_t nl = '\n';
  this->write_array(&nl, 1);
  this->flush();
}

uint32_t LoRaWANComponent::parse_hex_u32_(const std::string &hex) {
  return static_cast<uint32_t>(strtoul(hex.c_str(), nullptr, 16));
}

bool LoRaWANComponent::parse_hex_key_(const std::string &hex, uint8_t *out, size_t len) {
  if (hex.size() != len * 2) return false;
  for (size_t i = 0; i < len; i++) {
    auto nibble = [](char c) -> uint8_t {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return 0;
    };
    out[i] = (nibble(hex[i * 2]) << 4) | nibble(hex[i * 2 + 1]);
  }
  return true;
}

}  // namespace lorawan
}  // namespace esphome
