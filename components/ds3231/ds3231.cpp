#include "ds3231.h"
#include "esphome/core/log.h"

// Datasheet:
// - https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf

namespace esphome {
namespace ds3231 {

static const char *const TAG = "ds3231";

void DS3231Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up DS3231...");
  if (!this->read_rtc_()) {
    this->mark_failed();
  }
}

void DS3231Component::update() { this->read_time(); }

void DS3231Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DS3231:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with DS3231 failed!");
  }
  ESP_LOGCONFIG(TAG, "  Timezone: '%s'", this->timezone_.c_str());
}

float DS3231Component::get_setup_priority() const { return setup_priority::DATA; }

void DS3231Component::read_time() {
  if (!this->read_rtc_()) {
    return;
  }
  if (ds3231_.reg.ch) {
    ESP_LOGW(TAG, "RTC halted, not syncing to system clock.");
    return;
  }
  uint8_t raw_dow = ds3231_.reg.weekday;
  if (raw_dow == 0) {
    raw_dow = 7;  
  }
  ESPTime rtc_time{.second = uint8_t(ds3231_.reg.second + 10 * ds3231_.reg.second_10),
                         .minute = uint8_t(ds3231_.reg.minute + 10u * ds3231_.reg.minute_10),
                         .hour = uint8_t(ds3231_.reg.hour + 10u * ds3231_.reg.hour_10),
                         .day_of_week = uint8_t(ds3231_.reg.weekday),
                         .day_of_month = uint8_t(ds3231_.reg.day + 10u * ds3231_.reg.day_10),
                         .day_of_year = 1,  // ignored by recalc_timestamp_utc(false)
                         .month = uint8_t(ds3231_.reg.month + 10u * ds3231_.reg.month_10),
                         .year = uint16_t(ds3231_.reg.year + 10u * ds3231_.reg.year_10 + 2000)};
  rtc_time.recalc_timestamp_utc(false);
  if (!rtc_time.is_valid()) {
    ESP_LOGE(TAG, "Invalid RTC time, not syncing to system clock.");
    return;
  }
  time::RealTimeClock::synchronize_epoch_(rtc_time.timestamp);
}

void DS3231Component::write_time() {
  auto now = time::RealTimeClock::utcnow();
  if (!now.is_valid()) {
    ESP_LOGE(TAG, "Invalid system time, not syncing to RTC.");
    return;
  }
  ds3231_.reg.year = (now.year - 2000) % 10;
  ds3231_.reg.year_10 = (now.year - 2000) / 10 % 10;
  ds3231_.reg.month = now.month % 10;
  ds3231_.reg.month_10 = now.month / 10;
  ds3231_.reg.day = now.day_of_month % 10;
  ds3231_.reg.day_10 = now.day_of_month / 10;
  ds3231_.reg.weekday = now.day_of_week;
  ds3231_.reg.hour = now.hour % 10;
  ds3231_.reg.hour_10 = now.hour / 10;
  ds3231_.reg.minute = now.minute % 10;
  ds3231_.reg.minute_10 = now.minute / 10;
  ds3231_.reg.second = now.second % 10;
  ds3231_.reg.second_10 = now.second / 10;
  ds3231_.reg.ch = false;

  this->write_rtc_();
}

void DS3231Component::set_time(uint16_t year, uint16_t month, uint16_t day_of_month, uint16_t day_of_week, uint16_t hour, uint16_t minute, uint16_t second) {
  //auto now = time::RealTimeClock::utcnow();
  //if (!now.is_valid()) {
  //  ESP_LOGE(TAG, "Invalid system time, not syncing to RTC.");
  //  return;
  //}
  ds3231_.reg.year = (year - 2000) % 10;
  ds3231_.reg.year_10 = (year - 2000) / 10 % 10;
  ds3231_.reg.month = month % 10;
  ds3231_.reg.month_10 = month / 10;
  ds3231_.reg.day = day_of_month % 10;
  ds3231_.reg.day_10 = day_of_month / 10;
  ds3231_.reg.weekday = day_of_week;
  ds3231_.reg.hour = hour % 10;
  ds3231_.reg.hour_10 = hour / 10;
  ds3231_.reg.minute = minute % 10;
  ds3231_.reg.minute_10 = minute / 10;
  ds3231_.reg.second = second % 10;
  ds3231_.reg.second_10 = second / 10;
  ds3231_.reg.ch = false;

  this->write_rtc_();
}

bool DS3231Component::read_rtc_() {
  if (!this->read_bytes(0, this->ds3231_.raw, sizeof(this->ds3231_.raw))) {
    ESP_LOGE(TAG, "Can't read I2C data.");
    return false;
  }
  ESP_LOGD(TAG, "Read  %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u  CH:%s RS:%0u SQWE:%s OUT:%s", ds3231_.reg.hour_10,
           ds3231_.reg.hour, ds3231_.reg.minute_10, ds3231_.reg.minute, ds3231_.reg.second_10, ds3231_.reg.second,
           ds3231_.reg.year_10, ds3231_.reg.year, ds3231_.reg.month_10, ds3231_.reg.month, ds3231_.reg.day_10,
           ds3231_.reg.day, ONOFF(ds3231_.reg.ch), ds3231_.reg.rs, ONOFF(ds3231_.reg.sqwe), ONOFF(ds3231_.reg.out));

  return true;
}

bool DS3231Component::write_rtc_() {
  if (!this->write_bytes(0, this->ds3231_.raw, sizeof(this->ds3231_.raw))) {
    ESP_LOGE(TAG, "Can't write I2C data.");
    return false;
  }
  ESP_LOGD(TAG, "Write %0u%0u:%0u%0u:%0u%0u 20%0u%0u-%0u%0u-%0u%0u  CH:%s RS:%0u SQWE:%s OUT:%s", ds3231_.reg.hour_10,
           ds3231_.reg.hour, ds3231_.reg.minute_10, ds3231_.reg.minute, ds3231_.reg.second_10, ds3231_.reg.second,
           ds3231_.reg.year_10, ds3231_.reg.year, ds3231_.reg.month_10, ds3231_.reg.month, ds3231_.reg.day_10,
           ds3231_.reg.day, ONOFF(ds3231_.reg.ch), ds3231_.reg.rs, ONOFF(ds3231_.reg.sqwe), ONOFF(ds3231_.reg.out));
  return true;
}
}  // namespace ds3231
}  // namespace esphome
