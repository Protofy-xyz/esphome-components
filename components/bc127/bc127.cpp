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
      ESP_LOGCONFIG(TAG, this->rx_);
      ESP_LOGCONFIG(TAG, this->tx_);
      ESP_LOGCONFIG(TAG, this->baudrate_);

    }

    void BC127Component::loop()
    {
      ESP_LOGCONFIG(TAG, "Looping module");

    }

    void BC127Component::dump_config()
    {
      ESP_LOGCONFIG(TAG, "dump_config");
    }

  }
} 