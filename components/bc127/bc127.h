#pragma once

#include "esphome/core/hal.h"
#include "esphome/core/component.h"
#include "esphome.h"
#include <ArduinoJson.h>
// #include <SoftwareSerial.h>

namespace esphome
{
  namespace bc127
  {

    class BC127Component : public Component  {
    public:
      void setup() override;
      void loop() override;
      void dump_config() override;
      float get_setup_priority() const override { return setup_priority::DATA; }
      //   char* read_file(const char *filename);
      //   void write_file(const char *filename, const char *data);
      //   void append_file(const char *filename, const char *data);
      //   void add_sensor(sensor::Sensor *sensor);
      //   void append_to_json_file(const char *filename, JsonObject& new_object);
      //   void store_sensor_data(const char *filename);

      //   void set_time(time::RealTimeClock *time) { this->time_ = time; }
      void set_rx(int pin) { this->rx_ = pin; }
      void set_tx(int pin) { this->tx_ = pin; }
      void set_baudrate(uint32_t bauds) { this->baudrate_ = bauds; }
      //   void set_json_file_name(const std::string &json_file_name) { this->json_file_name_ = json_file_name; }
      //   void set_interval_seconds(uint32_t interval_seconds) { this->interval_seconds_ = interval_seconds; }

      //   void set_publish_data_when_online(bool publish_data_when_online);
      //   void set_publish_data_topic(const std::string &publish_data_topic);

      //   void process_pending_json_entries();

    protected:
      int rx_;
      int tx_;
      uint32_t baudrate_;
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

  }
}