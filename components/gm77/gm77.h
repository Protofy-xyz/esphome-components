#ifndef ESPHOME_COMPONENTS_GM77_GM77_H
#define ESPHOME_COMPONENTS_GM77_GM77_H

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace gm77 {

class GM77Component : public uart::UARTDevice, public Component {
 public:
  GM77Component();

  void setup() override;
  void loop() override;

 private:
  static const char *const TAG;
  void process_data(const std::string &data);
};

}  // namespace gm77
}  // namespace esphome

#endif  // ESPHOME_COMPONENTS_GM77_GM77_H