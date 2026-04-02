#pragma once

#include <string>
#include <vector>
#include <map>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP_IDF
#include <esp_http_server.h>
#endif

namespace esphome {
namespace setup_portal {

struct PortalField {
  std::string id;
  std::string label;
  std::string type;
  std::string default_value;
  float min_value{0};
  float max_value{0};
  bool has_min{false};
  bool has_max{false};
};

class SetupPortalComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI - 1.0f; }

  void set_title(const std::string &title) { this->title_ = title; }
  void set_pin(const std::string &pin) { this->pin_ = pin; }
  void set_logo_url(const std::string &url) { this->logo_url_ = url; }
  void set_accent_color(const std::string &color) { this->accent_color_ = color; }
  const std::string &get_accent_color() const { return this->accent_color_; }
  void add_field(const std::string &id, const std::string &label,
                 const std::string &type, const std::string &default_value,
                 float min_val, float max_val, bool has_min, bool has_max);

  std::string get_string(const std::string &id, const std::string &default_value = "");
  int get_int(const std::string &id, int default_value = 0);
  float get_float(const std::string &id, float default_value = 0.0f);

  bool check_pin(const std::string &pin);
  bool handle_save(const std::string &body);  // returns true if reboot scheduled

#ifdef USE_ESP_IDF
  // Chunked HTML serving — called from HTTP handlers
  void serve_page(httpd_req_t *req);
  void serve_scan(httpd_req_t *req);
#endif

 protected:
  void start_dns_server_();
  void start_http_server_();
  void handle_dns_();
  void do_scan_();

  void save_to_nvs_(const std::string &key, const std::string &value);
  std::string load_from_nvs_(const std::string &key, const std::string &default_value = "");
  void load_all_values_();
  std::string get_current_ssid_();

  std::string title_{"Device Setup"};
  std::string pin_;
  std::string logo_url_;
  std::string accent_color_{"#4a90d9"};
  std::vector<PortalField> fields_;
  std::map<std::string, std::string> values_;

  int dns_socket_{-1};
  bool needs_reboot_{false};
  uint32_t reboot_at_{0};
  volatile bool reload_values_{false};
  volatile bool scan_requested_{false};
  std::string scan_json_{"[]"};

#ifdef USE_ESP_IDF
  httpd_handle_t http_server_{nullptr};
#endif
};

}  // namespace setup_portal
}  // namespace esphome
