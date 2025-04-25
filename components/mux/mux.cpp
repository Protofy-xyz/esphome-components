#include "mux.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mux {

static const char *const TAG = "mux";

void MUXComponent::set_sel_pins(GPIOPin *sel0, GPIOPin *sel1, GPIOPin *sel2) {
  sel0_ = sel0;
  sel1_ = sel1;
  sel2_ = sel2;
  sel0_->setup();
  sel1_->setup();
  sel2_->setup();
}

void MUXComponent::set_result_pin(GPIOPin *result) {
  result_ = result;
  result_->setup();
}

bool MUXComponent::digital_read(uint8_t channel) {
  sel0_->digital_write((channel >> 0) & 0x01);
  sel1_->digital_write((channel >> 1) & 0x01);
  sel2_->digital_write((channel >> 2) & 0x01);
  delayMicroseconds(5);  // Allow signals to settle
  return result_->digital_read();
}

void MUXGPIOPin::setup() {
  // Called once at boot; optional
  this->pin_mode(flags_);
}

void MUXGPIOPin::pin_mode(gpio::Flags flags) {
  // In this design, there's nothing to do
  this->flags_ = flags;
}

bool MUXGPIOPin::digital_read() {
  return parent_->digital_read(channel_) != this->inverted_;
}

void MUXGPIOPin::digital_write(bool) {
  // Optional: implement if you want output capability later
}

std::string MUXGPIOPin::dump_summary() const {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "MUX channel %u", channel_);
  return std::string(buffer);
}

}  // namespace mux
}  // namespace esphome
