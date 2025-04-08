#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace pca9632 {

class PCA9632Component : public Component, public i2c::I2CDevice {
 public:
  PCA9632Component() = default;

  void setup() override;
  void dump_config() override;

  void set_pwm(uint8_t channel, uint8_t value);
  void set_current(uint8_t value);
  void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
  void set_group_control_mode(bool use_blinking);
  void set_blinking(uint16_t period_ms, uint8_t duty_cycle);
  
 protected:
  void write_register(uint8_t reg, uint8_t value);
};

// ACTIONS
// SetPWMAction
template<typename... Ts>
class SetPWMAction : public Action<Ts...>, public Parented<PCA9632Component> {
 public:
  TEMPLATABLE_VALUE(uint8_t, channel)
  TEMPLATABLE_VALUE(uint8_t, value)

  void play(Ts... x) override {
    this->parent_->set_pwm(this->channel_.value(x...), this->value_.value(x...));
  }
};

// SetCurrentAction
template<typename... Ts>
class SetCurrentAction : public Action<Ts...>, public Parented<PCA9632Component> {
 public:
  TEMPLATABLE_VALUE(uint8_t, value)

  void play(Ts... x) override {
    this->parent_->set_current(this->value_.value(x...));
  }
};

// SetColorAction
template<typename... Ts>
class SetColorAction : public Action<Ts...>, public Parented<PCA9632Component> {
 public:
  TEMPLATABLE_VALUE(uint8_t, r)
  TEMPLATABLE_VALUE(uint8_t, g)
  TEMPLATABLE_VALUE(uint8_t, b)
  TEMPLATABLE_VALUE(uint8_t, w)

  void play(Ts... x) override {
    this->parent_->set_color(this->r_.value(x...), this->g_.value(x...), this->b_.value(x...), this->w_.value(x...));
  }
};

// SetGroupModeAction
template<typename... Ts>
class SetGroupModeAction : public Action<Ts...>, public Parented<PCA9632Component> {
 public:
  TEMPLATABLE_VALUE(bool, mode)

  void play(Ts... x) override {
    this->parent_->set_group_control_mode(this->mode_.value(x...));
  }
};

// SetBlinkingAction
template<typename... Ts>
class SetBlinkingAction : public Action<Ts...>, public Parented<PCA9632Component> {
 public:
  TEMPLATABLE_VALUE(uint32_t, period)
  TEMPLATABLE_VALUE(uint8_t, duty)

  void play(Ts... x) override {
    this->parent_->set_blinking(this->period_.value(x...), this->duty_.value(x...));
  }
};

}  // namespace pca9632
}  // namespace esphome
