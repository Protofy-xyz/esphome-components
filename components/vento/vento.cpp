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

  auto *mqtt = mqtt::global_mqtt_client;
  if (mqtt == nullptr) {
    ESP_LOGE(TAG, "MQTT client not available");
    this->mark_failed();
    return;
  }

  mqtt->set_on_connect([this, mqtt](bool session_present) {
    std::string topic = mqtt->get_topic_prefix() + "/manifest";
    mqtt->publish(topic, this->manifest_, 0, true);
    ESP_LOGI(TAG, "Published manifest to %s (%d bytes)", topic.c_str(), this->manifest_.size());
  });
}

}  // namespace vento
}  // namespace esphome
