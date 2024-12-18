#include "esphome/core/log.h"
#include <ArduinoJson.h>
#include "gm77.h"  // Corrected header file name

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
    return;  // Avoid further initialization if another instance exists
  }
}
void GM77Component::loop() {
  // Verifica si hay datos disponibles en la UART
  if (this->available()) {
    String received_data = "";

    // Lee los datos de la UART
    while (this->available()) {
      char c = this->read();
      received_data += c;
      // ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
      // Si detectamos el carácter de fin de línea, procesamos el comando
      if (c == '\r') {
        ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
        this->process_data(received_data);
        received_data = "";  // Reinicia para el próximo mensaje
      }
    }
  }
}
// Send a raw command to the module
void GM77Component::send_command(const uint8_t *command, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    this->write_byte(command[i]);  // Correct method to write bytes
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

void GM77Component::scan_enable() {
  const uint8_t scan_enable_command[] = {0x04, 0xE4, 0x04, 0x00, 0xFF, 0x14};
  this->send_command(scan_enable_command, sizeof(scan_enable_command));
  ESP_LOGI(TAG, "Scan enable command sent");
}

void GM77Component::continuous_scanning() {
  const uint8_t continuous_scanning_command[] = {0x07, 0xC6, 0x04, 0x08, 0x00, 0x8A, 0x04, 0xFE, 0x99};
  this->send_command(continuous_scanning_command, sizeof(continuous_scanning_command));
  ESP_LOGI(TAG, "Continuous scanning command sent");
}

void GM77Component::host_mode() {
  const uint8_t host_mode_command[] = {0x07, 0xC6, 0x04, 0x08, 0x00, 0x8A, 0x08, 0xFE, 0x95};
  this->send_command(host_mode_command, sizeof(host_mode_command));
  ESP_LOGI(TAG, "Continuous scanning command sent");
}

void GM77Component::start_decode() {
  const uint8_t start_decode_command[] = {0x04, 0xE4, 0x04, 0x00, 0xFF, 0x14};
  this->send_command(start_decode_command, sizeof(start_decode_command));
  ESP_LOGI(TAG, "Start decode command sent");
  // Dump configuration
}

void GM77Component::process_data(const String &data) {

   this->add_on_tag_callback([this]()
                                     { ESP_LOGD(TAG, "ADD ON TAG CALLBACK"); });
          auto &callbacks = on_tag_callbacks;
          callbacks.call();
}

void GM77Component::add_on_tag_callback(std::function<void()> &&trigger_function)
    {
      on_tag_callbacks.add(std::move(trigger_function));
    }
void GM77Component::dump_config() {
  ESP_LOGCONFIG(TAG, "GM77 module configuration:");
  ESP_LOGCONFIG(TAG, "  Configured for UART communication");
}

}  // namespace gm77
}  // namespace esphome
