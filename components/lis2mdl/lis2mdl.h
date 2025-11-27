#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace lis2mdl {

class LIS2MDLComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void update() override;

  void set_x_sensor(sensor::Sensor *sensor) { x_sensor_ = sensor; }
  void set_y_sensor(sensor::Sensor *sensor) { y_sensor_ = sensor; }
  void set_z_sensor(sensor::Sensor *sensor) { z_sensor_ = sensor; }

 protected:
  bool read_axes_(int16_t &x, int16_t &y, int16_t &z);

  sensor::Sensor *x_sensor_{nullptr};
  sensor::Sensor *y_sensor_{nullptr};
  sensor::Sensor *z_sensor_{nullptr};
};

}  // namespace lis2mdl
}  // namespace esphome
