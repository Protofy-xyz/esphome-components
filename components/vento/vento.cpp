#include "vento.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vento {

static const char *const TAG = "vento";

float VentoComponent::get_setup_priority() const {
  return setup_priority::AFTER_CONNECTION;
}

void VentoComponent::setup() {
  if (this->yaml_hash_.empty()) {
    ESP_LOGW(TAG, "No yaml_hash configured — desync detection disabled");
  }
}

void VentoComponent::loop() {
  if (this->published_ || this->yaml_hash_.empty())
    return;

  auto *mqtt = mqtt::global_mqtt_client;
  if (mqtt == nullptr || !mqtt->is_connected())
    return;

  std::string topic = mqtt->get_topic_prefix() + "/yaml_hash";
  mqtt->publish(topic, this->yaml_hash_, 0, true);
  this->published_ = true;
  ESP_LOGI(TAG, "Published yaml_hash to %s", topic.c_str());
}

}  // namespace vento
}  // namespace esphome
