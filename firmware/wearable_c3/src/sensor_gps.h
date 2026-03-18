#pragma once

#include <Arduino.h>
#include <TinyGPS++.h>

#include "data_models.h"

namespace heatrisk {

class GpsSensor {
 public:
  GpsSensor(HardwareSerial& serial, int8_t rx_pin, int8_t tx_pin, uint32_t baud);

  bool begin();
  void poll();
  bool sample(GpsData* out_gps);
  bool isHealthy() const;

 private:
  HardwareSerial& serial_;
  TinyGPSPlus gps_;
  int8_t rx_pin_;
  int8_t tx_pin_;
  uint32_t baud_;
  bool healthy_ = false;
  uint32_t last_sentence_ms_ = 0;
  GpsData last_good_;
};

}  // namespace heatrisk
