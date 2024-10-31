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




void SDCardComponent::append_to_json_file(const char *filename, JsonObject& data_entry) {
  File file = SD.open(filename, FILE_READ);
  bool is_new_file = !file || file.size() == 0;
  String fileContent = "";

  // Check if file is new or empty
  if (is_new_file) {
    ESP_LOGI(TAG, "File is new");
  }
  if (!is_new_file) {
    // Read the current content
    while (file.available()) {
      fileContent += (char)file.read();
    }
  }
  file.close();

  if (is_new_file || fileContent.length() <= 3) { 
    // Initialize with an array if new or empty
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
      ESP_LOGE(TAG, "Failed to create file");
      return;
    }
    file.print("[\n");
  } else {
    // Remove last `\n]` to append the new object
    int last_bracket = fileContent.lastIndexOf(']');
    if (last_bracket != -1) {
      fileContent.remove(last_bracket - 1, 2);
    }
    file = SD.open(filename, FILE_WRITE);
    file.print(fileContent);
    file.print(",\n");
  }

  // Write the new JSON object as a single entry in the array
  String output;
  serializeJson(data_entry, output);
  file.print(output);
  file.print("\n]");  // Close the JSON array

  file.close();
  ESP_LOGI(TAG, "JSON data appended successfully: %s", output.c_str());
}


void SDCardComponent::store_sensor_data(const char *filename) {
  StaticJsonDocument<4096> doc;  // Adjust size if needed
  JsonObject data_entry = doc.to<JsonObject>();

  // Create the sensors array to hold each sensor's data
  JsonArray sensors_array = data_entry.createNestedArray("sensors");

  // Loop through each sensor and add its data to the sensors array
  for (sensor::Sensor *sensor : this->sensors_) {
    JsonObject sensor_data = sensors_array.createNestedObject();
    
    // Explicitly get the sensor name from its ID
    sensor_data["name"] = sensor->get_name().c_str();  // Ensures that we use the actual sensor name
    sensor_data["value"] = sensor->get_state();        // Store the sensor value
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

    tempFile.print("[\n");  // Start the JSON array in the temporary file with a newline

    String line;
    bool first_object = true;

    while (file.available()) {
        line = file.readStringUntil('\n');  // Read each line until a newline character is found

        if (line.length() == 0) {
            continue;  // Skip empty lines
        }

        line.trim();  // Remove any leading/trailing whitespace

        // Try to deserialize the JSON line
        DynamicJsonDocument doc(1024*10);  // Adjust the size if needed
        DeserializationError error = deserializeJson(doc, line);

        if (error) {
            // ESP_LOGE(TAG, "Failed to parse JSON line: %s", error.c_str());
            continue;
        }

        JsonObject obj = doc.as<JsonObject>();
        for (JsonPair kv : obj) {
            JsonObject sensor_data = kv.value().as<JsonObject>();
            if (!sensor_data["sent"].as<bool>()) {
                // Send the data (replace with actual MQTT send logic)
                ESP_LOGI(TAG, "Sending data for sensor: %s, value: %f", kv.key().c_str(), sensor_data["value"].as<float>());
                //remove the sent key
                sensor_data.remove("sent");
                // Serialize the JSON object to a string
                String payload;
                serializeJson(sensor_data, payload);
                // Send the data via MQTT
                mqtt::global_mqtt_client->publish(this->publish_data_topic_.c_str(), payload.c_str());

                // Assume sending is successful and mark as sent
                sensor_data["sent"] = true;
            }

            // Write back to the temporary file
            if (!first_object) {
                tempFile.print(",\n");  // Add a comma and newline between JSON objects
            } else {
                first_object = false;
            }

            serializeJson(doc, tempFile);  // Write the updated object to the temporary file
        }
    }

    tempFile.print("\n]");  // Close the JSON array with a newline and bracket

    file.close();
    tempFile.close();

    // Remove the original file and rename the temp file to the original file name only if there has been some data processed
    if (!first_object) {
        SD.remove(this->json_file_name_.c_str());
        SD.rename(tempFileName.c_str(), this->json_file_name_.c_str());
        // ESP_LOGI(TAG, "Updated JSON file with sent data");
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
