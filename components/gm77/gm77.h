#ifndef GM77_H
#define GM77_H

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace gm77 {

class GM77Component : public Component, public uart::UARTDevice {
 public:
  GM77Component();

  void setup() override;
  void loop() override;

  void enable_continuous_scan();
  void disable_continuous_scan();
  void start_decode();
  void stop_decode();

 protected:
  void send_command(const uint8_t *command, size_t length);
};

}  // namespace gm77
}  // namespace esphome

#endif  // GM77_H