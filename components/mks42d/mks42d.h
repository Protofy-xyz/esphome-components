#pragma once

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
  void dump_config() override;
  void process_frame(const std::vector<uint8_t> &data);
  void set_debug_received_messages(bool debug) { this->debug_received_messages_ = debug; }

  void set_target_position(int32_t target_position, int speed, int acceleration);


 protected:
  uint8_t can_id_;
  canbus::Canbus *canbus_{nullptr};
  bool debug_received_messages_{false};

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

}  // namespace mks42d
}  // namespace esphome
