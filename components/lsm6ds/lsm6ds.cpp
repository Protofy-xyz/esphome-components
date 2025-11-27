#include "lsm6ds.h"
#include "esphome/core/log.h"

namespace esphome {
namespace lsm6ds {

static const char *const TAG = "lsm6ds";

static const uint8_t LSM6DS_REG_WHO_AM_I = 0x0F;
static const uint8_t LSM6DS_REG_CTRL1_XL = 0x10;
static const uint8_t LSM6DS_REG_CTRL2_G = 0x11;
static const uint8_t LSM6DS_REG_CTRL3_C = 0x12;
static const uint8_t LSM6DS_REG_OUTX_L_G = 0x22;  // auto-increment to accel after gyro
static const uint8_t LSM6DS_WHO_AM_I_DSL33 = 0x6A;
static const uint8_t LSM6DS_WHO_AM_I_DSOX = 0x6C;
static const uint8_t LSM6DS_WHO_AM_I_DS3  = 0x69;

static const float LSM6DS_SENSITIVITY_GYRO_DPS = 0.00875f;      // 245 dps FS
static const float LSM6DS_SENSITIVITY_ACCEL_MS2 = 0.000598f;    // 2g FS -> m/s^2 per LSB

void LSM6DSComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LSM6DS...");

  uint8_t who_am_i = 0;
  if (!this->read_byte(LSM6DS_REG_WHO_AM_I, &who_am_i) ||
      (who_am_i != LSM6DS_WHO_AM_I_DSL33 && who_am_i != LSM6DS_WHO_AM_I_DSOX && who_am_i != LSM6DS_WHO_AM_I_DS3)) {
    ESP_LOGE(TAG, "LSM6DS not found or unexpected chip (WHO_AM_I=0x%02X). Expected one of 0x%02X/0x%02X/0x%02X.", who_am_i,
             LSM6DS_WHO_AM_I_DSL33, LSM6DS_WHO_AM_I_DSOX, LSM6DS_WHO_AM_I_DS3);
    this->mark_failed();
    return;
  }

  // Configure accelerometer: 104 Hz ODR, 2g full scale.
  if (!this->write_byte(LSM6DS_REG_CTRL1_XL, 0x40)) {
    ESP_LOGE(TAG, "Failed to configure CTRL1_XL.");
    this->mark_failed();
    return;
  }
  // Configure gyroscope: 104 Hz ODR, 245 dps full scale.
  if (!this->write_byte(LSM6DS_REG_CTRL2_G, 0x40)) {
    ESP_LOGE(TAG, "Failed to configure CTRL2_G.");
    this->mark_failed();
    return;
  }
  // Block data update + register auto-increment.
  if (!this->write_byte(LSM6DS_REG_CTRL3_C, 0x44)) {
    ESP_LOGE(TAG, "Failed to configure CTRL3_C.");
    this->mark_failed();
    return;
  }
}

void LSM6DSComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LSM6DS:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Accel X", this->accel_x_sensor_);
  LOG_SENSOR("  ", "Accel Y", this->accel_y_sensor_);
  LOG_SENSOR("  ", "Accel Z", this->accel_z_sensor_);
  LOG_SENSOR("  ", "Gyro X", this->gyro_x_sensor_);
  LOG_SENSOR("  ", "Gyro Y", this->gyro_y_sensor_);
  LOG_SENSOR("  ", "Gyro Z", this->gyro_z_sensor_);
}

void LSM6DSComponent::update() {
  int16_t gx, gy, gz, ax, ay, az;
  if (!this->read_raw_(gx, gy, gz, ax, ay, az)) {
    this->status_set_warning();
    return;
  }

  const float gx_dps = gx * LSM6DS_SENSITIVITY_GYRO_DPS;
  const float gy_dps = gy * LSM6DS_SENSITIVITY_GYRO_DPS;
  const float gz_dps = gz * LSM6DS_SENSITIVITY_GYRO_DPS;

  const float ax_ms2 = ax * LSM6DS_SENSITIVITY_ACCEL_MS2;
  const float ay_ms2 = ay * LSM6DS_SENSITIVITY_ACCEL_MS2;
  const float az_ms2 = az * LSM6DS_SENSITIVITY_ACCEL_MS2;

  if (this->gyro_x_sensor_ != nullptr)
    this->gyro_x_sensor_->publish_state(gx_dps);
  if (this->gyro_y_sensor_ != nullptr)
    this->gyro_y_sensor_->publish_state(gy_dps);
  if (this->gyro_z_sensor_ != nullptr)
    this->gyro_z_sensor_->publish_state(gz_dps);

  if (this->accel_x_sensor_ != nullptr)
    this->accel_x_sensor_->publish_state(ax_ms2);
  if (this->accel_y_sensor_ != nullptr)
    this->accel_y_sensor_->publish_state(ay_ms2);
  if (this->accel_z_sensor_ != nullptr)
    this->accel_z_sensor_->publish_state(az_ms2);

  //ESP_LOGD(TAG, "G[%.2f, %.2f, %.2f] dps  A[%.3f, %.3f, %.3f] m/s^2", gx_dps, gy_dps, gz_dps, ax_ms2, ay_ms2, az_ms2);
  this->status_clear_warning();
}

bool LSM6DSComponent::read_raw_(int16_t &gx, int16_t &gy, int16_t &gz, int16_t &ax, int16_t &ay, int16_t &az) {
  uint8_t buffer[12];
  if (!this->read_bytes(LSM6DS_REG_OUTX_L_G, buffer, sizeof(buffer))) {
    ESP_LOGW(TAG, "Failed to read data registers.");
    return false;
  }

  gx = static_cast<int16_t>((buffer[1] << 8) | buffer[0]);
  gy = static_cast<int16_t>((buffer[3] << 8) | buffer[2]);
  gz = static_cast<int16_t>((buffer[5] << 8) | buffer[4]);

  ax = static_cast<int16_t>((buffer[7] << 8) | buffer[6]);
  ay = static_cast<int16_t>((buffer[9] << 8) | buffer[8]);
  az = static_cast<int16_t>((buffer[11] << 8) | buffer[10]);

  return true;
}

}  // namespace lsm6ds
}  // namespace esphome
