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
  // Additional setup if needed
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
        // Process the received data
        if (received_data.find("QR") != std::string::npos || received_data.find("BARCODE") != std::string::npos) {
          this->start_decode();
        }
        received_data.clear();  // Reset for the next message
      }
    }
  }
}

void GM77Component::send_command(const uint8_t *command, size_t length) {
  this->write_array(command, length);
  this->flush();
  ESP_LOGD(TAG, "Command sent: %s", esphome::hexencode(command, length).c_str());
}

void GM77Component::enable_continuous_scan() {
  uint8_t command[] = {0x04, 0xE9, 0x04, 0x00, 0xFF, 0x0F};
  send_command(command, sizeof(command));
  ESP_LOGD(TAG, "Continuous scan mode enabled");
}

void GM77Component::disable_continuous_scan() {
  uint8_t command[] = {0x04, 0xEA, 0x04, 0x00, 0xFF, 0x0E};
  send_command(command, sizeof(command));
  ESP_LOGD(TAG, "Continuous scan mode disabled");
}

void GM77Component::start_decode() {
  uint8_t command[] = {0x04, 0xE4, 0x04, 0x00, 0xFF, 0x14};
  send_command(command, sizeof(command));
  ESP_LOGD(TAG, "Decoding started");
}

void GM77Component::stop_decode() {
  uint8_t command[] = {0x04, 0xE5, 0x04, 0x00, 0xFF, 0x13};
  send_command(command, sizeof(command));
  ESP_LOGD(TAG, "Decoding stopped");
}

void GM77Component::handle_not_read() {
  uint8_t not_read_message[] = {0x07, 0xC6, 0x04, 0x08, 0x00, 0x5E, 0x01, 0xFE, 0xC8};
  // Handle the "Not read" message
  ESP_LOGD(TAG, "Not read message received");
}

}  // namespace gm77
}  // namespace esphome