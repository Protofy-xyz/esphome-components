#include "sd_card_component.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/core/log.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace sd_card_component {

static const char *const TAG = "sd_card";

void SDCardComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up sd card component");

  // Use native SPI bus with this->spi_
  this->spi_setup();


  int retries = 2;
  while (!SD.begin(this->cs_pin_->get_pin()) && retries > 0) {
      ESP_LOGE(TAG, "Failed to initialize SD card, retrying... (%d)", retries);
      retries--;
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

void SDCardComponent::add_number(number::Number *number) {
  this->numbers_.push_back(number);
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
  // Open the file in append mode
  File file = SD.open(filename, FILE_APPEND);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for appending JSON data");
    return;
  }

  // Serialize the JSON entry and append it to the file, followed by a newline
  serializeJson(data_entry, file);
  file.println();
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


  // Create the numbers array to hold each number's data
  JsonArray numbers_array = data_entry.createNestedArray("numbers");

  // Loop through each number and add its data to the numbers array
  for (number::Number *number : this->numbers_) {
    JsonObject number_data = numbers_array.createNestedObject();
    number_data["name"] = number->get_name().c_str();
    number_data["value"] = number->state;
  }

  // Get the timestamp and add it to the JSON entry
  auto time = this->time_->now();
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d", 
           time.year, time.month, time.day_of_month, 
           time.hour, time.minute, time.second);
  data_entry["timestamp"] = timestamp;

  // Check MQTT connection and send immediately if connected
  bool mqtt_sent = false;
  if (mqtt::global_mqtt_client->is_connected()) {
    String payload;
    data_entry["sent_immediately"] = true;
    serializeJson(data_entry, payload);
    mqtt_sent = mqtt::global_mqtt_client->publish(this->publish_data_topic_.c_str(), payload.c_str());
    if (mqtt_sent) {
      ESP_LOGI(TAG, "Data sent to MQTT for timestamp: %s", timestamp);
    } else {
      ESP_LOGW(TAG, "Failed to send data to MQTT for timestamp: %s", timestamp);
      data_entry["sent_immediately"] = false;
    }
  } else {
    ESP_LOGW(TAG, "MQTT not connected, storing data for later for timestamp: %s", timestamp);
    data_entry["sent_immediately"] = false;
  }

  // Append the entire data entry to the JSON file if time is valid and data was not sent
  if (time.year != 1970 && !mqtt_sent) {
    this->append_to_json_file(filename, data_entry);
  } else {
    if(time.year == 1970) {
      ESP_LOGW(TAG, "Invalid time, skipping storage of sensor data");
    }
    if(mqtt_sent) {
      ESP_LOGW(TAG, "Data already sent to MQTT, skipping storage of sensor data");
    }
  }
}

void SDCardComponent::process_pending_json_entries() {
  const int max_publishes_per_run = 50; // Limit entries per loop
  int publish_count = 0;
  static unsigned long last_position = 0;  // Track last file position
  static unsigned long last_line = 0; // Track the line number

  // Open the main file for reading
  File file = SD.open(this->json_file_name_.c_str(), FILE_READ);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open JSON file for reading");
    return;
  }

  // Move to the last known position in the file
  file.seek(last_position);

  // Open a temporary file for writing retained entries
  File tempFile = SD.open("/temp.json", FILE_WRITE);
  if (!tempFile) {
    ESP_LOGE(TAG, "Failed to open temporary file for writing");
    file.close();
    return;
  }


  // Process each line in the original file
  bool first_entry = true;
  while (file.available() && publish_count < max_publishes_per_run) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) continue;

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, line);

    if (error) {
      ESP_LOGE(TAG, "Failed to parse JSON entry, error: %s", error.c_str());
      continue;
    }

    JsonObject entry = doc.as<JsonObject>();

    if (!entry["sent_immediately"].as<bool>() && mqtt::global_mqtt_client->is_connected()) {
      String payload;
      serializeJson(entry, payload);
      if (mqtt::global_mqtt_client->publish(this->publish_data_topic_.c_str(), payload.c_str())) {
        ESP_LOGI(TAG, "Data sent to MQTT for timestamp: %s", entry["timestamp"].as<const char*>());
        publish_count++;
        last_line++;  // Increment line number counter
        continue;  // Skip writing this entry to temp file
      } else {
        ESP_LOGW(TAG, "Failed to send data to MQTT for timestamp: %s", entry["timestamp"].as<const char*>());
      }
    }

    serializeJson(doc, tempFile);
    first_entry = false;
    
    last_line++; // Increment line number counter
  }

  // Save the current position in the file for the next call
  last_position = file.position();

  file.close();
  tempFile.close();

  if (publish_count >= max_publishes_per_run) {
    ESP_LOGI(TAG, "Processed %d entries this run, will resume next time. Last line number is: %lu", publish_count, last_line);
  } else {
    // Reset position and line number if all entries are processed
    last_position = 0;
    last_line = 0;
    SD.remove(this->json_file_name_.c_str());
    SD.rename("/temp.json", this->json_file_name_.c_str());
    ESP_LOGI(TAG, "All pending entries processed and file updated");
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
