#include "sfm4300.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sfm4300 {

static const char *const TAG = "sfm4300";

// I2C Commands (16-bit, sent MSB first)
static const uint16_t CMD_START_O2  = 0x3603;
static const uint16_t CMD_START_AIR = 0x3608;
static const uint16_t CMD_START_N2O = 0x3615;
static const uint16_t CMD_START_CO2 = 0x361E;
static const uint16_t CMD_STOP      = 0x3FF9;
static const uint16_t CMD_READ_SCALE = 0x3661;

// Sensirion CRC8 polynomial: x^8 + x^5 + x^4 + 1 = 0x31
uint8_t SFM4300Sensor::crc8_(const uint8_t *data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x31;
      else
        crc = crc << 1;
    }
  }
  return crc;
}

bool SFM4300Sensor::send_command_(uint16_t cmd) {
  uint8_t buf[2];
  buf[0] = cmd >> 8;
  buf[1] = cmd & 0xFF;
  return this->write(buf, 2) == i2c::ERROR_OK;
}

bool SFM4300Sensor::read_data_(uint8_t *data, size_t len) {
  return this->read(data, len) == i2c::ERROR_OK;
}

uint16_t SFM4300Sensor::get_start_command_() const {
  switch (this->gas_type_) {
    case GAS_O2:  return CMD_START_O2;
    case GAS_AIR: return CMD_START_AIR;
    case GAS_N2O: return CMD_START_N2O;
    case GAS_CO2: return CMD_START_CO2;
    default:      return CMD_START_O2;
  }
}

void SFM4300Sensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SFM4300...");

  uint16_t start_cmd = this->get_start_command_();

  // Step 0: Send stop command in case sensor was left in continuous mode
  // (e.g. after a soft reset). Ignore errors — sensor may not be measuring.
  this->send_command_(CMD_STOP);
  vTaskDelay(pdMS_TO_TICKS(50));

  // Step 1: Read scale factor and offset
  // Send CMD_READ_SCALE (0x3661) followed by the gas start command as argument
  uint8_t cmd_buf[4];
  cmd_buf[0] = CMD_READ_SCALE >> 8;    // 0x36
  cmd_buf[1] = CMD_READ_SCALE & 0xFF;  // 0x61
  cmd_buf[2] = start_cmd >> 8;          // e.g. 0x36
  cmd_buf[3] = start_cmd & 0xFF;        // e.g. 0x03

  if (this->write(cmd_buf, 4) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Failed to send read_scale command");
    this->mark_failed();
    return;
  }

  // Read 9 bytes: scale(2) + crc(1) + offset(2) + crc(1) + unit(2) + crc(1)
  // Retry with increasing delays — sensor may need time to process
  uint8_t data[9];
  bool read_ok = false;
  const uint32_t delays_ms[] = {100, 250, 500, 1000};
  for (int attempt = 0; attempt < 4; attempt++) {
    vTaskDelay(pdMS_TO_TICKS(delays_ms[attempt]));
    if (this->read_data_(data, 9)) {
      read_ok = true;
      ESP_LOGI(TAG, "Scale/offset read OK on attempt %d (delay %lums)", attempt + 1, delays_ms[attempt]);
      break;
    }
    ESP_LOGW(TAG, "Read attempt %d failed (delay %lums), retrying...", attempt + 1, delays_ms[attempt]);
  }

  if (!read_ok) {
    ESP_LOGE(TAG, "Failed to read scale/offset data after all retries");
    this->mark_failed();
    return;
  }

  // Log raw bytes for debugging
  ESP_LOGD(TAG, "Raw scale data: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);

  // Verify CRCs
  if (crc8_(data, 2) != data[2] || crc8_(data + 3, 2) != data[5]) {
    ESP_LOGE(TAG, "CRC check failed on scale/offset data");
    this->mark_failed();
    return;
  }

  this->scale_factor_ = (float)((int16_t)((data[0] << 8) | data[1]));
  this->offset_ = (int16_t)((data[3] << 8) | data[4]);

  if (this->scale_factor_ == 0) {
    ESP_LOGE(TAG, "Invalid scale factor (0)");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Scale factor: %.0f, Offset: %d", this->scale_factor_, this->offset_);

  // Step 2: Start continuous measurement
  if (!this->send_command_(start_cmd)) {
    ESP_LOGE(TAG, "Failed to start continuous measurement");
    this->mark_failed();
    return;
  }

  this->initialized_ = true;
  ESP_LOGI(TAG, "SFM4300 initialized successfully");
}

void SFM4300Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "SFM4300:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with SFM4300 failed!");
  }
  const char *gas_names[] = {"O2", "Air", "N2O", "CO2"};
  ESP_LOGCONFIG(TAG, "  Gas type: %s", gas_names[this->gas_type_]);
  ESP_LOGCONFIG(TAG, "  Scale factor: %.0f", this->scale_factor_);
  ESP_LOGCONFIG(TAG, "  Offset: %d", this->offset_);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Flow", this);
}

void SFM4300Sensor::update() {
  if (!this->initialized_) {
    return;
  }

  // Read 9 bytes: flow(2) + crc(1) + temperature(2) + crc(1) + status(2) + crc(1)
  uint8_t data[9];
  if (!this->read_data_(data, 9)) {
    ESP_LOGW(TAG, "Failed to read measurement data");
    this->status_set_warning();
    return;
  }

  // Verify flow CRC
  if (crc8_(data, 2) != data[2]) {
    ESP_LOGW(TAG, "CRC check failed on flow data");
    this->status_set_warning();
    return;
  }

  int16_t raw_flow = (int16_t)((data[0] << 8) | data[1]);
  float flow = (float)(raw_flow - this->offset_) / this->scale_factor_;

  ESP_LOGD(TAG, "Raw: %d, Flow: %.2f slm", raw_flow, flow);
  this->publish_state(flow);
  this->status_clear_warning();
}

float SFM4300Sensor::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace sfm4300
}  // namespace esphome
