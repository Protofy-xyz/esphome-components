#include "sd_card_component.h"
#include "esphome/core/log.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace sd_card_component {

static const char *const TAG = "sd_card";

void SDCardComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up sd card component");

  this->cs_pin_ = 13;
  SPI.begin(18, 19, 23, this->cs_pin_); // sck, miso, mosi, ss(cs)

  if (!SD.begin(this->cs_pin_)) {
    ESP_LOGE(TAG, "Card failed, or not present");
    return;
  }
  ESP_LOGI(TAG, "SD card initialized.");

}

void SDCardComponent::loop() {
  // store sensor data every 5s
  static unsigned long last_run = 0;
  if (millis() - last_run > 5000) {
    this->store_sensor_data("/datalog.json");
    last_run = millis();
  }

}

void SDCardComponent::add_sensor(sensor::Sensor *sensor) {
  this->sensors_.push_back(sensor);
}

void SDCardComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SDCardComponent:");
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  ESP_LOGCONFIG(TAG, "\tSD Card Size: %lluMB", cardSize);
  uint64_t cardUsedSize = SD.usedBytes() / (1024 * 1024);
  ESP_LOGCONFIG(TAG, "\tSD Card Used Size: %lluMB", cardUsedSize);
  ESP_LOGCONFIG(TAG, "\tSD Card Free Size: %lluMB", cardSize - cardUsedSize);
}

char* SDCardComponent::read_file(const char *filename) {
  File file = SD.open(filename);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return NULL;
  }

  String fileContent = "";
  while (file.available()) {
    fileContent += (char)file.read();
  }
  file.close();
  ESP_LOGI(TAG, "File %s read", filename);

  char* result = new char[fileContent.length() + 1];
  strcpy(result, fileContent.c_str());

  return result;
}

void SDCardComponent::write_file(const char *filename, const char *data) {
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
  }
  if (file.print(data)) {
    ESP_LOGI(TAG, "File written");
  } else {
    ESP_LOGE(TAG, "Write failed");
  }
  file.close();
}

void SDCardComponent::append_file(const char *filename, const char *data) {
  File file = SD.open(filename, FILE_APPEND);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for appending");
    return;
  }
  if (file.print(data)) {
    ESP_LOGI(TAG, "File appended");
  } else {
    ESP_LOGE(TAG, "Append failed");
  }
  file.close();
}

void SDCardComponent::append_to_json_file(const char *filename, JsonObject& new_object) {
  File file = SD.open(filename, FILE_READ);
  bool is_new_file = !file || file.size() == 0;
  String fileContent = "";

  if (!is_new_file) {
    // Read the current content
    while (file.available()) {
      fileContent += (char)file.read();
    }
  }
  file.close();

  if (is_new_file || fileContent.length() <= 2) { 
    // If the file is new or effectively empty, initialize with an array
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
      ESP_LOGE(TAG, "Failed to create file");
      return;
    }
    file.print("[");
  } else {
    // Otherwise, remove the last `]` to append new object
    fileContent.remove(fileContent.length() - 1);
    file = SD.open(filename, FILE_WRITE);
    file.print(fileContent);
    file.print(",");  // Add a comma to separate the new object
  }

  // Write the new JSON object
  String output;
  serializeJson(new_object, output);
  file.print(output);

  // Close the JSON array
  file.print("]");

  file.close();
  ESP_LOGI(TAG, "JSON object appended successfully: %s", output.c_str());
}



void SDCardComponent::store_sensor_data(const char *filename) {
  StaticJsonDocument<1024> doc;
  JsonObject new_object = doc.to<JsonObject>();

  for (sensor::Sensor *sensor : this->sensors_) {
    const std::string sensor_id_str = sensor->get_object_id();
    const char *sensor_id = sensor_id_str.c_str();
    float value = sensor->get_state();

    // Get timestamp from the time component
    auto time = this->time_->now();
    char timestamp[20];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
             time.year, time.month, time.day_of_month, 
             time.hour, time.minute, time.second);

    JsonObject sensor_data = new_object.createNestedObject(sensor_id);
    sensor_data["value"] = value;
    sensor_data["timestamp"] = timestamp;
    sensor_data["sent"] = false;
    
    // Append to json file only if time is valid
    if (time.year != 1970) {
      this->append_to_json_file(filename, new_object);
    }else{
      ESP_LOGE(TAG, "Invalid time, skipping sensor data storage");
    }
  }
}
}  // namespace sd_card_component
}  // namespace esphome
