#include "mks42d.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mks42d {

static const char *const TAG = "mks42d";

void MKS42DComponent::setup() {
  this->last_io_millis_ = millis();
}
void MKS42DComponent::loop() {
  uint32_t now = millis();
  if (now - this->last_io_millis_ >= this->throttle_) {
    this->last_io_millis_ = now;
    this->query_io_status();
    this->query_stall();
  }
}


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
  if (x[0] == 0xFE) {
    static const char *const MOTOR_STATES[] = {
      "run fail", "run starting", "run complete", "end limit stopped"
    };
    // ESP_LOGD(TAG, "Step control response = 0x%02X", x[1]);
    if (this->step_state_text_sensor_ != nullptr) {
      uint8_t idx = x[1] < 4 ? x[1] : 0;
      this->step_state_text_sensor_->publish_state(MOTOR_STATES[idx]);
    }
  }
  if (x[0] == 0x91){
    static const char *const HOMING_STATES[] = {
      "go home fail", "go home start", "go home success"
    };
    if (this->home_state_text_sensor_ != nullptr) {
      uint8_t idx = x[1] < 3 ? x[1] : 0;
      this->home_state_text_sensor_->publish_state(HOMING_STATES[idx]);
    }
  }
  if (x.size() >= 3 && x[0] == 0x34) {
    uint8_t status = x[1];
    bool in1 = status & 0x01;
    bool in2 = status & 0x02;
    bool out1 = status & 0x04;
    bool out2 = status & 0x08;

    ESP_LOGD(TAG, "IO Status: IN1=%u IN2=%u OUT1=%u OUT2=%u", in1, in2, out1, out2);
    if (this->in1_state_text_sensor_)
      this->in1_state_text_sensor_->publish_state(in1 ? "ON" : "OFF");
    if (this->in2_state_text_sensor_)
      this->in2_state_text_sensor_->publish_state(in2 ? "ON" : "OFF");
    if (this->out1_state_text_sensor_)
      this->out1_state_text_sensor_->publish_state(out1 ? "ON" : "OFF");
    if (this->out2_state_text_sensor_)
      this->out2_state_text_sensor_->publish_state(out2 ? "ON" : "OFF");
  }
  if (x.size() >= 2 && x[0] == 0x3E) {
    uint8_t stall_status = x[1];

    std::string new_state;
    if (stall_status == 1) {
      new_state = "stalled";
    } else {
      new_state = "normal";
    }

    if (this->stall_state_text_sensor_ != nullptr) {
      this->stall_state_text_sensor_->publish_state(new_state);
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
  ESP_LOGD(TAG, "[can_id=%u] Action: set_target_position(target_position=%d, speed=%d, acceleration=%d)", this->can_id_, target_position, speed, acceleration);
  std::vector<uint8_t> data;
  data.push_back(0xFE);  // Command ID
  data.push_back((speed >> 8) & 0xFF);
  data.push_back(speed & 0xFF);
  data.push_back(acceleration & 0xFF);
  data.push_back((target_position >> 16) & 0xFF);
  data.push_back((target_position >> 8) & 0xFF);
  data.push_back(target_position & 0xFF);

  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);

  send_raw(data);
}

void MKS42DComponent::send_home() {
  ESP_LOGD(TAG, "[can_id=%u] Action: send_home", this->can_id_);
  std::vector<uint8_t> data = {0x91, 0x92};
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}
void MKS42DComponent::enable_rotation(const std::string &state) {
  ESP_LOGD(TAG, "[can_id=%u] Action: enable_rotation(state=%s)", this->can_id_, state.c_str());
  std::vector<uint8_t> data = {0xF3, (state == "ON") ? 0x01 : 0x00};
  uint8_t crc = this->can_id_ + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::send_limit_remap(const std::string &state) {
  ESP_LOGD(TAG, "[can_id=%u] Action: send_limit_remap(state=%s)", this->can_id_, state.c_str());
  std::vector<uint8_t> data = {0x9E, (state == "ON") ? 0x01 : 0x00};
  uint8_t crc = this->can_id_ + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::query_stall() {
  ESP_LOGD(TAG, "[can_id=%u] Action: query_stall", this->can_id_);
  std::vector<uint8_t> data = {0x3E};
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::unstall_command() {
  ESP_LOGD(TAG, "[can_id=%u] Action: unstall_command", this->can_id_);
  std::vector<uint8_t> data = {0x3D};
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_no_limit_reverse(int degrees, int current_ma) {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_no_limit_reverse(degrees=%d, current_ma=%d)", this->can_id_, degrees, current_ma);
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
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_direction(const std::string &dir) {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_direction(dir=%s)", this->can_id_, dir.c_str());
  std::vector<uint8_t> data = {0x86, (dir == "CCW") ? 0x01 : 0x00};
  uint8_t crc = this->can_id_ + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_microstep(int microstep) {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_microstep(microstep=%d)", this->can_id_, microstep);
  std::vector<uint8_t> data = {0x84, (uint8_t)(microstep & 0xFF)};
  uint8_t crc = this->can_id_ + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_hold_current(int percent) {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_hold_current(percent=%d)", this->can_id_, percent);
  uint8_t value = (percent / 10) - 1;
  std::vector<uint8_t> data = {0x9B, value};
  uint8_t crc = this->can_id_ + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_working_current(int ma) {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_working_current(ma=%d)", this->can_id_, ma);
  std::vector<uint8_t> data = {
    0x83,
    (uint8_t)((ma >> 8) & 0xFF),
    (uint8_t)(ma & 0xFF)
  };
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_mode(int mode) {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_mode(mode=%d)", this->can_id_, mode);
  std::vector<uint8_t> data = {0x82, (uint8_t)mode};
  uint8_t crc = this->can_id_ + data[0] + data[1];
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_home_params(bool hm_trig_level, const std::string &hm_dir, int hm_speed, bool end_limit, bool sensorless) {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_home_params(trigger_level=%s, dir=%s, speed=%d, end_limit=%s, sensorless=%s)",
           this->can_id_, hm_trig_level?"true":"false", hm_dir.c_str(), hm_speed, end_limit?"true":"false", sensorless?"true":"false");
  std::vector<uint8_t> data = {
    0x90,
    (uint8_t)(hm_trig_level ? 1 : 0),
    (uint8_t)(hm_dir == "CCW" ? 1 : 0),
    (uint8_t)((hm_speed >> 8) & 0xFF),
    (uint8_t)(hm_speed & 0xFF),
    (uint8_t)(end_limit ? 1 : 0),
    (uint8_t)(sensorless ? 1 : 0)
  };
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::query_io_status() {
  ESP_LOGD(TAG, "[can_id=%u] Action: query_io_status", this->can_id_);
  std::vector<uint8_t> data = {0x34};
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

void MKS42DComponent::set_0() {
  ESP_LOGD(TAG, "[can_id=%u] Action: set_0", this->can_id_);
  std::vector<uint8_t> data = {0x92};
  uint8_t crc = this->can_id_;
  for (auto b : data) crc += b;
  data.push_back(crc & 0xFF);
  send_raw(data);
}

}  // namespace mks42d
}  // namespace esphome
