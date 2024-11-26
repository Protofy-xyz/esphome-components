#pragma once

#include <memory>
#include "esphome/core/automation.h"
#include "esphome/core/optional.h"

namespace esphome {
namespace bc127 {

class BC127Component; // Forward declaration of BC127Component

class BC127ConnectedTrigger : public Trigger<std::vector<uint8_t>> {
 public:
  explicit BC127ConnectedTrigger(BC127Component *obj);
};

class IncomingCallTrigger : public Trigger<std::string> {
 public:
  explicit IncomingCallTrigger(BC127Component *obj);
};

class EndedCallTrigger : public Trigger<std::string> {
 public:
  explicit EndedCallTrigger(BC127Component *obj);
};

// Action to start a call
class StartCallAction : public Action<> {
 public:
  explicit StartCallAction(BC127Component *parent, const std::string &number);

  void play() override;

  // Sets the phone number for the call
  void set_number(const std::string &number) { this->number_ = number; }

 private:
  BC127Component *parent_;  // Pointer to the BC127Component
  std::string number_;      // Phone number to call
};

}  // namespace bc127
}  // namespace esphome
