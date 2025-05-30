#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace ds3231 {

class DS3231Component : public time::RealTimeClock, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void read_time();
  void write_time();
  void set_time(uint16_t year, uint16_t month, uint16_t day_of_month, uint16_t day_of_week, uint16_t hour, uint16_t minute, uint16_t second);

 protected:
  bool read_rtc_();
  bool write_rtc_();
  union DS3231Reg {
    struct {
      uint8_t second : 4;
      uint8_t second_10 : 3;
      bool ch : 1;

      uint8_t minute : 4;
      uint8_t minute_10 : 3;
      uint8_t unused_1 : 1;

      uint8_t hour : 4;
      uint8_t hour_10 : 2;
      uint8_t unused_2 : 2;

      uint8_t weekday : 3;
      uint8_t unused_3 : 5;

      uint8_t day : 4;
      uint8_t day_10 : 2;
      uint8_t unused_4 : 2;

      uint8_t month : 4;
      uint8_t month_10 : 1;
      uint8_t unused_5 : 3;

      uint8_t year : 4;
      uint8_t year_10 : 4;

      uint8_t rs : 2;
      uint8_t unused_6 : 2;
      bool sqwe : 1;
      uint8_t unused_7 : 2;
      bool out : 1;
    } reg;
    mutable uint8_t raw[sizeof(reg)];
  } ds3231_;
};

template<typename... Ts> class WriteAction : public Action<Ts...>, public Parented<DS3231Component> {
 public:
  void play(Ts... x) override { this->parent_->write_time(); }
};

template<typename... Ts> class ReadAction : public Action<Ts...>, public Parented<DS3231Component> {
 public:
  void play(Ts... x) override { this->parent_->read_time(); }
};
}  // namespace ds3231
}  // namespace esphome
