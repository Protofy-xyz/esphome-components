#ifndef ESPHOME_COMPONENTS_GM77_GM77_H
#define ESPHOME_COMPONENTS_GM77_GM77_H

#pragma once

#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome.h"
#include <ArduinoJson.h>

namespace esphome {
namespace gm77 {

class GM77Component : public uart::UARTDevice, public Component {
 public:
  GM77Component();

  void setup() override;
  void loop() override;

 private:
  static const char *const TAG;

  bool available();
  char read();
};

}  // namespace gm77
}  // namespace esphome

#endif  // ESPHOME_COMPONENTS_GM77_GM77_H