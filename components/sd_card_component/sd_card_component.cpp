#include "sd_card_component.h"
#include "esphome/core/log.h"

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
  ESP_LOGI(TAG, "JSON object appended successfully");
}





}  // namespace sd_card_component
}  // namespace esphome
