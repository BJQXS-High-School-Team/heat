#pragma once

#include <Arduino.h>
#include <TinyGPS++.h>

#include "data_models.h"

namespace heatrisk {

struct GpsDebugInfo {
  bool stream_ok = false;
  bool has_fix = false;
  uint8_t satellites = 0;
  uint32_t bytes_read = 0;
  uint32_t sentences = 0;
  uint32_t last_sentence_age_ms = 0;
  double lat = 0.0;
  double lng = 0.0;
};

class GpsSensor {
 public:
  GpsSensor(HardwareSerial& serial, int8_t rx_pin, int8_t tx_pin, uint32_t baud);

  bool begin();
  void poll();
  bool sample(GpsData* out_gps);
  bool isHealthy() const;
  void getDebugInfo(uint32_t now_ms, GpsDebugInfo* out_info) const;

 private:
  HardwareSerial& serial_;
  TinyGPSPlus gps_;
  int8_t rx_pin_;
  int8_t tx_pin_;
  uint32_t baud_;
  bool healthy_ = false;
  uint32_t last_sentence_ms_ = 0;
  uint32_t bytes_read_ = 0;
  uint32_t sentences_ = 0;
  bool last_has_fix_ = false;
  uint8_t last_satellites_ = 0;
  GpsData last_good_;
};

}  // namespace heatrisk
