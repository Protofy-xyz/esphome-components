#pragma once

#include "esphome/core/component.h"
#include <string>

namespace esphome {
namespace vento {

class VentoComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;

  void set_manifest(const char *manifest) { this->manifest_ = manifest; }

 protected:
  std::string manifest_;
  bool published_{false};
};

}  // namespace vento
}  // namespace esphome
