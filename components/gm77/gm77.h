#ifndef GM77_H
#define GM77_H
#define SCAN_ENABLE_COMMAND "SCAN_ENABLE"
#define SCAN_DISABLE_COMMAND "SCAN_DISABLE"
#define CONTINUOUS_MODE_COMMAND "CONTINUOUS_MODE"
#define DECODE_START_COMMAND "DECODE_START"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace gm77 {

class GM77Component : public uart::UARTDevice, public Component {
 public:
  GM77Component();

  void setup() override;
  void loop() override;

  void enable_continuous_scan();
  void disable_scan();
  void send_command(const std::string &command);
  void process_data(const std::string &data);
  void start_decode();

 private:
  static const char *const TAG;
};

}  // namespace gm77
}  // namespace esphome

#endif  // ESPHOME_COMPONENTS_GM77_GM77_H