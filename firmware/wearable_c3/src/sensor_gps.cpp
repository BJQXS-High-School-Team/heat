#include "sensor_gps.h"

namespace heatrisk {

GpsSensor::GpsSensor(HardwareSerial& serial, int8_t rx_pin, int8_t tx_pin,
                     uint32_t baud)
    : serial_(serial), rx_pin_(rx_pin), tx_pin_(tx_pin), baud_(baud) {}

bool GpsSensor::begin() {
  serial_.begin(baud_, SERIAL_8N1, rx_pin_, tx_pin_);
  return true;
}

void GpsSensor::poll() {
  while (serial_.available() > 0) {
    const char c = static_cast<char>(serial_.read());
    ++bytes_read_;
    if (gps_.encode(c)) {
      last_sentence_ms_ = millis();
      ++sentences_;
    }
  }
}

bool GpsSensor::sample(GpsData* out_gps) {
  if (out_gps == nullptr) {
    return false;
  }

  GpsData current = last_good_;
  current.has_fix = gps_.location.isValid();
  current.satellites = gps_.satellites.isValid()
                           ? static_cast<uint8_t>(gps_.satellites.value())
                           : 0;
  last_has_fix_ = current.has_fix;
  last_satellites_ = current.satellites;

  if (gps_.location.isValid()) {
    current.lat = gps_.location.lat();
    current.lng = gps_.location.lng();
    last_good_ = current;
  }

  const uint32_t now = millis();
  const bool stream_ok = (now - last_sentence_ms_) <= 5000;
  healthy_ = stream_ok;

  *out_gps = current;
  return current.has_fix;
}

bool GpsSensor::isHealthy() const { return healthy_; }

void GpsSensor::getDebugInfo(uint32_t now_ms, GpsDebugInfo* out_info) const {
  if (out_info == nullptr) {
    return;
  }

  out_info->stream_ok =
      (last_sentence_ms_ != 0) && ((now_ms - last_sentence_ms_) <= 5000);
  out_info->has_fix = last_has_fix_;
  out_info->satellites = last_satellites_;
  out_info->bytes_read = bytes_read_;
  out_info->sentences = sentences_;
  out_info->last_sentence_age_ms =
      (last_sentence_ms_ == 0) ? 0xFFFFFFFFUL : (now_ms - last_sentence_ms_);
  out_info->lat = last_good_.lat;
  out_info->lng = last_good_.lng;
}

}  // namespace heatrisk
