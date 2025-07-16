#include "gps.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace gps {

static const char *const TAG = "gps";

bool starts_with(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), str.begin());
}

void GPS::dump_config() {
  ESP_LOGCONFIG(TAG, "GPS:");
  LOG_SENSOR("  ", "Latitude", latitude_sensor_);
  LOG_SENSOR("  ", "Longitude", longitude_sensor_);
  LOG_SENSOR("  ", "Speed", speed_sensor_);
  LOG_SENSOR("  ", "Course", course_sensor_);
  LOG_SENSOR("  ", "Altitude", altitude_sensor_);
  LOG_SENSOR("  ", "Satellites", satellites_sensor_);
  LOG_SENSOR("  ", "HDOP", hdop_sensor_);
}

void GPS::update() {
  if (latitude_sensor_ != nullptr)
    latitude_sensor_->publish_state(latitude_);

  if (longitude_sensor_ != nullptr)
    longitude_sensor_->publish_state(longitude_);

  if (speed_sensor_ != nullptr)
    speed_sensor_->publish_state(speed_);

  if (course_sensor_ != nullptr)
    course_sensor_->publish_state(course_);

  if (altitude_sensor_ != nullptr)
    altitude_sensor_->publish_state(altitude_);

  if (satellites_sensor_ != nullptr)
    satellites_sensor_->publish_state(satellites_);

  if (hdop_sensor_ != nullptr)
    hdop_sensor_->publish_state(hdop_);
}

void GPS::loop() {
  while (available()) {
    char c = read();
    if (c == '\n') {
      this->parse_nmea_sentence_(rx_buffer_);
      rx_buffer_.clear();
    } else {
      rx_buffer_ += c;
    }
  }
}

void GPS::parse_nmea_sentence_(const std::string &line) {
  if (!starts_with(line, "$"))
    return;

  std::vector<std::string> tokens;
  size_t start = 0, end;
  while ((end = line.find(',', start)) != std::string::npos) {
    tokens.push_back(line.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(line.substr(start));  // Último token

  if (tokens[0] == "$GPGGA" || tokens[0] == "$GNGGA") {
    if (tokens.size() >= 10) {
      if (!tokens[6].empty() && tokens[6] != "0") {  // fix quality
        if (!tokens[2].empty() && !tokens[3].empty())
          latitude_ = parse_coordinate(tokens[2], tokens[3]);
        if (!tokens[4].empty() && !tokens[5].empty())
          longitude_ = parse_coordinate(tokens[4], tokens[5]);
        if (!tokens[7].empty())
          satellites_ = std::stoi(tokens[7]);
        if (!tokens[8].empty())
          hdop_ = std::stof(tokens[8]);
        if (!tokens[9].empty())
          altitude_ = std::stof(tokens[9]);

        ESP_LOGV(TAG, "GGA → Lat: %.6f, Lon: %.6f, Alt: %.2f m, Sats: %d, HDOP: %.2f",
                 latitude_, longitude_, altitude_, satellites_, hdop_);
      } else {
        ESP_LOGV(TAG, "GGA → No fix (fix quality = 0)");
      }
    } else {
      ESP_LOGW(TAG, "GGA → Not enough tokens (%zu)", tokens.size());
    }
  } else if (tokens[0] == "$GPVTG" || tokens[0] == "$GNVTG") {
    if (tokens.size() >= 8) {
      if (!tokens[1].empty())
        course_ = std::stof(tokens[1]);
      if (!tokens[7].empty())
        speed_ = std::stof(tokens[7]);
      ESP_LOGV(TAG, "VTG → Course: %.2f°, Speed: %.2f km/h", course_, speed_);
    } else {
      ESP_LOGW(TAG, "VTG → Not enough tokens (%zu)", tokens.size());
    }
  } else if (tokens[0] == "$GPRMC" || tokens[0] == "$GNRMC") {
    // Opcional: puedes añadir parseo RMC aquí
  }
}
float GPS::parse_coordinate(const std::string &value, const std::string &hemisphere) {
  if (value.empty() || hemisphere.empty())
    return NAN;

  float raw = std::stof(value);
  float deg = std::floor(raw / 100.0f);
  float min = raw - (deg * 100.0f);
  float decimal = deg + (min / 60.0f);

  if (hemisphere == "S" || hemisphere == "W")
    decimal *= -1;

  return decimal;
}


}  // namespace gps
}  // namespace esphome
