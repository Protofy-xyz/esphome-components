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
  // Enable continuous scan mode on setup
  this->enable_continuous_scan();
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

void GM77Component::send_command(const std::string &command) {
  this->write_array(reinterpret_cast<const uint8_t *>(command.c_str()), command.length());
  this->flush();
  ESP_LOGD(TAG, "Command sent: %s", command.c_str());
}

void GM77Component::enable_continuous_scan() {
  this->send_command(SCAN_ENABLE_COMMAND);
  this->send_command(CONTINUOUS_MODE_COMMAND);
}

void GM77Component::disable_scan() {
  this->send_command(SCAN_DISABLE_COMMAND);
}

void GM77Component::process_data(const std::string &data) {
  if (data.find("SCAN_SUCCESS") != std::string::npos) {
    ESP_LOGD(TAG, "Scan successful: %s", data.c_str());
    this->start_decode();
  } else {
    ESP_LOGD(TAG, "Scan not successful: %s", data.c_str());
  }
}

void GM77Component::start_decode() {
  // Implement decoding logic here
  ESP_LOGD(TAG, "Decoding started");
}

} // namespace gm77
} // namespace esphome