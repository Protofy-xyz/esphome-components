#include "pca9632.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pca9632 {

static const char *const TAG = "pca9632";

void PCA9632Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PCA9632...");

  // MODE1: SLEEP=0 (bit 4), AI=1 (bit 5) → 0x20
  write_register(0x00, 0x20);

  // MODE2: DMBLNK=1 (bit 5), OCH=0 (bit 3), INVRT=0 → 0x20
  write_register(0x01, 0x20);

  // LEDOUT: Default to PWM control
  write_register(0x08, 0xAA);
}

void PCA9632Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PCA9632:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PCA9632 failed!");
  }
}

void PCA9632Component::write_register(uint8_t reg, uint8_t value) {
  uint8_t data[2] = {reg, value};
  this->write(data, 2);  // esto envía el registro seguido del valor
}

void PCA9632Component::set_pwm(uint8_t channel, uint8_t value) {
  if (channel > 3) return;
  this->write_register(0x02 + channel, value);  // PWM0 starts at 0x02
  ESP_LOGD(TAG, "Set PWM channel %d to %d", channel, value);
}

void PCA9632Component::set_current(uint8_t value) {
  // Set PWM0-3 to this brightness
  for (uint8_t i = 0; i < 4; i++) {
    this->set_pwm(i, value);
  }
  ESP_LOGD(TAG, "Set current to %d", value);
}

void PCA9632Component::set_color(uint8_t r, uint8_t g, uint8_t b) {
  this->set_pwm(0, r);  // Red
  this->set_pwm(1, g);  // Green
  this->set_pwm(2, b);  // Blue
  ESP_LOGD(TAG, "Set color to R:%d G:%d B:%d", r, g, b);
}

void PCA9632Component::set_group_control_mode(bool use_blinking) {
  if (use_blinking) {
    // Set all channels to group control (0b11) = 0xFF
    write_register(0x08, 0xFF);
    // Enable DMBLNK (bit 5) in MODE2 for blinking
    write_register(0x01, 0x20);
  } else {
    // Back to PWM mode (0b10) = 0xAA
    write_register(0x08, 0xAA);
    // Disable DMBLNK (bit 5)
    write_register(0x01, 0x00);
  }
  ESP_LOGD(TAG, "Set group control mode to %s", use_blinking ? "blinking" : "normal");
}


void PCA9632Component::set_blinking(uint16_t period_ms, uint8_t duty_cycle) {
  // Blink period = (GRPFREQ + 1) / 24 seconds → GRPFREQ = period_s * 24 - 1
  uint8_t freq = std::min<uint8_t>(255, std::max(0, ((period_ms * 24) / 1000) - 1));
  this->write_register(0x07, freq);        // GRPFREQ
  this->write_register(0x06, duty_cycle);  // GRPPWM
  ESP_LOGD(TAG, "Set blinking period to %d ms with duty cycle %d%%", period_ms, duty_cycle);
}


}  // namespace pca9632
}  // namespace esphome
