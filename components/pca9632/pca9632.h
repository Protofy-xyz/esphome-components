#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pca9632 {

class PCA9632Component : public Component, public i2c::I2CDevice {
 public:
  PCA9632Component() = default;

  void setup() override;
  void dump_config() override;

  void set_pwm(uint8_t channel, uint8_t value);
  void set_current(uint8_t value);
  void set_color(uint8_t r, uint8_t g, uint8_t b);
  void set_group_control_mode(bool use_blinking);
  void set_blinking(uint16_t period_ms, uint8_t duty_cycle);
  
 protected:
  void write_register(uint8_t reg, uint8_t value);
};

}  // namespace pca9632
}  // namespace esphome
