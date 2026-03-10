#pragma once

#include <string>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace sd_card_spi {

class SDCardSPIComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_root_path(const std::string &root_path);
  void set_cs_pin(int pin) { this->cs_pin_ = pin; }
  void set_sck_pin(int pin) { this->sck_pin_ = pin; }
  void set_miso_pin(int pin) { this->miso_pin_ = pin; }
  void set_mosi_pin(int pin) { this->mosi_pin_ = pin; }
  void set_frequency(uint32_t frequency) { this->frequency_ = frequency; }

  bool write_file(const std::string &path, const std::string &data, bool append);
  bool delete_file(const std::string &path);
  bool read_file(const std::string &path, size_t max_bytes, std::string &out);
  bool exists(const std::string &path);
  bool initialized() const { return this->mounted_; }

 protected:
  bool mounted_{false};
  std::string root_path_{"/"};
  uint32_t last_mount_retry_ms_{0};
  int cs_pin_{-1};
  int sck_pin_{-1};
  int miso_pin_{-1};
  int mosi_pin_{-1};
  uint32_t frequency_{4000000};

  bool try_mount_();
  std::string resolve_path_(const std::string &path) const;
};

template<typename... Ts> class WriteFileAction : public Action<Ts...>, public Parented<SDCardSPIComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, path)
  TEMPLATABLE_VALUE(std::string, data)
  TEMPLATABLE_VALUE(bool, append)

  void play(Ts... x) override { this->parent_->write_file(this->path_.value(x...), this->data_.value(x...), this->append_.value(x...)); }
};

template<typename... Ts> class DeleteFileAction : public Action<Ts...>, public Parented<SDCardSPIComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, path)

  void play(Ts... x) override { this->parent_->delete_file(this->path_.value(x...)); }
};

template<typename... Ts> class ReadFileAction : public Action<Ts...>, public Parented<SDCardSPIComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, path)
  TEMPLATABLE_VALUE(size_t, max_bytes)

  void play(Ts... x) override {
    std::string content;
    this->parent_->read_file(this->path_.value(x...), this->max_bytes_.value(x...), content);
  }
};

}  // namespace sd_card_spi
}  // namespace esphome
