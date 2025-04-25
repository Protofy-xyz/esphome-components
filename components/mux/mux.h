#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace mux {

class MUXComponent : public Component {
 public:
  void set_sel_pins(GPIOPin *sel0, GPIOPin *sel1, GPIOPin *sel2);
  void set_result_pin(GPIOPin *result);
  void set_update_interval(uint32_t interval) { update_interval_ = interval; }

  bool digital_read(uint8_t channel);

  float get_setup_priority() const override { return setup_priority::IO; }

 protected:
  GPIOPin *sel0_;
  GPIOPin *sel1_;
  GPIOPin *sel2_;
  GPIOPin *result_;
  uint32_t update_interval_;
};

class MUXGPIOPin : public GPIOPin {
 public:
  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_parent(MUXComponent *parent) { parent_ = parent; }
  void set_channel(uint8_t channel) { channel_ = channel; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }

  gpio::Flags get_flags() const override { return this->flags_; }

 protected:
  MUXComponent *parent_;
  uint8_t channel_;
  bool inverted_;
  gpio::Flags flags_;
};

}  // namespace mux
}  // namespace esphome
