#pragma once

//  #include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

namespace esphome {
namespace sd_card_component {

class SDCardComponent : public Component{
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  char* read_file(const char *filename);
  void write_file(const char *filename, const char *data);
  void append_file(const char *filename, const char *data);
  void append_json(const char *filename, const char *data);  

 protected:
  File file_;
  int cs_pin_;
};

}  // namespace sd_card_component
}  // namespace esphome