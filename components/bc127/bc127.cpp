#include "esphome/core/log.h"
#include <ArduinoJson.h>
#include "bc127.h"

namespace esphome
{
  namespace bc127
  {

    static const char *const TAG = "bc127";

    void BC127Component::setup()
    {
      StaticJsonDocument<256> doc;
      ESP_LOGCONFIG(TAG, "Setting up module");
      // ESP_LOGCONFIG(TAG, this->rx_);
      // ESP_LOGCONFIG(TAG, this->tx_);
      // ESP_LOGCONFIG(TAG, this->baudrate_);
      

    }

    void BC127Component::loop()
    {
      ESP_LOGCONFIG(TAG, "Looping module");
      //   if (this->available()) {
      //   String received_data = "";
      //   while (this->available()) {
      //     char c = this->read();
      //     received_data += c;
      //   }
      //   ESP_LOGD(TAG, "Data received: %s", received_data.c_str());
      //   //this->process_data(received_data);
      // }

    }

    void BC127Component::dump_config()
    {
      ESP_LOGCONFIG(TAG, "BC127 module:");
      // ESP_LOGCONFIG(TAG, "  TX Pin: %d", this->tx_);
      // ESP_LOGCONFIG(TAG, "  RX Pin: %d", this->rx_);
      // ESP_LOGCONFIG(TAG, "  Baud Rate: %d", this->baudrate_);
    }

  }
} 
