#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace sfm4300 {

enum GasType : uint8_t {
  GAS_O2 = 0,
  GAS_AIR = 1,
  GAS_N2O = 2,
  GAS_CO2 = 3,
};

class SFM4300Sensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;

  void set_gas_type(GasType gas_type) { this->gas_type_ = gas_type; }

 protected:
  bool send_command_(uint16_t cmd);
  bool read_data_(uint8_t *data, size_t len);
  static uint8_t crc8_(const uint8_t *data, size_t len);
  uint16_t get_start_command_() const;

  GasType gas_type_{GAS_O2};
  float scale_factor_{0};
  int16_t offset_{0};
  bool initialized_{false};
};

}  // namespace sfm4300
}  // namespace esphome
