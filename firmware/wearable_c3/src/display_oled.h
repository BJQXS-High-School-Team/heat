#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "data_models.h"

namespace heatrisk {

struct OledViewModel {
  float heart_rate_bpm = 0.0f;
  float spo2_pct = 0.0f;
  float body_temp_c = 0.0f;
  float activity_score = 0.0f;
  bool gps_has_fix = false;
  uint8_t gps_satellites = 0;
  StateLevel local_state = StateLevel::kNormal;
  StateLevel effective_state = StateLevel::kNormal;
  bool alarm_active = false;
  bool wifi_connected = false;
  bool override_active = false;
};

class OledDisplay {
 public:
  OledDisplay(uint8_t width, uint8_t height, int8_t reset_pin);

  bool begin(uint8_t i2c_addr);
  void render(const OledViewModel& vm);
  bool isHealthy() const;

 private:
  Adafruit_SSD1306 display_;
  bool healthy_ = false;
};

}  // namespace heatrisk
