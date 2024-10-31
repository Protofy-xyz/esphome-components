#include "sd_card_component.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/core/log.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace sd_card_component {

static const char *const TAG = "sd_card";

void SDCardComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up sd card component");

  SPI.begin(18, 19, 23, this->cs_pin_);

  int retries = 5;
  while (!SD.begin(this->cs_pin_) && retries > 0) {
      ESP_LOGE(TAG, "Failed to initialize SD card, retrying... (%d)", retries);
      retries--;
      delay(1000);
  }
  this->sd_card_initialized_ = retries > 0;
  if (this->sd_card_initialized_) {
      ESP_LOGI(TAG, "SD card initialized successfully");
  } else {
      ESP_LOGE(TAG, "Failed to initialize SD card");
  }
}

void SDCardComponent::loop() {
  if (!this->sd_card_initialized_) {
    return;
  }
  static unsigned long last_run = 0;
  if (millis() - last_run > (this->interval_seconds_ * 1000)) {
    this->store_sensor_data(this->json_file_name_.c_str());
    if(this->publish_data_when_online_){
      if(mqtt::global_mqtt_client->is_connected()){
        this->process_pending_json_entries();
      } else {
        ESP_LOGE(TAG, "MQTT not connected, skipping processing of pending JSON entries");
      }
    }
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




void SDCardComponent::append_to_json_file(const char *filename, JsonObject &data_entry) {
  // Open the file in write mode, preserving its content but allowing us to modify the last line
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for appending data");
    return;
  }

  // If the file is empty, initialize the JSON array structure
  if (file.size() == 0) {
    file.print("[\n");
  } else {
    // Seek to the second-to-last character to replace the last `\n]`
    file.seek(file.size() - 2);
    file.print(",\n");
  }

  // Write the new JSON object as a single entry in the array
  String output;
  serializeJson(data_entry, output);
  file.print(output);
  file.print("\n]");  // Close the JSON array

  file.close();
  ESP_LOGI(TAG, "JSON data appended successfully");
}


void SDCardComponent::store_sensor_data(const char *filename) {
  StaticJsonDocument<4096> doc;  // Adjust size if needed
  JsonObject data_entry = doc.to<JsonObject>();

  // Create the sensors array to hold each sensor's data
  JsonArray sensors_array = data_entry.createNestedArray("sensors");

  // Loop through each sensor and add its data to the sensors array
  for (sensor::Sensor *sensor : this->sensors_) {
    JsonObject sensor_data = sensors_array.createNestedObject();
    sensor_data["name"] = sensor->get_name().c_str();
    sensor_data["value"] = sensor->get_state();
  }

  // Get the timestamp and add it to the JSON entry
  auto time = this->time_->now();
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d", 
           time.year, time.month, time.day_of_month, 
           time.hour, time.minute, time.second);
  data_entry["timestamp"] = timestamp;
  data_entry["sent"] = false;

  // Append the entire data entry to the JSON file if time is valid
  if (time.year != 1970) {
    this->append_to_json_file(filename, data_entry);
  } else {
    ESP_LOGE(TAG, "Invalid time, skipping sensor data storage");
  }
}

void SDCardComponent::process_pending_json_entries() {
  File file = SD.open(this->json_file_name_.c_str(), FILE_READ);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open JSON file for reading");
    return;
  }

  String tempFileName = "/temp.json";
  File tempFile = SD.open(tempFileName.c_str(), FILE_WRITE);
  if (!tempFile) {
    ESP_LOGE(TAG, "Failed to open temporary file for writing");
    file.close();
    return;
  }

  DynamicJsonDocument doc(1024 * 50); // Adjust size based on the expected file size
  DeserializationError error = deserializeJson(doc, file);
  file.close(); // Close the file after reading

  if (error) {
    ESP_LOGE(TAG, "Failed to parse JSON array from file");
    tempFile.close();
    SD.remove(tempFileName.c_str());
    return;
  }

  JsonArray array = doc.as<JsonArray>();
  bool data_modified = false;

  // Start the array in the temporary file
  tempFile.print("[\n");
  bool first_entry = true;

  // Iterate over each object in the array
  for (JsonObject entry : array) {
    bool entry_modified = false;

    // Check each sensor's "sent" field and publish unsent entries
    if (!entry["sent"].as<bool>()) {
      // Send data (replace with actual MQTT send logic)
      ESP_LOGI(TAG, "Sending data for timestamp: %s", entry["timestamp"].as<const char*>());
      String payload;
      serializeJson(entry, payload);
      mqtt::global_mqtt_client->publish(this->publish_data_topic_.c_str(), payload.c_str());
      delay(10);

      // Mark as sent after successful publish
      entry["sent"] = true;
      entry_modified = true;
    }

    // Write entry to the temporary file
    if (!first_entry) {
      tempFile.print(",\n"); // Add a comma between objects
    } else {
      first_entry = false;
    }

    // Serialize each entry (modified or not) to the temporary file
    serializeJson(entry, tempFile);

    if (entry_modified) {
      data_modified = true;
    }
  }

  tempFile.print("\n]"); // Close the JSON array
  tempFile.close();

  // Replace original file only if we modified any entries
  if (data_modified) {
    SD.remove(this->json_file_name_.c_str());
    SD.rename(tempFileName.c_str(), this->json_file_name_.c_str());
    ESP_LOGI(TAG, "Updated JSON file with sent data");
  } else {
    // If no data was modified, delete the temporary file
    SD.remove(tempFileName.c_str());
  }
}

void SDCardComponent::set_publish_data_when_online(bool publish_data_when_online) {
  this->publish_data_when_online_ = publish_data_when_online;
}

void SDCardComponent::set_publish_data_topic(const std::string &publish_data_topic) {
  this->publish_data_topic_ = publish_data_topic;
}


}  // namespace sd_card_component
}  // namespace esphome
