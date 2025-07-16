#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"

#include <vector>

namespace esphome {
namespace gps {

class GPS;

class GPSListener {
 public:
  virtual void on_update() = 0;
 protected:
  friend GPS;
  GPS *parent_;
};

class GPS : public PollingComponent, public uart::UARTDevice {
 public:
  void set_latitude_sensor(sensor::Sensor *s) { latitude_sensor_ = s; }
  void set_longitude_sensor(sensor::Sensor *s) { longitude_sensor_ = s; }
  void set_speed_sensor(sensor::Sensor *s) { speed_sensor_ = s; }
  void set_course_sensor(sensor::Sensor *s) { course_sensor_ = s; }
  void set_altitude_sensor(sensor::Sensor *s) { altitude_sensor_ = s; }
  void set_satellites_sensor(sensor::Sensor *s) { satellites_sensor_ = s; }
  void set_hdop_sensor(sensor::Sensor *s) { hdop_sensor_ = s; }

  void register_listener(GPSListener *listener) {
    listener->parent_ = this;
    listeners_.push_back(listener);
  }

  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void dump_config() override;
  void loop() override;
  void update() override;

 protected:
  void parse_nmea_sentence_(const std::string &sentence);
  float parse_coordinate(const std::string &value, const std::string &hemisphere);


  float latitude_{NAN};
  float longitude_{NAN};
  float speed_{NAN};
  float course_{NAN};
  float altitude_{NAN};
  uint16_t satellites_{0};
  float hdop_{NAN};

  std::string rx_buffer_;

  sensor::Sensor *latitude_sensor_{nullptr};
  sensor::Sensor *longitude_sensor_{nullptr};
  sensor::Sensor *speed_sensor_{nullptr};
  sensor::Sensor *course_sensor_{nullptr};
  sensor::Sensor *altitude_sensor_{nullptr};
  sensor::Sensor *satellites_sensor_{nullptr};
  sensor::Sensor *hdop_sensor_{nullptr};

  std::vector<GPSListener *> listeners_;
};

}  // namespace gps
}  // namespace esphome
