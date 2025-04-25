#include "mks42d.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mks42d {

static const char *const TAG = "mks42d";

void MKS42DComponent::setup() {}

void MKS42DComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MKS42D Component:");
  ESP_LOGCONFIG(TAG, "  CAN ID: %u", this->can_id_);
}

void MKS42DComponent::process_frame(const std::vector<uint8_t> &x) {
  if (this->debug_received_messages_) {
    ESP_LOGD(TAG, "Received raw CAN data:");
    for (size_t i = 0; i < x.size(); i++) {
      ESP_LOGD(TAG, "  data[%d] = 0x%02X", i, x[i]);
    }
  }
}

void MKS42DComponent::send_raw(const std::vector<uint8_t> &data) {
  if (this->debug_received_messages_) {
    ESP_LOGD(TAG, "Sending CAN frame:");
    for (size_t i = 0; i < data.size(); i++) {
      ESP_LOGD(TAG, "  data[%d] = 0x%02X", i, data[i]);
    }
  }
  if (this->canbus_ != nullptr)
    this->canbus_->send_data(this->can_id_, false, data);
}

void MKS42DComponent::set_target_position(int32_t target_position, int speed, int acceleration) {
  std::vector<uint8_t> data;
  data.push_back(0xFE);  // Command ID
  data.push_back((speed >> 8) & 0xFF);
  data.push_back(speed & 0xFF);
  data.push_back(acceleration & 0xFF);
  data.push_back((target_position >> 16) & 0xFF);
  data.push_back((target_position >> 8) & 0xFF);
  data.push_back(target_position & 0xFF);

  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);

  send_raw(data);
}

void MKS42DComponent::send_home() {
  std::vector<uint8_t> data = {0x91, 0x92};
  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}
void MKS42DComponent::enable_rotation(const std::string &state) {
  std::vector<uint8_t> data = {0xF3, (state == "ON") ? 0x01 : 0x00};
  uint8_t crc = 1 + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::send_limit_remap(const std::string &state) {
  std::vector<uint8_t> data = {0x9E, (state == "ON") ? 0x01 : 0x00};
  uint8_t crc = 1 + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::query_stall() {
  std::vector<uint8_t> data = {0x3E};
  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::unstall_command() {
  std::vector<uint8_t> data = {0x3D};
  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_no_limit_reverse(int degrees, int current_ma) {
  uint32_t ret = (degrees * 0x4000) / 360;
  std::vector<uint8_t> data = {
    0x94,
    (uint8_t)((ret >> 24) & 0xFF),
    (uint8_t)((ret >> 16) & 0xFF),
    (uint8_t)((ret >> 8) & 0xFF),
    (uint8_t)(ret & 0xFF),
    (uint8_t)((current_ma >> 8) & 0xFF),
    (uint8_t)(current_ma & 0xFF)
  };
  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_direction(const std::string &dir) {
  std::vector<uint8_t> data = {0x86, (dir == "CCW") ? 0x01 : 0x00};
  uint8_t crc = 1 + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_microstep(int microstep) {
  std::vector<uint8_t> data = {0x84, (uint8_t)(microstep & 0xFF)};
  uint8_t crc = 1 + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_hold_current(int percent) {
  uint8_t value = (percent / 10) - 1;
  std::vector<uint8_t> data = {0x9B, value};
  uint8_t crc = 1 + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_working_current(int ma) {
  std::vector<uint8_t> data = {
    0x83,
    (uint8_t)((ma >> 8) & 0xFF),
    (uint8_t)(ma & 0xFF)
  };
  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_mode(int mode) {
  std::vector<uint8_t> data = {0x82, (uint8_t)mode};
  uint8_t crc = 1 + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_home_params(bool hm_trig_level, const std::string &hm_dir, int hm_speed, bool end_limit, bool sensorless) {
  std::vector<uint8_t> data = {
    0x90,
    (uint8_t)(hm_trig_level ? 1 : 0),
    (uint8_t)(hm_dir == "CCW" ? 1 : 0),
    (uint8_t)((hm_speed >> 8) & 0xFF),
    (uint8_t)(hm_speed & 0xFF),
    (uint8_t)(end_limit ? 1 : 0),
    (uint8_t)(sensorless ? 1 : 0)
  };
  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::query_io_status() {
  std::vector<uint8_t> data = {0x34};
  uint8_t crc = 1;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

}  // namespace mks42d
}  // namespace esphome
