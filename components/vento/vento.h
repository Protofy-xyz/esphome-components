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

  void set_yaml_hash(const char *hash) { this->yaml_hash_ = hash; }

 protected:
  std::string yaml_hash_;
  bool published_{false};
};

}  // namespace vento
}  // namespace esphome
