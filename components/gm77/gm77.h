#pragma once

#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome.h"
#include <ArduinoJson.h>

// #include <SoftwareSerial.h>

namespace esphome
{
  namespace gm77
  {

    class GM77Component : public Component, public uart::UARTDevice
    {
    public:
      GM77Component();
      virtual ~GM77Component() {}
      void setup() override;
      void loop() override;
      void dump_config() override;
      float get_setup_priority() const override { return setup_priority::DATA; }
      void add_on_tag_callback(std::function<void()> &&trigger_function);
      std::string tag;

      void send_command(const uint8_t *command, size_t length);
      void wake_up();
      void led_on();
      void host_mode();
      void scan_enable();
      void start_decode();
      void continuous_scanning();


    protected:
      void process_data(const String &data);
      CallbackManager<void()> on_tag_callbacks;
    };

    extern GM77Component *controller;
  }
}
