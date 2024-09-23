#pragma once

#include "esphome/core/hal.h"
#include "esphome.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "esphome/components/time/real_time_clock.h"  // Add this line

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
  void set_time(time::RealTimeClock *time) { this->time_ = time; }  // Add this line

 protected:
  File file_;
  int cs_pin_;
  std::vector<sensor::Sensor *> sensors_;
  time::RealTimeClock *time_;  // Add this line
};

}  // namespace sd_card_component
}  // namespace esphome
