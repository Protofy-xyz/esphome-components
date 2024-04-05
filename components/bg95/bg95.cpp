
#include "bg95.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bg95 {

static const char *const TAG = "bg95";
HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

#define GSM_PIN "ss"
const char apn[] = "YourAPN";
const char gprsUser[] = "ss";
const char gprsPass[] = "ss";
bool is_modem_on = false;
void BG95Component::modem_turn_on() {
  this->enable_pin_->digital_write(true);
  this->on_pin_->digital_write(true);
  vTaskDelay(50);
  this->on_pin_->digital_write(false);
}
void BG95Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BG95...");
  
  this->enable_pin_->setup();
  this->enable_pin_->digital_write(false);
  this->on_pin_->setup();
  this->on_pin_->digital_write(false);

  vTaskDelay(50);
  modem_turn_on();

  ESP_LOGCONFIG(TAG, "Initializing modem...");
  // Set GSM module baud rate
  SerialAT.begin(115200,SERIAL_8N1,16,17,false);  

  /**/
  ESP_LOGCONFIG(TAG, "Initializing modem...");
  modem.init();

  String modemInfo = modem.getModemInfo();
  ESP_LOGCONFIG(TAG, "Modem info: %s", modemInfo);
  ESP_LOGCONFIG(TAG, "\r\n---- Setup Done ----");
}

float BG95Component::get_setup_priority() const { return setup_priority::DATA; }
void BG95Component::dump_config() {
  ESP_LOGCONFIG(TAG, "BG95:");
}

}  // namespace bg95
}  // namespace esphome

