#pragma once

#include <MAX30105.h>
#include <Wire.h>

#include "data_models.h"

namespace heatrisk {

class Max30102Sensor {
 public:
  bool begin(TwoWire* wire = &Wire, uint8_t led_brightness = 18,
             uint8_t sample_average = 4, uint8_t led_mode = 2,
             uint16_t sample_rate = 50, uint16_t pulse_width = 215,
             uint16_t adc_range = 2048, uint8_t pulse_amplitude_red = 0x10,
             uint8_t pulse_amplitude_ir = 0x10);
  bool sample(PhysiologyData* inout_physiology);
  bool isHealthy() const;

 private:
  void PushSample(uint32_t red, uint32_t ir);
  bool HaveWindowReady() const;
  void ComputeWindowMetrics();

  MAX30105 sensor_;
  bool initialized_ = false;
  bool healthy_ = false;
  float last_heart_rate_bpm_ = 0.0f;
  float last_spo2_pct_ = 0.0f;
  bool has_hr_ = false;
  bool has_spo2_ = false;
  uint32_t last_debug_ms_ = 0;
  uint32_t last_red_ = 0;
  uint32_t last_ir_ = 0;
  uint16_t valid_sample_count_ = 0;
  uint16_t samples_since_eval_ = 0;

  static constexpr uint16_t kWindowSize = 500;
  static constexpr uint16_t kEvalStepSamples = 100;
  uint32_t red_window_[kWindowSize] = {0};
  uint32_t ir_window_[kWindowSize] = {0};
};

}  // namespace heatrisk
