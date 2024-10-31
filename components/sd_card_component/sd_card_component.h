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
  
  // Method to add a sensor to the SD card component
  void add_sensor(sensor::Sensor *sensor);

  // Modified methods to append all sensor data in one write
  void append_to_json_file(const char *filename, JsonObject& sensors_data);
  void store_sensor_data(const char *filename);

  // Setters for configuration options
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_cs_pin(int cs_pin) { this->cs_pin_ = cs_pin; }
  void set_json_file_name(const std::string &json_file_name) { this->json_file_name_ = json_file_name; }
  void set_interval_seconds(uint32_t interval_seconds) { this->interval_seconds_ = interval_seconds; }
  void set_publish_data_when_online(bool publish_data_when_online);
  void set_publish_data_topic(const std::string &publish_data_topic);

  // Method to process and send pending JSON entries
  void process_pending_json_entries();

 protected:
  File file_;
  int cs_pin_;                               // Chip select pin for SD card
  std::string json_file_name_;               // Name of JSON file for storing sensor data
  uint32_t interval_seconds_;                // Interval between data storage
  std::vector<sensor::Sensor *> sensors_;    // List of sensors added to the component
  time::RealTimeClock *time_;                // Real-time clock for timestamping
  bool publish_data_when_online_;            // Flag to publish data when online
  std::string publish_data_topic_;           // MQTT topic for publishing data
  bool sd_card_initialized_ = false;         // SD card initialization status
};

}  // namespace sd_card_component
}  // namespace esphome
