#include "automation.h"
#include "bc127.h"

namespace esphome {
namespace bc127 {

BC127ConnectedTrigger::BC127ConnectedTrigger(BC127Component *obj) {
  std::vector<uint8_t> bytes = obj->bytes;
  obj->add_on_connected_callback([this, bytes]() { this->trigger(bytes); });
}

IncomingCallTrigger::IncomingCallTrigger(BC127Component *obj) {
  std::string bytes = obj->callerId;
  obj->add_on_call_callback([this, bytes]() { this->trigger(bytes); });
}

EndedCallTrigger::EndedCallTrigger(BC127Component *obj) {
  std::string bytes = obj->callerId;
  obj->add_on_ended_call_callback([this, bytes]() { this->trigger(bytes); });
}

// Implementation of StartCallAction
StartCallAction::StartCallAction(BC127Component *parent, const std::string &number)
    : parent_(parent), number_(number) {}

void StartCallAction::play() {
  if (this->parent_->state != BC127_CONNECTED) {
    ESP_LOGW("BC127", "Cannot make call: BC127 is not connected.");
    return;
  }

  ESP_LOGI("BC127", "Triggering call to: %s", this->number_.c_str());
  this->parent_->start_call(this->number_);
}

}  // namespace bc127
}  // namespace esphome
