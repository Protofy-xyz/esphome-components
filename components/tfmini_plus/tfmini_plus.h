#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace tfmini_plus {

class TFMiniPlusComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void set_distance_sensor(sensor::Sensor *s) { this->distance_sensor_ = s; }
  void set_strength_sensor(sensor::Sensor *s) { this->strength_sensor_ = s; }
  void set_temperature_sensor(sensor::Sensor *s) { this->temperature_sensor_ = s; }
  void set_distance_unit_mm(bool mm) { this->distance_unit_mm_ = mm; }
  void set_frame_rate(uint16_t rate) { this->frame_rate_ = rate; this->frame_rate_set_ = true; }

 protected:
  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *strength_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  bool distance_unit_mm_{false};
  uint16_t frame_rate_{0};
  bool frame_rate_set_{false};
};

}  // namespace tfmini_plus
}  // namespace esphome
