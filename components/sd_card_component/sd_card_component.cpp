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
  // Print SD card info: total space and used space
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  ESP_LOGCONFIG(TAG, "\tSD Card Size: %lluMB", cardSize);
  uint64_t cardUsedSize = SD.usedBytes() / (1024 * 1024);
  ESP_LOGCONFIG(TAG, "\tSD Card Used Size: %lluMB", cardUsedSize);
  ESP_LOGCONFIG(TAG, "\tSD Card Free Size: %lluMB", cardSize - cardUsedSize);
}

// Read a file and return it as a string
char* SDCardComponent::read_file(const char *filename) {
  // Open the file
  File file = SD.open(filename);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return NULL;
  }

  // Read the file content
  String fileContent = "";
  while (file.available()) {
    fileContent += (char)file.read();
  }
  // Close the file
  file.close();
  ESP_LOGI(TAG, "File %s read", filename);

  // Allocate memory for the char* to return
  char* result = new char[fileContent.length() + 1];  // +1 for the null terminator
  strcpy(result, fileContent.c_str());

  return result;  // Remember to free this memory after using it
}

// Write a file
void SDCardComponent::write_file(const char *filename, const char *data) {
  // Open the file
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
  }
  // Write to the file
  if (file.print(data)) {
    ESP_LOGI(TAG, "File written");
  } else {
    ESP_LOGE(TAG, "Write failed");
  }
  // Close the file
  file.close();
}

// Append to a file
void SDCardComponent::append_file(const char *filename, const char *data) {
  // Open the file
  File file = SD.open(filename, FILE_APPEND);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for appending");
    return;
  }
  // Write to the file
  if (file.print(data)) {
    ESP_LOGI(TAG, "File appended");
  } else {
    ESP_LOGE(TAG, "Append failed");
  }
  // Close the file
  file.close();
}

void SDCardComponent::append_to_json_file(const char *filename, JsonObject& new_object) {
  // Open the file
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return;
  }
  // If the file doesn't exist, create it
  if (!file) {
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
      ESP_LOGE(TAG, "Failed to create file");
      return;
    }
    file.close();
  }
  //if the file is empty, write an empty array
  if (file.size() == 0) {
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
      ESP_LOGE(TAG, "Failed to open file for writing");
      return;
    }
    if (file.print("[]")) {
      ESP_LOGI(TAG, "Empty array written to file");
    } else {
      ESP_LOGE(TAG, "Failed to write empty array to file");
    }
    file.close();
  }

  // Read the file content
  String fileContent = "";
  while (file.available()) {
    fileContent += (char)file.read();
  }
  file.close();

  // Parse the existing JSON data
  StaticJsonDocument<1024> doc; // Adjust size as needed
  DeserializationError error = deserializeJson(doc, fileContent);
  if (error) {
    ESP_LOGE(TAG, "Failed to parse JSON file: %s", error.c_str());
    return;
  }

  // Check if the root is an array
  JsonArray jsonArray;
  if (doc.is<JsonArray>()) {
    jsonArray = doc.as<JsonArray>();
  } else {
    ESP_LOGE(TAG, "JSON root is not an array");
    return;
  }

  // Append the new object to the JSON array
  jsonArray.add(new_object);

  // Serialize the updated JSON back to a string
  String output;
  serializeJson(doc, output);

  // Write the updated JSON back to the file
  file = SD.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
  }
  if (file.print(output)) {
    ESP_LOGI(TAG, "JSON object appended successfully");
  } else {
    ESP_LOGE(TAG, "Failed to write JSON to file");
  }
  file.close();
}

}  // namespace sd_card_component
}  // namespace esphome
