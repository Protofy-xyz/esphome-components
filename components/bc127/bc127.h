#pragma once

#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome.h"
#include <ArduinoJson.h>
#include "automation.h"
#include "phoneContactManager.h"

#define BC127_NOT_READY 0
#define BC127_READY 1
#define BC127_START_PAIRING 2
#define BC127_CONNECTED 3
#define BC127_INCOMING_CALL 4
#define BC127_CALL_IN_COURSE 5
#define BC127_CALL_BLOCKED 6
#define BC127_CALL_OUTGOING 7

// #include <SoftwareSerial.h>

namespace esphome
{
  namespace bc127
  {

    class BC127Component : public Component, public uart::UARTDevice
    {
    public:
      std::vector<uint8_t> bytes = {'a', 'b', 'c'};
      std::string callerId;
      BC127Component();
      virtual ~BC127Component() {}
      void setup() override;
      void loop() override;
      void dump_config() override;
      float get_setup_priority() const override { return setup_priority::DATA; }
      void add_on_connected_callback(std::function<void()> &&trigger_function);
      void add_on_call_callback(std::function<void()> &&trigger_function);
      void add_on_ended_call_callback(std::function<void()> &&trigger_function);

      void call_dial(const char *phone_number);
      void call_answer();
      void call_reject();
      void call_end();

      void add_phone_contact(const char *name, const char *number);
      void remove_phone_contact(const char *name, const char *number);
      const std::vector<std::string> get_contacts();


      void set_onetime(int val) { this->onetime = val; }
      int get_onetime() { return this->onetime; }

    protected:
      int onetime;
      int state = 0;
      String ble_phone_address = "";
      String hfp_connection_id = "";
      void process_data(const String &data);
      void send_command(const std::string &command);
      void set_state(int state);
      CallbackManager<void()> on_connected_callbacks;
      CallbackManager<void()> on_call_callbacks;
      CallbackManager<void()> on_ended_call_callbacks;
      PhoneContactManager phoneContactManager;
    };

    extern BC127Component *controller;
  }
}
