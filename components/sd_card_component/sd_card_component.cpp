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

  // Create the initial file if it doesn't exist
  // char* fileContent = read_file("/datalog.txt");
  // if (fileContent == NULL) {
  //   write_file("/datalog.txt", "[]\n");
  // } else {
  //   delete[] fileContent;
  // }

  // // Log sensor values
  // StaticJsonDocument<1024> jsonBuffer;
  // JsonArray jsonArray = jsonBuffer.to<JsonArray>();

  // for (auto &sensor_pair : this->sensors_) {
  //   JsonObject newEntry = jsonArray.createNestedObject();
  //   newEntry["sensor"] = sensor_pair.second;
  //   newEntry["value"] = sensor_pair.first->state;
  //   newEntry["ts"] = id(time_component).now().timestamp;
  // }

  // char output[1024];
  // serializeJson(jsonArray, output, sizeof(output));
  // this->append_json("/datalog.txt", output);
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

}  // namespace sd_card_component
}  // namespace esphome
