#include "sensors_max30102.h"

#include <Arduino.h>
#include <string.h>

#include "max30102_algorithm.h"

namespace heatrisk {

namespace {

constexpr uint32_t kDebugLogIntervalMs = 1000;

}  // namespace

bool Max30102Sensor::begin(TwoWire* wire, uint8_t led_brightness,
                           uint8_t sample_average, uint8_t led_mode,
                           uint16_t sample_rate, uint16_t pulse_width,
                           uint16_t adc_range, uint8_t pulse_amplitude_red,
                           uint8_t pulse_amplitude_ir) {
  if (wire == nullptr) {
    return false;
  }

  if (!sensor_.begin(*wire, I2C_SPEED_STANDARD)) {
    initialized_ = false;
    healthy_ = false;
    return false;
  }

  sensor_.setup(led_brightness, sample_average, led_mode, sample_rate,
                pulse_width, adc_range);
  sensor_.setPulseAmplitudeRed(pulse_amplitude_red);
  sensor_.setPulseAmplitudeIR(pulse_amplitude_ir);

  initialized_ = true;
  healthy_ = true;
  return true;
}

void Max30102Sensor::PushSample(uint32_t red, uint32_t ir) {
  if (valid_sample_count_ < kWindowSize) {
    red_window_[valid_sample_count_] = red;
    ir_window_[valid_sample_count_] = ir;
    ++valid_sample_count_;
  } else {
    memmove(red_window_, red_window_ + 1,
            sizeof(red_window_[0]) * (kWindowSize - 1));
    memmove(ir_window_, ir_window_ + 1,
            sizeof(ir_window_[0]) * (kWindowSize - 1));
    red_window_[kWindowSize - 1] = red;
    ir_window_[kWindowSize - 1] = ir;
  }
  ++samples_since_eval_;
}

bool Max30102Sensor::HaveWindowReady() const {
  return valid_sample_count_ >= kWindowSize &&
         samples_since_eval_ >= kEvalStepSamples;
}

void Max30102Sensor::ComputeWindowMetrics() {
  Max30102AlgoResult result;
  if (!ComputeMax30102Metrics(ir_window_, red_window_, kWindowSize, &result)) {
    return;
  }

  if (result.heart_rate_valid &&
      result.heart_rate_bpm >= 35.0f &&
      result.heart_rate_bpm <= 220.0f) {
    last_heart_rate_bpm_ =
        has_hr_ ? (last_heart_rate_bpm_ * 0.6f + result.heart_rate_bpm * 0.4f)
                : result.heart_rate_bpm;
    has_hr_ = true;
  }

  if (result.spo2_valid &&
      result.spo2_pct >= 70.0f &&
      result.spo2_pct <= 100.0f) {
    last_spo2_pct_ =
        has_spo2_ ? (last_spo2_pct_ * 0.6f + result.spo2_pct * 0.4f)
                  : result.spo2_pct;
    has_spo2_ = true;
  }

  samples_since_eval_ = 0;
}

bool Max30102Sensor::sample(PhysiologyData* inout_physiology) {
  if (!initialized_ || inout_physiology == nullptr) {
    healthy_ = false;
    return false;
  }

  sensor_.check();

  bool has_sample = false;
  bool contact_ok = false;
  while (sensor_.available()) {
    const uint32_t red = sensor_.getFIFORed();
    const uint32_t ir = sensor_.getFIFOIR();
    sensor_.nextSample();
    has_sample = true;
    last_red_ = red;
    last_ir_ = ir;

    const bool sample_contact_ok = (ir > 5000) && (red > 5000);
    if (!sample_contact_ok) {
      continue;
    }
    contact_ok = true;
    PushSample(red, ir);
  }

  if (HaveWindowReady()) {
    ComputeWindowMetrics();
  }

  if (has_hr_) {
    inout_physiology->heart_rate_bpm = last_heart_rate_bpm_;
  }
  if (has_spo2_) {
    inout_physiology->spo2_pct = last_spo2_pct_;
  }

  healthy_ = contact_ok && has_hr_ && has_spo2_;

  const uint32_t now_ms = millis();
  if ((now_ms - last_debug_ms_) >= kDebugLogIntervalMs) {
    last_debug_ms_ = now_ms;
    Serial.printf(
        "[C3][MAX30102] sample=%d contact=%d red=%lu ir=%lu hr=%.1f spo2=%.1f has_hr=%d has_spo2=%d healthy=%d window=%u pending=%u\n",
        has_sample, contact_ok, static_cast<unsigned long>(last_red_),
        static_cast<unsigned long>(last_ir_), last_heart_rate_bpm_,
        last_spo2_pct_, has_hr_, has_spo2_, healthy_, valid_sample_count_,
        samples_since_eval_);
  }

  return healthy_;
}

bool Max30102Sensor::isHealthy() const { return healthy_; }

}  // namespace heatrisk
