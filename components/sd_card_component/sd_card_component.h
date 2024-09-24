#pragma once

#include "esphome/core/hal.h"
#include "esphome.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace sd_card_component {

class SDCardComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  char* read_file(const char *filename);
  void write_file(const char *filename, const char *data);
  void append_file(const char *filename, const char *data);
  void add_sensor(sensor::Sensor *sensor);
  void append_to_json_file(const char *filename, JsonObject& new_object);
  void store_sensor_data(const char *filename);
  
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_cs_pin(int cs_pin) { this->cs_pin_ = cs_pin; }
  void set_json_file_name(const std::string &json_file_name) { this->json_file_name_ = json_file_name; }
  void set_interval_seconds(uint32_t interval_seconds) { this->interval_seconds_ = interval_seconds; }

  void set_publish_data_when_online(bool publish_data_when_online);
  void set_publish_data_topic(const std::string &publish_data_topic);

  void process_pending_json_entries();

 protected:
  File file_;
  int cs_pin_;
  std::string json_file_name_;
  uint32_t interval_seconds_;
  std::vector<sensor::Sensor *> sensors_;
  time::RealTimeClock *time_;
  bool publish_data_when_online_;
  std::string publish_data_topic_;
};

}  // namespace sd_card_component
}  // namespace esphome
