#include "sd_card_spi.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <sys/stat.h>

#include "ff.h"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sd_card_spi {

static const char *const TAG = "sd_card_spi";
static const char *const MOUNT_POINT = "/sd";

static sdmmc_card_t *s_card = nullptr;
static bool s_bus_initialized = false;

void SDCardSPIComponent::set_root_path(const std::string &root_path) {
  if (root_path.empty()) {
    this->root_path_ = "/";
    return;
  }
  this->root_path_ = (root_path[0] == '/') ? root_path : "/" + root_path;
}

void SDCardSPIComponent::setup() {
  this->try_mount_();
}

void SDCardSPIComponent::loop() {
  if (this->mounted_) {
    return;
  }
  const uint32_t now = millis();
  if (now - this->last_mount_retry_ms_ < 5000) {
    return;
  }
  this->last_mount_retry_ms_ = now;
  this->try_mount_();
}

bool SDCardSPIComponent::try_mount_() {
  if (this->cs_pin_ < 0 || this->sck_pin_ < 0 || this->miso_pin_ < 0 || this->mosi_pin_ < 0) {
    ESP_LOGE(TAG, "Invalid pin config");
    this->mounted_ = false;
    return false;
  }

  // Clean up any previous session
  if (s_card != nullptr) {
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
    s_card = nullptr;
  }
  if (s_bus_initialized) {
    spi_bus_free(SPI2_HOST);
    s_bus_initialized = false;
  }

  // Initialize SPI bus with DMA
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = this->mosi_pin_;
  bus_cfg.miso_io_num = this->miso_pin_;
  bus_cfg.sclk_io_num = this->sck_pin_;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = 4096;

  esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret == ESP_OK) {
    s_bus_initialized = true;
  } else if (ret == ESP_ERR_INVALID_STATE) {
    // SPI host already initialized by another component (for example ethernet).
    // Reuse existing bus instead of failing.
    ESP_LOGW(TAG, "SPI bus already initialized; reusing existing host");
  } else {
    ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
    this->mounted_ = false;
    return false;
  }

  // Configure SD SPI device
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = static_cast<gpio_num_t>(this->cs_pin_);
  slot_config.host_id = SPI2_HOST;

  // Configure SD host
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI2_HOST;
  host.max_freq_khz = this->frequency_ / 1000;

  // Mount FAT filesystem
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 5;
  mount_config.allocation_unit_size = 16 * 1024;

  ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "SD mount failed: %s (0x%x)", esp_err_to_name(ret), ret);
    s_card = nullptr;
    if (s_bus_initialized) {
      spi_bus_free(SPI2_HOST);
      s_bus_initialized = false;
    }
    this->mounted_ = false;
    return false;
  }

  this->mounted_ = true;
  ESP_LOGI(TAG, "SD card mounted (%.0f MB)",
           (double) ((uint64_t) s_card->csd.capacity * s_card->csd.sector_size) / (1024.0 * 1024.0));
  return true;
}

void SDCardSPIComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SD Card SPI:");
  ESP_LOGCONFIG(TAG, "  Mounted: %s", YESNO(this->mounted_));
  ESP_LOGCONFIG(TAG, "  Root path: %s", this->root_path_.c_str());
  ESP_LOGCONFIG(TAG, "  CS Pin: %d", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "  SPI Pins: SCK=%d MISO=%d MOSI=%d", this->sck_pin_, this->miso_pin_, this->mosi_pin_);
  ESP_LOGCONFIG(TAG, "  SPI Frequency: %u Hz", (unsigned int) this->frequency_);
  if (this->mounted_ && s_card != nullptr) {
    const uint64_t size_mb =
        (uint64_t) s_card->csd.capacity * s_card->csd.sector_size / (1024 * 1024);
    ESP_LOGCONFIG(TAG, "  Size: %llu MB", size_mb);
  }
}

bool SDCardSPIComponent::write_file(const std::string &path, const std::string &data, bool append) {
  if (!this->mounted_ && !this->try_mount_()) {
    ESP_LOGW(TAG, "Skipping write; SD not mounted");
    return false;
  }

  const std::string vfs_path = std::string(MOUNT_POINT) + this->resolve_path_(path);

  FILE *f = fopen(vfs_path.c_str(), append ? "a" : "w");
  if (f == nullptr) {
    ESP_LOGE(TAG, "Failed to open '%s' for write (errno=%d)", vfs_path.c_str(), errno);
    return false;
  }

  const size_t written = fwrite(data.c_str(), 1, data.size(), f);
  fclose(f);

  if (written != data.size()) {
    ESP_LOGE(TAG, "Partial write on '%s' (%u/%u bytes)", vfs_path.c_str(), (unsigned) written, (unsigned) data.size());
    return false;
  }

  ESP_LOGD(TAG, "%s %u bytes to %s", append ? "Appended" : "Wrote", (unsigned) written, vfs_path.c_str());
  return true;
}

bool SDCardSPIComponent::delete_file(const std::string &path) {
  if (!this->mounted_ && !this->try_mount_()) {
    ESP_LOGW(TAG, "Skipping delete; SD not mounted");
    return false;
  }

  // Use FatFs API directly — POSIX remove() returns ENOSYS on some ESP-IDF builds
  std::string resolved = this->resolve_path_(path);
  std::string ff_path;
  if (!resolved.empty() && resolved[0] == '/') {
    ff_path = resolved.substr(1);
  } else {
    ff_path = resolved;
  }

  FRESULT res = f_unlink(ff_path.c_str());
  if (res != FR_OK) {
    ESP_LOGE(TAG, "Failed to delete '%s' (FRESULT=%d)", ff_path.c_str(), (int) res);
    return false;
  }

  ESP_LOGD(TAG, "Deleted %s", ff_path.c_str());
  return true;
}

bool SDCardSPIComponent::read_file(const std::string &path, size_t max_bytes, std::string &out) {
  out.clear();
  if (!this->mounted_ && !this->try_mount_()) {
    ESP_LOGW(TAG, "Skipping read; SD not mounted");
    return false;
  }

  const std::string vfs_path = std::string(MOUNT_POINT) + this->resolve_path_(path);

  FILE *f = fopen(vfs_path.c_str(), "r");
  if (f == nullptr) {
    ESP_LOGE(TAG, "Failed to open '%s' for read (errno=%d)", vfs_path.c_str(), errno);
    return false;
  }

  out.resize(max_bytes);
  const size_t read_bytes = fread(&out[0], 1, max_bytes, f);
  fclose(f);
  out.resize(read_bytes);

  ESP_LOGI(TAG, "Read %u bytes from %s", (unsigned) read_bytes, vfs_path.c_str());
  if (!out.empty()) {
    ESP_LOGD(TAG, "Content: %s", out.c_str());
  }
  return true;
}

bool SDCardSPIComponent::exists(const std::string &path) {
  if (!this->mounted_) {
    return false;
  }
  const std::string vfs_path = std::string(MOUNT_POINT) + this->resolve_path_(path);
  struct stat st;
  return stat(vfs_path.c_str(), &st) == 0;
}

std::string SDCardSPIComponent::resolve_path_(const std::string &path) const {
  if (path.empty()) {
    return "";
  }
  if (path[0] == '/') {
    return path;
  }
  if (this->root_path_.empty() || this->root_path_ == "/") {
    return "/" + path;
  }
  if (this->root_path_.back() == '/') {
    return this->root_path_ + path;
  }
  return this->root_path_ + "/" + path;
}

std::vector<std::string> SDCardSPIComponent::list_directory(const std::string &path, const std::string &suffix) {
  std::vector<std::string> result;
  if (!this->mounted_ && !this->try_mount_()) {
    ESP_LOGW(TAG, "Skipping list; SD not mounted");
    return result;
  }

  // Use FatFs API directly to avoid POSIX stub linker warnings.
  // FatFs uses drive-relative paths: "" or "." for root, not "/".
  FF_DIR dir;
  FILINFO info;
  std::string resolved = this->resolve_path_(path);
  // Strip leading slash — FatFs root is "" not "/"
  std::string ff_path;
  if (resolved == "/") {
    ff_path = "";
  } else if (!resolved.empty() && resolved[0] == '/') {
    ff_path = resolved.substr(1);
  } else {
    ff_path = resolved;
  }

  FRESULT res = f_opendir(&dir, ff_path.c_str());
  if (res != FR_OK) {
    ESP_LOGE(TAG, "Failed to open directory '%s' (FRESULT=%d)", ff_path.c_str(), (int) res);
    return result;
  }

  while (true) {
    res = f_readdir(&dir, &info);
    if (res != FR_OK || info.fname[0] == '\0') break;
    if (info.fattrib & AM_DIR) continue;
    std::string name(info.fname);
    if (!suffix.empty()) {
      if (name.size() < suffix.size()) continue;
      if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
    }
    result.push_back(name);
  }
  f_closedir(&dir);

  std::sort(result.begin(), result.end());
  ESP_LOGD(TAG, "Listed %u files in %s", (unsigned) result.size(), ff_path.c_str());
  return result;
}

}  // namespace sd_card_spi
}  // namespace esphome
