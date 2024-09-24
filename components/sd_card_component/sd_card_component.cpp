#include "sd_card_component.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/core/log.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace sd_card_component {

static const char *const TAG = "sd_card";

void SDCardComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up sd card component");


  SPI.begin(18, 19, 23, this->cs_pin_); // Use get_pin() to obtain the pin number

  if (!SD.begin(this->cs_pin_)) {
    ESP_LOGE(TAG, "Card failed, or not present");
    return;
  }
  ESP_LOGI(TAG, "SD card initialized.");
}

void SDCardComponent::loop() {
  // store sensor data every interval_seconds
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

  if (is_new_file || fileContent.length() <= 3) { 
    // If the file is new or effectively empty, initialize with an array and newline
    file = SD.open(filename, FILE_WRITE);
    if (!file) {
      ESP_LOGE(TAG, "Failed to create file");
      return;
    }
    file.print("[\n");
  } else {
    // Otherwise, remove the last `\n]` to append the new object
    int last_comma = fileContent.lastIndexOf(',');
    int last_bracket = fileContent.lastIndexOf(']');
    if (last_bracket != -1 && last_bracket > last_comma) {
      fileContent.remove(last_bracket - 1, 2);  // Remove the newline and closing bracket
    } else {
      fileContent.remove(last_comma);  // Remove the last comma if it exists
    }

    file = SD.open(filename, FILE_WRITE);
    file.print(fileContent);
    file.print(",\n");  // Add a comma and a newline to separate the new object
  }

  // Write the new JSON object with a newline
  String output;
  serializeJson(new_object, output);
  file.print(output);
  file.print("\n]");  // Close the JSON array with a newline and bracket

  file.close();
  ESP_LOGI(TAG, "JSON object appended successfully: %s", output.c_str());
}


void SDCardComponent::store_sensor_data(const char *filename) {
  for (sensor::Sensor *sensor : this->sensors_) {
    StaticJsonDocument<1024> doc;
    JsonObject new_object = doc.to<JsonObject>();

    const std::string sensor_id_str = sensor->get_object_id();
    const char *sensor_id = sensor_id_str.c_str();
    float value = sensor->get_state();

    // Get timestamp from the time component
    auto time = this->time_->now();
    char timestamp[20];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d", 
             time.year, time.month, time.day_of_month, 
             time.hour, time.minute, time.second);

    new_object[sensor_id]["value"] = value;
    new_object[sensor_id]["timestamp"] = timestamp;
    new_object[sensor_id]["sent"] = false;

    // Append to json file only if time is valid
    if (time.year != 1970) {
      this->append_to_json_file(filename, new_object);
    } else {
      ESP_LOGE(TAG, "Invalid time, skipping sensor data storage");
    }
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
