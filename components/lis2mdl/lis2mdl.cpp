#include "lis2mdl.h"
#include "esphome/core/log.h"

namespace esphome {
namespace lis2mdl {

static const char *const TAG = "lis2mdl";

static const uint8_t LIS2MDL_REG_WHO_AM_I = 0x4F;
static const uint8_t LIS2MDL_REG_CFG_A = 0x60;
static const uint8_t LIS2MDL_REG_CFG_B = 0x61;
static const uint8_t LIS2MDL_REG_CFG_C = 0x62;
static const uint8_t LIS2MDL_REG_OUTX_L = 0x68;

static const uint8_t LIS2MDL_WHO_AM_I = 0x40;
static const float LIS2MDL_SENSITIVITY_UT = 0.15f;  // microtesla per LSB

void LIS2MDLComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LIS2MDL...");

  uint8_t who_am_i = 0;
  if (!this->read_byte(LIS2MDL_REG_WHO_AM_I, &who_am_i) || who_am_i != LIS2MDL_WHO_AM_I) {
    ESP_LOGE(TAG, "LIS2MDL not found at address 0x%02X (WHO_AM_I=0x%02X).", this->address_, who_am_i);
    this->mark_failed();
    return;
  }

  // Enable temperature compensation, ODR 100 Hz, continuous conversion mode.
  if (!this->write_byte(LIS2MDL_REG_CFG_A, 0x8C)) {
    this->mark_failed();
    ESP_LOGE(TAG, "Failed writing CFG_REG_A.");
    return;
  }
  // Leave filtering/offset cancellation at defaults.
  if (!this->write_byte(LIS2MDL_REG_CFG_B, 0x00)) {
    this->mark_failed();
    ESP_LOGE(TAG, "Failed writing CFG_REG_B.");
    return;
  }
  // Block data update to avoid tearing while reading multiple registers.
  if (!this->write_byte(LIS2MDL_REG_CFG_C, 0x10)) {
    this->mark_failed();
    ESP_LOGE(TAG, "Failed writing CFG_REG_C.");
    return;
  }
}

void LIS2MDLComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LIS2MDL:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Magnetic Field X", this->x_sensor_);
  LOG_SENSOR("  ", "Magnetic Field Y", this->y_sensor_);
  LOG_SENSOR("  ", "Magnetic Field Z", this->z_sensor_);
}

void LIS2MDLComponent::update() {
  int16_t raw_x, raw_y, raw_z;
  if (!this->read_axes_(raw_x, raw_y, raw_z)) {
    this->status_set_warning();
    return;
  }

  const float x = raw_x * LIS2MDL_SENSITIVITY_UT;
  const float y = raw_y * LIS2MDL_SENSITIVITY_UT;
  const float z = raw_z * LIS2MDL_SENSITIVITY_UT;

  if (this->x_sensor_ != nullptr)
    this->x_sensor_->publish_state(x);
  if (this->y_sensor_ != nullptr)
    this->y_sensor_->publish_state(y);
  if (this->z_sensor_ != nullptr)
    this->z_sensor_->publish_state(z);

  //ESP_LOGD(TAG, "X=%.2fuT Y=%.2fuT Z=%.2fuT", x, y, z);
  this->status_clear_warning();
}

bool LIS2MDLComponent::read_axes_(int16_t &x, int16_t &y, int16_t &z) {
  uint8_t buffer[6];
  if (!this->read_bytes(LIS2MDL_REG_OUTX_L, buffer, 6)) {
    ESP_LOGW(TAG, "Failed to read measurement registers.");
    return false;
  }

  x = static_cast<int16_t>((buffer[1] << 8) | buffer[0]);
  y = static_cast<int16_t>((buffer[3] << 8) | buffer[2]);
  z = static_cast<int16_t>((buffer[5] << 8) | buffer[4]);

  return true;
}

}  // namespace lis2mdl
}  // namespace esphome
