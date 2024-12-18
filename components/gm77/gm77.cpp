#include "esphome/core/log.h"
#include <ArduinoJson.h>
#include "gm77.h" // Corrected header file name

namespace esphome {
namespace gm77 {

static const char *const TAG = "GM77";

// Define the static controller pointer
GM77Component *controller = nullptr;

// Constructor
GM77Component::GM77Component() {
  // Empty or initialization code, if needed
}

// Setup method
void GM77Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up GM77 module");

  if (controller == nullptr) {
    controller = this;
  } else {
    ESP_LOGE(TAG, "Already have an instance of the GM77");
    return; // Avoid further initialization if another instance exists
  }

  this->send_command("RESET");
  ESP_LOGCONFIG(TAG, "Module reset command sent");
}

// Loop method
void GM77Component::loop() {
  // Check if data is available on UART
  if (this->available()) {
    std::string received_data;

    // Read data from UART
    while (this->available()) {
      char c = this->read();
      received_data += c;

      // If newline is detected, process the command
      if (c == '\r') {
        ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
        this->process_data(received_data);
        received_data.clear(); // Clear the buffer for the next message
      }
    }
  }
}

// Send a raw command to the module
void GM77Component::send_command(const uint8_t *command, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    this->write_byte(command[i]); // Correct method to write bytes
  }
  ESP_LOGI(TAG, "Command sent");
}

// Send the wake-up command
void GM77Component::wake_up() {
  const uint8_t wake_up_command[] = {0x00};
  this->send_command(wake_up_command, sizeof(wake_up_command));
  ESP_LOGI(TAG, "Wake-up command sent");
}

// Send the LED ON command
void GM77Component::led_on() {
  const uint8_t led_on_command[] = {0x05, 0xE7, 0x04, 0x00, 0x01, 0xFF, 0x0F};
  this->send_command(led_on_command, sizeof(led_on_command));
  ESP_LOGI(TAG, "LED ON command sent");
}

// Dump configuration
void GM77Component::dump_config() {
  ESP_LOGCONFIG(TAG, "GM77 module configuration:");
  ESP_LOGCONFIG(TAG, "  Configured for UART communication");
}

} // namespace gm77
} // namespace esphome
