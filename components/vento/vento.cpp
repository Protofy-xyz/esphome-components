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
  if (this->manifest_.empty()) {
    ESP_LOGW(TAG, "No manifest to publish");
    this->mark_failed();
    return;
  }
}

void VentoComponent::update() {
  auto *mqtt = mqtt::global_mqtt_client;
  if (mqtt == nullptr || !mqtt->is_connected())
    return;

  std::string topic = mqtt->get_topic_prefix() + "/manifest";
  mqtt->publish(topic, this->manifest_, 0, true);

  if (!this->published_) {
    this->published_ = true;
    ESP_LOGI(TAG, "Published manifest to %s (%d bytes)", topic.c_str(), this->manifest_.size());
  }
}

}  // namespace vento
}  // namespace esphome
