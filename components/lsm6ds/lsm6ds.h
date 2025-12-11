#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace lsm6ds {

enum AccelRange {
  ACCEL_RANGE_2G,
  ACCEL_RANGE_4G,
  ACCEL_RANGE_8G,
  ACCEL_RANGE_16G,
};

enum GyroRange {
  GYRO_RANGE_125DPS,
  GYRO_RANGE_250DPS,
  GYRO_RANGE_500DPS,
  GYRO_RANGE_1000DPS,
  GYRO_RANGE_2000DPS,
};

class LSM6DSComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void update() override;

  void set_accel_x_sensor(sensor::Sensor *s) { accel_x_sensor_ = s; }
  void set_accel_y_sensor(sensor::Sensor *s) { accel_y_sensor_ = s; }
  void set_accel_z_sensor(sensor::Sensor *s) { accel_z_sensor_ = s; }

  void set_gyro_x_sensor(sensor::Sensor *s) { gyro_x_sensor_ = s; }
  void set_gyro_y_sensor(sensor::Sensor *s) { gyro_y_sensor_ = s; }
  void set_gyro_z_sensor(sensor::Sensor *s) { gyro_z_sensor_ = s; }
  void set_accel_range(AccelRange range) { accel_range_ = range; }
  void set_gyro_range(GyroRange range) { gyro_range_ = range; }
  void set_shake_sensor(binary_sensor::BinarySensor *s) { shake_binary_sensor_ = s; }
  void set_shake_threshold(float threshold) { shake_threshold_ms2_ = threshold; }
  void set_shake_latch_ms(uint32_t latch_ms) { shake_latch_ms_ = latch_ms; }

 protected:
  bool read_raw_(int16_t &gx, int16_t &gy, int16_t &gz, int16_t &ax, int16_t &ay, int16_t &az);
  void handle_shake_(float ax_ms2, float ay_ms2, float az_ms2);

  sensor::Sensor *accel_x_sensor_{nullptr};
  sensor::Sensor *accel_y_sensor_{nullptr};
  sensor::Sensor *accel_z_sensor_{nullptr};
  sensor::Sensor *gyro_x_sensor_{nullptr};
  sensor::Sensor *gyro_y_sensor_{nullptr};
  sensor::Sensor *gyro_z_sensor_{nullptr};
  binary_sensor::BinarySensor *shake_binary_sensor_{nullptr};

  AccelRange accel_range_{ACCEL_RANGE_2G};
  GyroRange gyro_range_{GYRO_RANGE_250DPS};
  float accel_sensitivity_{0.000598f};  // m/s^2 per LSB @ 2g
  float gyro_sensitivity_{0.00875f};    // dps per LSB @ 250 dps
  float shake_threshold_ms2_{8.0f};
  uint32_t shake_latch_ms_{500};
  bool shake_state_{false};
  uint32_t last_shake_ms_{0};
  float prev_ax_ms2_{0};
  float prev_ay_ms2_{0};
  float prev_az_ms2_{0};
  float prev_accel_mag_{0};
  bool has_prev_accel_{false};
};

}  // namespace lsm6ds
}  // namespace esphome
