#include "setup_portal.h"
#include "portal_html.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <lwip/sockets.h>
#include <cstring>
#endif

#include "esphome/components/wifi/wifi_component.h"

namespace esphome {
namespace setup_portal {

static const char *const TAG = "setup_portal";
static const char *const NVS_NS = "setup_portal";
static const uint32_t REBOOT_DELAY_MS = 1500;

// ========== Helpers ==========

static std::string url_decode(const std::string &str) {
  std::string r;
  r.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '+') r += ' ';
    else if (str[i] == '%' && i + 2 < str.size()) {
      char h[3] = {str[i+1], str[i+2], 0};
      r += (char) strtol(h, nullptr, 16);
      i += 2;
    } else r += str[i];
  }
  return r;
}

static std::string get_param(const std::string &body, const std::string &key) {
  std::string s = key + "=";
  size_t p = body.find(s);
  while (p != std::string::npos) {
    if (p == 0 || body[p-1] == '&') break;
    p = body.find(s, p + 1);
  }
  if (p == std::string::npos) return "";
  p += s.size();
  size_t e = body.find('&', p);
  return url_decode(body.substr(p, e == std::string::npos ? body.size() - p : e - p));
}

static std::string html_esc(const std::string &str) {
  std::string r;
  r.reserve(str.size());
  for (char c : str) {
    switch (c) {
      case '&': r += "&amp;"; break;
      case '<': r += "&lt;"; break;
      case '>': r += "&gt;"; break;
      case '"': r += "&quot;"; break;
      case '\'': r += "&#39;"; break;
      default: r += c;
    }
  }
  return r;
}

#ifdef USE_ESP_IDF
// Small buffer for snprintf chunks — avoids heap allocations
static char buf_[2048];

static void chunk(httpd_req_t *r, const char *s) {
  httpd_resp_send_chunk(r, s, HTTPD_RESP_USE_STRLEN);
}

static void chunkf(httpd_req_t *r, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf_, sizeof(buf_), fmt, args);
  va_end(args);
  httpd_resp_send_chunk(r, buf_, HTTPD_RESP_USE_STRLEN);
}
#endif

// ========== Component ==========

void SetupPortalComponent::add_field(const std::string &id, const std::string &label,
                                      const std::string &type, const std::string &default_value,
                                      float min_val, float max_val, bool has_min, bool has_max) {
  PortalField f;
  f.id = id; f.label = label; f.type = type; f.default_value = default_value;
  f.min_value = min_val; f.max_value = max_val; f.has_min = has_min; f.has_max = has_max;
  this->fields_.push_back(f);
}

void SetupPortalComponent::setup() {
#ifdef USE_ESP_IDF
  ESP_LOGCONFIG(TAG, "Setting up Setup Portal...");
  nvs_flash_init();
  this->load_all_values_();

  // Load custom PIN from NVS
  std::string saved_pin = this->load_from_nvs_("_pin", "");
  if (!saved_pin.empty()) this->pin_ = saved_pin;

  // Apply saved WiFi credentials
  std::string ssid = this->load_from_nvs_("_wifi_ssid", "");
  std::string pass = this->load_from_nvs_("_wifi_pass", "");
  if (!ssid.empty()) {
    ESP_LOGI(TAG, "Applying saved WiFi: %s", ssid.c_str());
    wifi::WiFiAP sta;
    sta.set_ssid(ssid);
    sta.set_password(pass);
    wifi::global_wifi_component->set_sta(sta);
    wifi::global_wifi_component->start_scanning();
  }

  this->start_dns_server_();
  this->start_http_server_();
  this->scan_requested_ = true;

  ESP_LOGCONFIG(TAG, "Setup Portal ready (title=%s, pin=%s, fields=%d)",
                this->title_.c_str(), this->pin_.empty() ? "none" : "****", (int) this->fields_.size());
#endif
}

void SetupPortalComponent::loop() {
#ifdef USE_ESP_IDF
  this->handle_dns_();

  if (this->reload_values_) {
    this->load_all_values_();
    this->reload_values_ = false;
  }

  if (this->needs_reboot_ && millis() >= this->reboot_at_) {
    ESP_LOGI(TAG, "Rebooting...");
    App.safe_reboot();
  }

  // Keep AP alive (unless temporarily disabled for disconnect)
  if (this->ap_disabled_until_ && millis() >= this->ap_disabled_until_) {
    this->ap_disabled_until_ = 0;
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    ESP_LOGI(TAG, "AP re-enabled after disconnect");
  } else if (!this->ap_disabled_until_) {
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_STA) {
      esp_wifi_set_mode(WIFI_MODE_APSTA);
    }
  }

  // Scan in main loop
  if (this->scan_requested_) {
    this->scan_requested_ = false;
    this->do_scan_();
  }
#endif
}

void SetupPortalComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Setup Portal:");
  ESP_LOGCONFIG(TAG, "  Title: %s", this->title_.c_str());
  ESP_LOGCONFIG(TAG, "  PIN: %s", this->pin_.empty() ? "disabled" : "enabled");
  for (const auto &f : this->fields_) {
    ESP_LOGCONFIG(TAG, "  %s: %s", f.id.c_str(), this->get_string(f.id, f.default_value).c_str());
  }
}

// ========== Public API ==========

std::string SetupPortalComponent::get_string(const std::string &id, const std::string &def) {
  auto it = this->values_.find(id);
  return (it != this->values_.end() && !it->second.empty()) ? it->second : def;
}

int SetupPortalComponent::get_int(const std::string &id, int def) {
  std::string v = this->get_string(id, "");
  return v.empty() ? def : atoi(v.c_str());
}

float SetupPortalComponent::get_float(const std::string &id, float def) {
  std::string v = this->get_string(id, "");
  return v.empty() ? def : atof(v.c_str());
}

bool SetupPortalComponent::check_pin(const std::string &pin) {
  return this->pin_.empty() || pin == this->pin_;
}

void SetupPortalComponent::disconnect_ap(uint32_t duration_ms) {
#ifdef USE_ESP_IDF
  this->ap_disabled_until_ = millis() + duration_ms;
  esp_wifi_set_mode(WIFI_MODE_STA);
  ESP_LOGI(TAG, "AP temporarily disabled for %dms", (int) duration_ms);
#endif
}

// ========== NVS ==========

void SetupPortalComponent::save_to_nvs_(const std::string &key, const std::string &value) {
#ifdef USE_ESP_IDF
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
    nvs_set_str(h, key.c_str(), value.c_str());
    nvs_commit(h);
    nvs_close(h);
  }
#endif
}

std::string SetupPortalComponent::load_from_nvs_(const std::string &key, const std::string &def) {
#ifdef USE_ESP_IDF
  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return def;
  size_t sz = 0;
  if (nvs_get_str(h, key.c_str(), nullptr, &sz) != ESP_OK) { nvs_close(h); return def; }
  std::string v;
  v.resize(sz - 1);
  if (nvs_get_str(h, key.c_str(), &v[0], &sz) != ESP_OK) { nvs_close(h); return def; }
  nvs_close(h);
  return v;
#else
  return def;
#endif
}

void SetupPortalComponent::load_all_values_() {
  for (const auto &f : this->fields_)
    this->values_[f.id] = this->load_from_nvs_(f.id, f.default_value);
}

std::string SetupPortalComponent::get_current_ssid_() {
#ifdef USE_ESP_IDF
  wifi_ap_record_t info;
  if (esp_wifi_sta_get_ap_info(&info) == ESP_OK)
    return std::string(reinterpret_cast<char *>(info.ssid));
#endif
  return "";
}

// ========== DNS ==========

void SetupPortalComponent::start_dns_server_() {
#ifdef USE_ESP_IDF
  this->dns_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (this->dns_socket_ < 0) return;
  int fl = fcntl(this->dns_socket_, F_GETFL, 0);
  fcntl(this->dns_socket_, F_SETFL, fl | O_NONBLOCK);
  struct sockaddr_in a = {};
  a.sin_family = AF_INET;
  a.sin_port = htons(53);
  a.sin_addr.s_addr = INADDR_ANY;
  if (bind(this->dns_socket_, (struct sockaddr *) &a, sizeof(a)) < 0) {
    close(this->dns_socket_);
    this->dns_socket_ = -1;
    return;
  }
  ESP_LOGI(TAG, "DNS server started on port 53");
#endif
}

void SetupPortalComponent::handle_dns_() {
#ifdef USE_ESP_IDF
  if (this->dns_socket_ < 0) return;
  uint8_t buf[256];
  struct sockaddr_in cli = {};
  socklen_t cl = sizeof(cli);
  int len = recvfrom(this->dns_socket_, buf, sizeof(buf), 0, (struct sockaddr *) &cli, &cl);
  if (len <= 12) return;
  uint8_t resp[512];
  memcpy(resp, buf, len);
  resp[2] = 0x84; resp[3] = 0x00;
  resp[6] = 0x00; resp[7] = 0x01;
  int p = len;
  resp[p++]=0xC0; resp[p++]=0x0C;
  resp[p++]=0x00; resp[p++]=0x01;
  resp[p++]=0x00; resp[p++]=0x01;
  resp[p++]=0x00; resp[p++]=0x00; resp[p++]=0x00; resp[p++]=0x3C;
  resp[p++]=0x00; resp[p++]=0x04;
  resp[p++]=192; resp[p++]=168; resp[p++]=4; resp[p++]=1;
  sendto(this->dns_socket_, resp, p, 0, (struct sockaddr *) &cli, cl);
#endif
}

// ========== HTTP ==========

#ifdef USE_ESP_IDF

static esp_err_t h_root(httpd_req_t *r) {
  auto *c = static_cast<SetupPortalComponent *>(r->user_ctx);
  httpd_resp_set_type(r, "text/html; charset=UTF-8");
  c->serve_page(r);
  httpd_resp_send_chunk(r, NULL, 0);
  return ESP_OK;
}

static esp_err_t h_auth(httpd_req_t *r) {
  auto *c = static_cast<SetupPortalComponent *>(r->user_ctx);
  char q[64]={0}, pv[32]={0};
  httpd_req_get_url_query_str(r, q, sizeof(q));
  httpd_query_key_value(q, "pin", pv, sizeof(pv));
  bool ok = c->check_pin(url_decode(pv));
  httpd_resp_set_type(r, "application/json");
  const char *resp = ok ? "{\"ok\":true}" : "{\"ok\":false}";
  httpd_resp_send(r, resp, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t h_scan(httpd_req_t *r) {
  auto *c = static_cast<SetupPortalComponent *>(r->user_ctx);
  httpd_resp_set_type(r, "application/json");
  c->serve_scan(r);
  return ESP_OK;
}

static esp_err_t h_save(httpd_req_t *r) {
  auto *c = static_cast<SetupPortalComponent *>(r->user_ctx);
  int tl = r->content_len;
  if (tl > 1024) tl = 1024;
  std::string body;
  body.resize(tl);
  int rx = 0;
  while (rx < tl) { int n = httpd_req_recv(r, &body[rx], tl - rx); if (n <= 0) break; rx += n; }
  body.resize(rx);
  c->handle_save(body);
  httpd_resp_set_type(r, "text/html; charset=UTF-8");
  httpd_resp_send(r, HTML_SAVE_REBOOT, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t h_disconnect(httpd_req_t *r) {
  auto *c = static_cast<SetupPortalComponent *>(r->user_ctx);
  httpd_resp_set_type(r, "application/json");
  httpd_resp_send(r, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
  c->disconnect_ap();
  return ESP_OK;
}

static esp_err_t h_captive(httpd_req_t *r) {
  httpd_resp_set_status(r, "302 Found");
  httpd_resp_set_hdr(r, "Location", "http://192.168.4.1/");
  httpd_resp_send(r, NULL, 0);
  return ESP_OK;
}

#endif

void SetupPortalComponent::start_http_server_() {
#ifdef USE_ESP_IDF
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 80;
  cfg.max_uri_handlers = 10;
  cfg.stack_size = 8192;
  cfg.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&this->http_server_, &cfg) != ESP_OK) return;

  httpd_uri_t u1 = {.uri="/", .method=HTTP_GET, .handler=h_root, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u1);
  httpd_uri_t u2 = {.uri="/auth", .method=HTTP_GET, .handler=h_auth, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u2);
  httpd_uri_t u3 = {.uri="/scan", .method=HTTP_GET, .handler=h_scan, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u3);
  httpd_uri_t u4 = {.uri="/save", .method=HTTP_POST, .handler=h_save, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u4);
  httpd_uri_t u5d = {.uri="/disconnect", .method=HTTP_GET, .handler=h_disconnect, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u5d);
  httpd_uri_t u5 = {.uri="/generate_204", .method=HTTP_GET, .handler=h_captive, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u5);
  httpd_uri_t u6 = {.uri="/hotspot-detect.html", .method=HTTP_GET, .handler=h_captive, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u6);
  httpd_uri_t u7 = {.uri="/connecttest.txt", .method=HTTP_GET, .handler=h_captive, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u7);
  httpd_uri_t u8 = {.uri="/*", .method=HTTP_GET, .handler=h_captive, .user_ctx=this};
  httpd_register_uri_handler(this->http_server_, &u8);

  ESP_LOGI(TAG, "HTTP server started on port 80");
#endif
}

// ========== Serve Page (chunked — zero large allocations) ==========

#ifdef USE_ESP_IDF
void SetupPortalComponent::serve_page(httpd_req_t *req) {
  // Refresh values from NVS before serving (picks up changes from MQTT/external writes)
  this->load_all_values_();
  std::string title = html_esc(this->title_);
  std::string ssid = html_esc(this->get_current_ssid_());
  bool has_pin = !this->pin_.empty();
  const char *accent = this->accent_color_.c_str();

  // Darken accent for :active
  char accent_dk[8] = "#333";
  if (this->accent_color_.size() == 7) {
    int r = std::max(0, (int)strtol(this->accent_color_.substr(1,2).c_str(), nullptr, 16) - 25);
    int g = std::max(0, (int)strtol(this->accent_color_.substr(3,2).c_str(), nullptr, 16) - 25);
    int b = std::max(0, (int)strtol(this->accent_color_.substr(5,2).c_str(), nullptr, 16) - 25);
    snprintf(accent_dk, sizeof(accent_dk), "#%02x%02x%02x", r, g, b);
  }

  // CSS + head
  chunkf(req, HTML_CSS, title.c_str(), accent, accent, accent_dk);

  // Helper: send logo as separate chunks (handles any size, including large base64)
  auto send_logo = [&]() {
    if (!this->logo_url_.empty()) {
      chunk(req, "<div class=\"logo\"><img src=\"");
      chunk(req, this->logo_url_.c_str());
      chunk(req, "\" alt=\"\"></div>");
    }
  };

  // PIN screen
  if (has_pin) {
    chunk(req, "<div id=\"ps\">");
    send_logo();
    chunkf(req, "<h1>%s</h1><div class=\"b\">"
           "<label>PIN</label>"
           "<input type=\"password\" id=\"pin\" inputmode=\"numeric\" autocomplete=\"off\" autofocus>"
           "<button class=\"btn\" onclick=\"auth()\">Enter</button>"
           "</div></div>", title.c_str());
  }

  // Config screen
  const char *cs_style = has_pin ? "style=\"display:none\"" : "";
  chunkf(req, "<div id=\"cs\" %s>", cs_style);
  send_logo();
  // Title with lock icon for PIN change
  if (has_pin) {
    chunkf(req, "<div style=\"position:relative\">"
           "<h1>%s</h1>"
           "<a class=\"lnk\" onclick=\"document.getElementById('pc').classList.toggle('open')\" "
           "style=\"position:absolute;right:0;top:50%%;transform:translateY(-50%%);outline:none;-webkit-tap-highlight-color:transparent\">"
           "<svg width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"#bbb\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
           "<rect x=\"3\" y=\"11\" width=\"18\" height=\"11\" rx=\"2\" ry=\"2\"/><path d=\"M7 11V7a5 5 0 0110 0v4\"/></svg></a>"
           "</div>", title.c_str());
  } else {
    chunkf(req, "<h1>%s</h1>", title.c_str());
  }

  // Form starts here — PIN change and WiFi are both inside it
  chunk(req, "<form method=\"POST\" action=\"/save\">"
        "<input type=\"hidden\" name=\"pin\" id=\"fp\">");

  // PIN change expandable (between title and WiFi — where the lock icon is)
  if (has_pin) {
    chunk(req, "<div class=\"pin-c\" id=\"pc\"><div class=\"b\" style=\"margin-bottom:12px\"><h2>PIN</h2>"
          "<div class=\"pw\"><input type=\"password\" name=\"new_pin\" id=\"np\" placeholder=\"New PIN\" inputmode=\"numeric\">"
          "<button type=\"button\" class=\"eye\" onclick=\"tog('np',this)\">"
          "<svg width=\"18\" height=\"18\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\">"
          "<path d=\"M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z\"/><circle cx=\"12\" cy=\"12\" r=\"3\"/></svg>"
          "</button></div></div></div>");
  }

  const char *wifi_ph = ssid.empty() ? "" : "Leave empty to keep current";
  chunkf(req, HTML_CONFIG_WIFI, wifi_ph);

  // Custom fields
  if (!this->fields_.empty()) {
    chunk(req, "<div class=\"b\"><h2>Settings</h2>");
    for (const auto &f : this->fields_) {
      std::string val = html_esc(this->get_string(f.id, f.default_value));
      chunkf(req, "<label>%s</label>", f.label.c_str());
      if (f.type == "interval") {
        // Interval field: number + unit selector, value always in seconds
        int secs = atoi(val.c_str());
        int displayVal = secs;
        const char *selS = " selected", *selM = "", *selH = "";
        if (secs > 0 && secs % 3600 == 0) { displayVal = secs / 3600; selS = ""; selH = " selected"; }
        else if (secs > 0 && secs % 60 == 0) { displayVal = secs / 60; selS = ""; selM = " selected"; }
        int minSecs = f.has_min ? (int) f.min_value : 1;
        chunkf(req, "<div class=\"row\"><input type=\"number\" id=\"iv_%s\" value=\"%d\" min=\"1\" style=\"flex:1\">"
               "<select id=\"iu_%s\" style=\"width:auto;flex:0 0 auto\">"
               "<option value=\"1\"%s>sec</option><option value=\"60\"%s>min</option><option value=\"3600\"%s>hr</option>"
               "</select></div>"
               "<input type=\"hidden\" name=\"%s\" id=\"ivh_%s\" data-min=\"%d\">",
               f.id.c_str(), displayVal,
               f.id.c_str(), selS, selM, selH, f.id.c_str(), f.id.c_str(), minSecs);
      } else {
        chunkf(req, "<input type=\"%s\" name=\"%s\" value=\"%s\"",
               f.type.c_str(), f.id.c_str(), val.c_str());
        if (f.has_min) chunkf(req, " min=\"%d\"", (int) f.min_value);
        if (f.has_max) chunkf(req, " max=\"%d\"", (int) f.max_value);
        chunk(req, ">");
      }
    }
    chunk(req, "</div>");
  }

  // PIN change + save button + status
  std::string status = ssid.empty() ? "Not connected to WiFi" : "Connected to: " + ssid;
  chunkf(req, HTML_CONFIG_END, status.c_str());

  // Script
  chunkf(req, HTML_SCRIPT, has_pin ? 1 : 0, ssid.c_str(), accent);
}

void SetupPortalComponent::serve_scan(httpd_req_t *req) {
  this->scan_requested_ = true;
  httpd_resp_send(req, this->scan_json_.c_str(), this->scan_json_.length());
}
#endif

// ========== Scan ==========

void SetupPortalComponent::do_scan_() {
#ifdef USE_ESP_IDF
  wifi_mode_t mode;
  esp_wifi_get_mode(&mode);
  if (mode == WIFI_MODE_AP) esp_wifi_set_mode(WIFI_MODE_APSTA);

  wifi_scan_config_t sc = {};
  sc.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  sc.scan_time.active.min = 120;
  sc.scan_time.active.max = 500;

  if (esp_wifi_scan_start(&sc, true) != ESP_OK) { this->scan_json_ = "[]"; return; }

  uint16_t cnt = 0;
  esp_wifi_scan_get_ap_num(&cnt);
  if (cnt > 20) cnt = 20;
  std::vector<wifi_ap_record_t> recs(cnt);
  esp_wifi_scan_get_ap_records(&cnt, recs.data());

  std::map<std::string, int8_t> seen;
  for (const auto &r : recs) {
    std::string s(reinterpret_cast<const char*>(r.ssid));
    if (s.empty()) continue;
    auto it = seen.find(s);
    if (it == seen.end() || r.rssi > it->second) seen[s] = r.rssi;
  }

  std::vector<std::pair<std::string, int8_t>> sorted(seen.begin(), seen.end());
  std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b){ return a.second > b.second; });

  std::string j = "[";
  for (size_t i = 0; i < sorted.size(); i++) {
    if (i) j += ",";
    j += "{\"s\":\"" + html_esc(sorted[i].first) + "\",\"r\":" + std::to_string(sorted[i].second) + "}";
  }
  j += "]";
  this->scan_json_ = j;
  ESP_LOGD(TAG, "Scan: %d networks", (int) seen.size());
#endif
}

// ========== Save ==========

void SetupPortalComponent::handle_save(const std::string &body) {
  if (!this->check_pin(get_param(body, "pin"))) { ESP_LOGW(TAG, "Bad PIN"); return; }

  for (const auto &f : this->fields_) {
    std::string v = get_param(body, f.id);
    this->save_to_nvs_(f.id, v);
  }

  std::string new_pin = get_param(body, "new_pin");
  if (!new_pin.empty()) {
    this->save_to_nvs_("_pin", new_pin);
    ESP_LOGI(TAG, "PIN changed");
  }

  std::string new_ssid = get_param(body, "ssid");
  std::string new_pass = get_param(body, "password");
  std::string saved = this->load_from_nvs_("_wifi_ssid", "");

  if (!new_ssid.empty() && (new_ssid != saved || !new_pass.empty())) {
    this->save_to_nvs_("_wifi_ssid", new_ssid);
    this->save_to_nvs_("_wifi_pass", new_pass);
  }

  // Always reboot to apply all changes cleanly
  this->needs_reboot_ = true;
  this->reboot_at_ = millis() + REBOOT_DELAY_MS;
  ESP_LOGI(TAG, "Settings saved, rebooting...");
}

}  // namespace setup_portal
}  // namespace esphome
