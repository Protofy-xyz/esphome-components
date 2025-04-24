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

  this->canbus_->send_data(this->can_id_, false, data);
}


}  // namespace mks42d
}  // namespace esphome
