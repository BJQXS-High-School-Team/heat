#include "sensors_max30102.h"

#include <heartRate.h>
#include <math.h>

namespace heatrisk {

namespace {

float Clamp(float value, float low, float high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

}  // namespace

bool Max30102Sensor::begin(TwoWire* wire) {
  if (wire == nullptr) {
    return false;
  }

  if (!sensor_.begin(*wire, I2C_SPEED_FAST)) {
    initialized_ = false;
    healthy_ = false;
    return false;
  }

  // 采用库推荐参数，输出趋势数据用于预警，不作为医疗诊断。
  sensor_.setup(/*ledBrightness=*/60, /*sampleAverage=*/4, /*ledMode=*/2,
                /*sampleRate=*/100, /*pulseWidth=*/411, /*adcRange=*/4096);
  sensor_.setPulseAmplitudeRed(0x1F);
  sensor_.setPulseAmplitudeIR(0x1F);

  initialized_ = true;
  healthy_ = true;
  return true;
}

bool Max30102Sensor::PushSample(uint32_t red, uint32_t ir) {
  if (red == 0 || ir == 0) {
    return false;
  }
  red_window_[window_index_] = red;
  ir_window_[window_index_] = ir;
  window_index_ = (window_index_ + 1) % kWindowSize;
  if (window_count_ < kWindowSize) {
    ++window_count_;
  }
  return true;
}

float Max30102Sensor::EstimateSpo2Trend() {
  if (window_count_ < 12) {
    return NAN;
  }

  float red_sum = 0.0f;
  float ir_sum = 0.0f;
  for (uint8_t i = 0; i < window_count_; ++i) {
    red_sum += static_cast<float>(red_window_[i]);
    ir_sum += static_cast<float>(ir_window_[i]);
  }
  const float red_dc = red_sum / window_count_;
  const float ir_dc = ir_sum / window_count_;
  if (red_dc <= 1.0f || ir_dc <= 1.0f) {
    return NAN;
  }

  float red_var = 0.0f;
  float ir_var = 0.0f;
  for (uint8_t i = 0; i < window_count_; ++i) {
    const float red_delta = static_cast<float>(red_window_[i]) - red_dc;
    const float ir_delta = static_cast<float>(ir_window_[i]) - ir_dc;
    red_var += red_delta * red_delta;
    ir_var += ir_delta * ir_delta;
  }
  const float red_ac = sqrtf(red_var / window_count_);
  const float ir_ac = sqrtf(ir_var / window_count_);
  if (red_ac <= 0.1f || ir_ac <= 0.1f) {
    return NAN;
  }

  const float ratio = (red_ac / red_dc) / (ir_ac / ir_dc);
  const float spo2 = 110.0f - 25.0f * ratio;
  return Clamp(spo2, 70.0f, 100.0f);
}

bool Max30102Sensor::sample(PhysiologyData* inout_physiology) {
  if (!initialized_ || inout_physiology == nullptr) {
    healthy_ = false;
    return false;
  }

  const uint32_t ir = sensor_.getIR();
  const uint32_t red = sensor_.getRed();
  const bool contact_ok = (ir > 5000) && (red > 5000);

  if (!contact_ok) {
    healthy_ = false;
    if (has_hr_) {
      inout_physiology->heart_rate_bpm = last_heart_rate_bpm_;
    }
    if (has_spo2_) {
      inout_physiology->spo2_pct = last_spo2_pct_;
    }
    return false;
  }

  PushSample(red, ir);

  if (checkForBeat(ir)) {
    const uint32_t now = millis();
    if (last_beat_ms_ > 0) {
      const float delta_s = (now - last_beat_ms_) / 1000.0f;
      if (delta_s > 0.01f) {
        const float bpm = 60.0f / delta_s;
        if (bpm > 35.0f && bpm < 220.0f) {
          last_heart_rate_bpm_ =
              has_hr_ ? (last_heart_rate_bpm_ * 0.7f + bpm * 0.3f) : bpm;
          has_hr_ = true;
        }
      }
    }
    last_beat_ms_ = now;
  }

  const float spo2_est = EstimateSpo2Trend();
  if (!isnan(spo2_est) && spo2_est >= 70.0f && spo2_est <= 100.0f) {
    last_spo2_pct_ =
        has_spo2_ ? (last_spo2_pct_ * 0.7f + spo2_est * 0.3f) : spo2_est;
    has_spo2_ = true;
  }

  if (has_hr_) {
    inout_physiology->heart_rate_bpm = last_heart_rate_bpm_;
  }
  if (has_spo2_) {
    inout_physiology->spo2_pct = last_spo2_pct_;
  }

  // 仅当心率和血氧趋势值都可用时，标记传感器健康。
  healthy_ = has_hr_ && has_spo2_;
  return healthy_;
}

bool Max30102Sensor::isHealthy() const { return healthy_; }

}  // namespace heatrisk
