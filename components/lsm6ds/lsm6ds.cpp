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

  // Configure accelerometer: 104 Hz ODR, range selected below.
  uint8_t ctrl1_xl = 0x40;  // ODR = 104 Hz
  switch (this->accel_range_) {
    case ACCEL_RANGE_2G:
      ctrl1_xl |= 0x00;
      this->accel_sensitivity_ = 0.000598f;  // 0.061 mg/LSB
      break;
    case ACCEL_RANGE_4G:
      ctrl1_xl |= 0x08;
      this->accel_sensitivity_ = 0.001196f;  // 0.122 mg/LSB
      break;
    case ACCEL_RANGE_8G:
      ctrl1_xl |= 0x0C;
      this->accel_sensitivity_ = 0.002392f;  // 0.244 mg/LSB
      break;
    case ACCEL_RANGE_16G:
      ctrl1_xl |= 0x04;
      this->accel_sensitivity_ = 0.004784f;  // 0.488 mg/LSB
      break;
  }
  if (!this->write_byte(LSM6DS_REG_CTRL1_XL, ctrl1_xl)) {
    ESP_LOGE(TAG, "Failed to configure CTRL1_XL.");
    this->mark_failed();
    return;
  }

  // Configure gyroscope: 104 Hz ODR, range selected below.
  uint8_t ctrl2_g = 0x40;  // ODR = 104 Hz
  switch (this->gyro_range_) {
    case GYRO_RANGE_125DPS:
      ctrl2_g |= 0x02;  // FS_125 bit
      this->gyro_sensitivity_ = 0.004375f;
      break;
    case GYRO_RANGE_250DPS:
      this->gyro_sensitivity_ = 0.00875f;
      break;
    case GYRO_RANGE_500DPS:
      ctrl2_g |= 0x10;
      this->gyro_sensitivity_ = 0.0175f;
      break;
    case GYRO_RANGE_1000DPS:
      ctrl2_g |= 0x20;
      this->gyro_sensitivity_ = 0.035f;
      break;
    case GYRO_RANGE_2000DPS:
      ctrl2_g |= 0x30;
      this->gyro_sensitivity_ = 0.07f;
      break;
  }
  if (!this->write_byte(LSM6DS_REG_CTRL2_G, ctrl2_g)) {
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

  const float gx_dps = gx * this->gyro_sensitivity_;
  const float gy_dps = gy * this->gyro_sensitivity_;
  const float gz_dps = gz * this->gyro_sensitivity_;

  const float ax_ms2 = ax * this->accel_sensitivity_;
  const float ay_ms2 = ay * this->accel_sensitivity_;
  const float az_ms2 = az * this->accel_sensitivity_;

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
