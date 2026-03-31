#include "vento.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace vento {

static const char *const TAG = "vento";
static const int PUBLISH_COUNT = 3;
static const uint32_t PUBLISH_INTERVAL_MS = 2000;

float VentoComponent::get_setup_priority() const {
  return setup_priority::AFTER_CONNECTION;
}

void VentoComponent::setup() {
  if (this->manifest_.empty()) {
    ESP_LOGW(TAG, "No manifest to publish");
    this->mark_failed();
  }
}

void VentoComponent::loop() {
  if (this->publish_count_ >= PUBLISH_COUNT)
    return;

  auto *mqtt = mqtt::global_mqtt_client;
  if (mqtt == nullptr || !mqtt->is_connected())
    return;

  uint32_t now = esphome::millis();
  if (this->last_publish_ != 0 && now - this->last_publish_ < PUBLISH_INTERVAL_MS)
    return;

  std::string topic = mqtt->get_topic_prefix() + "/manifest";
  mqtt->publish(topic, this->manifest_, 0, true);
  this->last_publish_ = now;
  this->publish_count_++;
  ESP_LOGI(TAG, "Published manifest to %s (%d bytes, attempt %d/%d)",
           topic.c_str(), this->manifest_.size(), this->publish_count_, PUBLISH_COUNT);
}

}  // namespace vento
}  // namespace esphome
