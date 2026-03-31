#pragma once

#include "esphome/core/component.h"
#include <string>

namespace esphome {
namespace vento {

class VentoComponent : public Component {
 public:
  void setup() override;
  float get_setup_priority() const override;

  void set_manifest(const char *manifest) { this->manifest_ = manifest; }

 protected:
  std::string manifest_;
};

}  // namespace vento
}  // namespace esphome
