#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>

namespace esphome {
namespace bg95 {

class BG95Component : public Component {
 public:
  void set_tx_pin(GPIOPin *tx_pin) { tx_pin_ = tx_pin; }
  void set_rx_pin(GPIOPin *rx_pin) { rx_pin_ = rx_pin; }
  void set_enable_pin(GPIOPin *enable_pin) { enable_pin_ = enable_pin; }
  void set_on_pin(GPIOPin *on_pin) { on_pin_ = on_pin; }
  void initialize_modem();
  void modem_turn_on();
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
 protected:
  GPIOPin *tx_pin_;
  GPIOPin *rx_pin_;
  GPIOPin *enable_pin_;
  GPIOPin *on_pin_;

};

}  // namespace bg95
}  // namespace esphome