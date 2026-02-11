#include "tfmini_plus.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tfmini_plus {

static const char *const TAG = "tfmini_plus";

void TFMiniPlusComponent::setup() {
  ESP_LOGI(TAG, "Setting up TFMini Plus (I2C)...");
  esphome::delay(500);

  // Request firmware version
  ESP_LOGI(TAG, "Requesting firmware version...");
  uint8_t cmd_ver[] = {0x5A, 0x04, 0x01, 0x5F};
  if (this->write(cmd_ver, sizeof(cmd_ver)) == i2c::ERROR_OK) {
    esphome::delay(100);
    uint8_t resp[7];
    if (this->read(resp, sizeof(resp)) == i2c::ERROR_OK) {
      if (resp[0] == 0x5A && resp[1] == 0x07 && resp[2] == 0x01) {
        ESP_LOGI(TAG, "Firmware version: V%d.%d.%d", resp[5], resp[4], resp[3]);
      } else {
        char hex[32]; int hlen = 0;
        for (int i = 0; i < 7 && hlen < 28; i++)
          hlen += snprintf(hex + hlen, sizeof(hex) - hlen, "%s%02X", i ? "," : "", resp[i]);
        ESP_LOGW(TAG, "Unexpected version response: %s", hex);
      }
    } else {
      ESP_LOGW(TAG, "No response to version request");
    }
  }

  // Configure distance unit
  if (this->distance_unit_mm_) {
    ESP_LOGD(TAG, "Setting distance unit to mm");
    uint8_t cmd[] = {0x5A, 0x05, 0x05, 0x06, 0x6A};
    this->write(cmd, sizeof(cmd));
    esphome::delay(50);
  }

  // Configure frame rate
  if (this->frame_rate_set_) {
    ESP_LOGD(TAG, "Setting frame rate to %u Hz", this->frame_rate_);
    uint8_t rate_l = this->frame_rate_ & 0xFF;
    uint8_t rate_h = (this->frame_rate_ >> 8) & 0xFF;
    uint8_t cmd[] = {0x5A, 0x06, 0x03, rate_l, rate_h, 0x00};
    cmd[5] = (cmd[0] + cmd[1] + cmd[2] + cmd[3] + cmd[4]) & 0xFF;
    this->write(cmd, sizeof(cmd));
    esphome::delay(50);
  }

  // Save settings if changed
  if (this->distance_unit_mm_ || this->frame_rate_set_) {
    ESP_LOGD(TAG, "Saving settings");
    uint8_t cmd[] = {0x5A, 0x04, 0x11, 0x6F};
    this->write(cmd, sizeof(cmd));
    esphome::delay(50);
  }
}

void TFMiniPlusComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "TFMini Plus (I2C):");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
  ESP_LOGCONFIG(TAG, "  Distance unit: %s", this->distance_unit_mm_ ? "mm" : "cm");
  if (this->frame_rate_set_)
    ESP_LOGCONFIG(TAG, "  Frame rate: %u Hz", this->frame_rate_);
  LOG_SENSOR("  ", "Distance", this->distance_sensor_);
  LOG_SENSOR("  ", "Strength", this->strength_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
}

void TFMiniPlusComponent::update() {
  // Request data: I2C_FORMAT_CM (0x01) or I2C_FORMAT_MM (0x06)
  uint8_t param = this->distance_unit_mm_ ? 0x06 : 0x01;
  uint8_t checksum = (0x5A + 0x05 + 0x00 + param) & 0xFF;
  uint8_t cmd[] = {0x5A, 0x05, 0x00, param, checksum};

  if (this->write(cmd, sizeof(cmd)) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C write failed");
    return;
  }

  esphome::delay(10);

  uint8_t data[9];
  if (this->read(data, sizeof(data)) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C read failed");
    return;
  }

  // Verify header and checksum
  if (data[0] != 0x59 || data[1] != 0x59) {
    char hex[40]; int hlen = 0;
    for (int i = 0; i < 9 && hlen < 36; i++)
      hlen += snprintf(hex + hlen, sizeof(hex) - hlen, "%s%02X", i ? "," : "", data[i]);
    ESP_LOGW(TAG, "Invalid frame: %s", hex);
    return;
  }

  uint8_t cksum = 0;
  for (int i = 0; i < 8; i++) cksum += data[i];
  if (cksum != data[8]) {
    ESP_LOGW(TAG, "Checksum mismatch");
    return;
  }

  uint16_t distance = data[2] | (data[3] << 8);
  uint16_t strength = data[4] | (data[5] << 8);
  int16_t temp_raw = data[6] | (data[7] << 8);
  float temperature = temp_raw / 8.0f - 256.0f;

  ESP_LOGD(TAG, "Distance: %u %s, Strength: %u, Temp: %.1f C",
           distance, this->distance_unit_mm_ ? "mm" : "cm",
           strength, temperature);

  if (this->distance_sensor_ != nullptr)
    this->distance_sensor_->publish_state(distance);
  if (this->strength_sensor_ != nullptr)
    this->strength_sensor_->publish_state(strength);
  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temperature);
}

}  // namespace tfmini_plus
}  // namespace esphome
