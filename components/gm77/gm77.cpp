#include "esphome/core/log.h"
#include <ArduinoJson.h>
#include "bc127.h"

namespace esphome
{
  namespace gm77
  {

    static const char *const TAG = "gm77";

    GM77Component::GM77Component()
    {
      // Constructor vacío o inicialización si es necesario
    }

    void GM77Component::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up module");
      if (controller == nullptr)
      {
        controller = this;
      }
      else
      {
        ESP_LOGE(TAG, "Already have an instance of the GM77");
      }
      this->send_command("RESET");
      ESP_LOGCONFIG(TAG, "Resetting module");
    }

    void GM77Component::loop()
    {
      // Verifica si hay datos disponibles en la UART
      if (this->available())
      {
        String received_data = "";

        // Lee los datos de la UART
        while (this->available())
        {
          char c = this->read();
          received_data += c;
          // ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
          // Si detectamos el carácter de fin de línea, procesamos el comando
          if (c == '\r')
          {
            ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
            this->process_data(received_data);
            received_data = ""; // Reinicia para el próximo mensaje
          }
        }
      }
    
    void GM77Component::send_command(const uint8_t *command, size_t length) {
      for (size_t i = 0; i < length; ++i) {
        this->write(command[i]);
    }
    ESP_LOGI(TAG, "Command sent");
}

void GM77Component::wake_up() {
  const uint8_t wake_up_command[] = {0x00};
  this->send_command(wake_up_command, sizeof(wake_up_command));
  ESP_LOGI(TAG, "wake_up command sent");
}

void GM77Component::led_on() {
  const uint8_t led_on_command[] = {0x05, 0xE7, 0x04, 0x00, 0x01, 0xFF, 0x0F};
  this->send_command(led_on_command, sizeof(led_on_command));
  ESP_LOGI(TAG, "LED turned on");
    

    void GM77Component::dump_config()
    {
      ESP_LOGCONFIG(TAG, "GM77 module:");
      // Puedes agregar cualquier configuración que quieras mostrar aquí
      // Ejemplo:
      ESP_LOGCONFIG(TAG, "  Configured for UART communication with GM77");
    }

    

    GM77Component *controller = nullptr;
  } // namespace gm77
} // namespace esphome
