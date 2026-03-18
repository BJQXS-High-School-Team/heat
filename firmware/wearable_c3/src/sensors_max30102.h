#pragma once

#include <MAX30105.h>
#include <Wire.h>

#include "data_models.h"

namespace heatrisk {

class Max30102Sensor {
 public:
  bool begin(TwoWire* wire = &Wire);
  bool sample(PhysiologyData* inout_physiology);
  bool isHealthy() const;

 private:
  float EstimateSpo2Trend();
  bool PushSample(uint32_t red, uint32_t ir);

  MAX30105 sensor_;
  bool initialized_ = false;
  bool healthy_ = false;
  uint32_t last_beat_ms_ = 0;
  float last_heart_rate_bpm_ = 0.0f;
  float last_spo2_pct_ = 0.0f;
  bool has_hr_ = false;
  bool has_spo2_ = false;

  static constexpr uint8_t kWindowSize = 32;
  uint32_t red_window_[kWindowSize] = {0};
  uint32_t ir_window_[kWindowSize] = {0};
  uint8_t window_count_ = 0;
  uint8_t window_index_ = 0;
};

}  // namespace heatrisk
