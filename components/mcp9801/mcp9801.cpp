#include "mcp9801.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mcp9801 {

static const uint8_t MCP9801_REG_AMBIENT_TEMP = 0x00;

static const char *const TAG = "mcp9801";

void MCP9801Sensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up %s...", this->name_.c_str());

  // El MCP9801 no tiene Manufacturer ID ni Device ID
  // Asumimos que si responde en I2C, está presente
  uint16_t dummy;
  if (!this->read_byte_16(MCP9801_REG_AMBIENT_TEMP, &dummy)) {
    this->mark_failed();
    ESP_LOGE(TAG, "%s not responding at expected I2C address.", this->name_.c_str());
    return;
  }
}

void MCP9801Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "%s:", this->name_.c_str());
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with %s failed!", this->name_.c_str());
  }
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Temperature", this);
}

void MCP9801Sensor::update() {
  uint16_t raw_temp;
  if (!this->read_byte_16(MCP9801_REG_AMBIENT_TEMP, &raw_temp)) {
    this->status_set_warning();
    return;
  }

  float temp = NAN;

  // Swap bytes if needed
  uint8_t msb = raw_temp >> 8;
  uint8_t lsb = raw_temp & 0xFF;

  // Reconstruct raw value
  int16_t value = ((int16_t) raw_temp) >> 4;

  // Sign extend if negative
  if (value & 0x0800) {
    value |= 0xF000;
  }

  temp = value * 0.0625f;

  if (std::isnan(temp)) {
    this->status_set_warning();
    return;
  }

  ESP_LOGD(TAG, "%s: Got temperature=%.4f°C", this->name_.c_str(), temp);
  this->publish_state(temp);
  this->status_clear_warning();
}

float MCP9801Sensor::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace mcp9801
}  // namespace esphome
