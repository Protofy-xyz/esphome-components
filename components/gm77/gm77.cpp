#include "gm77.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gm77 {

static const char *const TAG = "gm77";

GM77Component::GM77Component() {
  // Constructor vacío o inicialización si es necesario
}

void GM77Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up GM77...");
}

void GM77Component::loop() {
  // Check if data is available on UART
  if (this->available()) {
    std::string received_data;

    // Read data from UART
    while (this->available()) {
      char c = this->read();
      received_data += c;

      // If end of line character is detected, process the command
      if (c == '\r') {
        ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
        this->process_data(received_data);
        received_data.clear();  // Reset for the next message
      }
    }
  }
}

void GM77Component::process_data(const std::string &data) {
  ESP_LOGD(TAG, "Processing data: %s", data.c_str());
  // Implement data processing logic here
}

}  // namespace gm77
}  // namespace esphome