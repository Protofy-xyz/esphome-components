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
// #include <SoftwareSerial.h>

namespace esphome
{
  namespace bc127
  {

    class BC127Component : public Component, public uart::UARTDevice
    {
    public:
      std::vector<uint8_t> bytes = { 'a', 'b','c' };
      std::string callerId;
      BC127Component();
      virtual ~BC127Component() {}
      void setup() override;
      void loop() override;
      void dump_config() override;
      float get_setup_priority() const override { return setup_priority::DATA; }
      void add_on_connected_callback(std::function<void()> &&trigger_function);
      void add_on_call_callback(std::function<void()> &&trigger_function);

      void call_answer();
      void call_reject();
      void call_end();

      void add_phone_contact(const char *name, const char *number);

      //   char* read_file(const char *filename);
      //   void write_file(const char *filename, const char *data);
      //   void append_file(const char *filename, const char *data);
      //   void add_sensor(sensor::Sensor *sensor);
      //   void append_to_json_file(const char *filename, JsonObject& new_object);
      //   void store_sensor_data(const char *filename);

      //   void set_time(time::RealTimeClock *time) { this->time_ = time; }
      void set_onetime(int val) { this->onetime = val; }
      int get_onetime() { return this->onetime; }

      // void set_tx(int pin) { this->tx_ = pin; }
      // void set_baudrate(uint32_t bauds) { this->baudrate_ = bauds; }
      //   void set_json_file_name(const std::string &json_file_name) { this->json_file_name_ = json_file_name; }
      //   void set_interval_seconds(uint32_t interval_seconds) { this->interval_seconds_ = interval_seconds; }

      //   void set_publish_data_when_online(bool publish_data_when_online);
      //   void set_publish_data_topic(const std::string &publish_data_topic);

      //   void process_pending_json_entries();

    protected:
      int onetime;
      int state = 0;
      String ble_phone_address= "";
      String hfp_connection_id = "";
      void process_data(const String &data);
      void send_command(const std::string &command);
      void set_state(int state);
      CallbackManager<void()>       on_connected_callbacks;
      CallbackManager<void()>       on_call_callbacks;
      PhoneContactManager phoneContactManager;
      // int rx_;
      // int tx_;
      // uint32_t baudrate_;
      // EspSoftwareSerial::UART myPort_;

      //   File file_;
      //   int cs_pin_;
      //   std::string json_file_name_;
      //   uint32_t interval_seconds_;
      //   std::vector<sensor::Sensor *> sensors_;
      //   time::RealTimeClock *time_;
      //   bool publish_data_when_online_;
      //   std::string publish_data_topic_;
      //   bool sd_card_initialized_ = false;
    };

    extern BC127Component *controller;
  }
}