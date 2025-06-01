#pragma once

#include "esphome/core/hal.h"
#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/canbus/canbus.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace mks42d {

class MKS42DComponent : public Component {
 public:
  void set_can_id(uint8_t id) { this->can_id_ = id; }
  void set_canbus_parent(canbus::Canbus *canbus) { this->canbus_ = canbus; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  void process_frame(const std::vector<uint8_t> &data);
  void set_debug_received_messages(bool debug) { this->debug_received_messages_ = debug; }

  void send_raw(const std::vector<uint8_t> &data);
  void set_target_position(int32_t target_position, int speed, int acceleration);
  void set_speed(int speed, int acceleration, const std::string &direction);
  void send_home();
  void enable_rotation(const std::string &state);
  void send_limit_remap(const std::string &state);
  void query_stall();
  void unstall_command();
  void set_no_limit_reverse(int degrees, int current_ma);
  void set_direction(const std::string &dir);
  void set_microstep(int microstep);
  void set_hold_current(int percent);
  void set_working_current(int ma);
  void set_mode(int mode);
  void set_home_params(bool hm_trig_level, const std::string &hm_dir, int hm_speed, bool end_limit, bool sensorless);
  void query_io_status();
  void set_0();
  void set_protection(const std::string &state);
  void set_throttle(uint32_t ms) { this->throttle_ = ms; }

  void set_step_state_text_sensor(text_sensor::TextSensor *s) { this->step_state_text_sensor_ = s; }
  void set_home_state_text_sensor(text_sensor::TextSensor *s) { this->home_state_text_sensor_ = s; }
  void set_in1_state_text_sensor(text_sensor::TextSensor *s)   { this->in1_state_text_sensor_  = s; }
  void set_in2_state_text_sensor(text_sensor::TextSensor *s)   { this->in2_state_text_sensor_  = s; }
  void set_out1_state_text_sensor(text_sensor::TextSensor *s)  { this->out1_state_text_sensor_ = s; }
  void set_out2_state_text_sensor(text_sensor::TextSensor *s)  { this->out2_state_text_sensor_ = s; }
  void set_stall_state_text_sensor(text_sensor::TextSensor *s) { this->stall_state_text_sensor_ = s; }

 protected:
  uint8_t can_id_;
  canbus::Canbus *canbus_{nullptr};
  bool debug_received_messages_{false};
  uint32_t throttle_{60000};  // default 60s
  uint32_t last_io_millis_{0};

  text_sensor::TextSensor *step_state_text_sensor_{nullptr};
  text_sensor::TextSensor *home_state_text_sensor_{nullptr};
  text_sensor::TextSensor *in1_state_text_sensor_{nullptr};
  text_sensor::TextSensor *in2_state_text_sensor_{nullptr};
  text_sensor::TextSensor *out1_state_text_sensor_{nullptr};
  text_sensor::TextSensor *out2_state_text_sensor_{nullptr};
  text_sensor::TextSensor *stall_state_text_sensor_{nullptr};
};

//SetTargetPositionAction 
template<typename... Ts>
class SetTargetPositionAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(int32_t, target_position)
  TEMPLATABLE_VALUE(int, speed)
  TEMPLATABLE_VALUE(int, acceleration)

  void play(Ts... x) override {
    this->parent_->set_target_position(this->target_position_.value(x...),
                                       this->speed_.value(x...),
                                       this->acceleration_.value(x...));
  }
};
// SetSpeedAction
template<typename... Ts>
class SetSpeedAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(int, speed)
  TEMPLATABLE_VALUE(int, acceleration)
  TEMPLATABLE_VALUE(std::string, direction)

  void play(Ts... x) override {
    this->parent_->set_speed(
      this->speed_.value(x...),
      this->acceleration_.value(x...),
      this->direction_.value(x...)
    );
  }
};
//SendHomeAction
template<typename... Ts>
class SendHomeAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  void play(Ts... x) override {
    this->parent_->send_home();
  }
};

// EnableRotationAction
template<typename... Ts>
class EnableRotationAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, state)

  void play(Ts... x) override {
    this->parent_->enable_rotation(this->state_.value(x...));
  }
};

// SendLimitRemapAction
template<typename... Ts>
class SendLimitRemapAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, state)

  void play(Ts... x) override {
    this->parent_->send_limit_remap(this->state_.value(x...));
  }
};

// QueryStallAction
template<typename... Ts>
class QueryStallAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  void play(Ts... x) override {
    this->parent_->query_stall();
  }
};

// UnstallCommandAction
template<typename... Ts>
class UnstallCommandAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  void play(Ts... x) override {
    this->parent_->unstall_command();
  }
};

// SetNoLimitReverseAction
template<typename... Ts>
class SetNoLimitReverseAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(int, degrees)
  TEMPLATABLE_VALUE(int, current_ma)

  void play(Ts... x) override {
    this->parent_->set_no_limit_reverse(
      this->degrees_.value(x...),
      this->current_ma_.value(x...)
    );
  }
};

// SetDirectionAction
template<typename... Ts>
class SetDirectionAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, direction)

  void play(Ts... x) override {
    this->parent_->set_direction(this->direction_.value(x...));
  }
};

// SetMicrostepAction
template<typename... Ts>
class SetMicrostepAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(int, microstep)

  void play(Ts... x) override {
    this->parent_->set_microstep(this->microstep_.value(x...));
  }
};

// SetHoldCurrentAction
template<typename... Ts>
class SetHoldCurrentAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(int, percent)

  void play(Ts... x) override {
    this->parent_->set_hold_current(this->percent_.value(x...));
  }
};

// SetWorkingCurrentAction
template<typename... Ts>
class SetWorkingCurrentAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(int, ma)

  void play(Ts... x) override {
    this->parent_->set_working_current(this->ma_.value(x...));
  }
};

// SetModeAction
template<typename... Ts>
class SetModeAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(int, mode)

  void play(Ts... x) override {
    this->parent_->set_mode(this->mode_.value(x...));
  }
};

// SetHomeParamsAction
template<typename... Ts>
class SetHomeParamsAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  TEMPLATABLE_VALUE(bool, hm_trig_level)
  TEMPLATABLE_VALUE(std::string, hm_dir)
  TEMPLATABLE_VALUE(int, hm_speed)
  TEMPLATABLE_VALUE(bool, end_limit)
  TEMPLATABLE_VALUE(bool, sensorless_homing)

  void play(Ts... x) override {
    this->parent_->set_home_params(
      this->hm_trig_level_.value(x...),
      this->hm_dir_.value(x...),
      this->hm_speed_.value(x...),
      this->end_limit_.value(x...),
      this->sensorless_homing_.value(x...)
    );
  }
};

// QueryIOStatusAction
template<typename... Ts>
class QueryIOStatusAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  void play(Ts... x) override {
    this->parent_->query_io_status();
  }
};

// Set0Action
template<typename... Ts>
class Set0Action : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
  void play(Ts... x) override {
    this->parent_->set_0();
  }
};
// SetProtectionAction
template<typename... Ts>
class SetProtectionAction : public Action<Ts...>, public Parented<MKS42DComponent> {
 public:
 TEMPLATABLE_VALUE(std::string, state)

 void play(Ts... x) override {
   this->parent_->set_protection(this->state_.value(x...));
 }
};

}  // namespace mks42d
}  // namespace esphome