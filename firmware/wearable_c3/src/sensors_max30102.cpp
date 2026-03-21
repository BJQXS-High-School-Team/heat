#include "sensors_max30102.h"

#include <Arduino.h>
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

  // 优先稳态，降低 I2C 访问压力和 LED 电流脉冲。
  sensor_.setup(led_brightness, sample_average, led_mode, sample_rate,
                pulse_width, adc_range);
  sensor_.setPulseAmplitudeRed(pulse_amplitude_red);
  sensor_.setPulseAmplitudeIR(pulse_amplitude_ir);

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

  // 避免调用 getIR()/getRed() 的 safeCheck(250) 阻塞路径。
  // 这里改为一次非阻塞 FIFO 轮询，只消化当前已经到达的样本，
  // 防止 MAX30102 在 I2C 异常或无新样本时长时间占住主循环。
  sensor_.check();

  uint32_t red = 0;
  uint32_t ir = 0;
  bool has_sample = false;
  bool contact_ok = false;
  bool beat_detected = false;
  float bpm_candidate = 0.0f;
  bool should_log = false;
  while (sensor_.available()) {
    red = sensor_.getFIFORed();
    ir = sensor_.getFIFOIR();
    sensor_.nextSample();
    has_sample = true;

    const bool sample_contact_ok = (ir > 5000) && (red > 5000);
    if (!sample_contact_ok) {
      continue;
    }
    contact_ok = true;

    PushSample(red, ir);

    // 对 FIFO 中的每个有效样本都做一次心跳检测，避免丢样本导致 HR 一直为 0。
    if (checkForBeat(ir)) {
      beat_detected = true;
      const uint32_t now = millis();
      if (last_beat_ms_ > 0) {
        const float delta_s = (now - last_beat_ms_) / 1000.0f;
        if (delta_s > 0.25f) {
          const float bpm = 60.0f / delta_s;
          bpm_candidate = bpm;
          if (bpm > 35.0f && bpm < 220.0f) {
            last_heart_rate_bpm_ =
                has_hr_ ? (last_heart_rate_bpm_ * 0.7f + bpm * 0.3f) : bpm;
            has_hr_ = true;
          }
        }
      }
      last_beat_ms_ = now;
    }
  }

  const uint32_t now_ms = millis();
  if ((now_ms - last_debug_ms_) >= kDebugLogIntervalMs) {
    last_debug_ms_ = now_ms;
    should_log = true;
  }

  if (!has_sample) {
    healthy_ = has_hr_ && has_spo2_;
    if (has_hr_) {
      inout_physiology->heart_rate_bpm = last_heart_rate_bpm_;
    }
    if (has_spo2_) {
      inout_physiology->spo2_pct = last_spo2_pct_;
    }
    if (should_log) {
      Serial.printf(
          "[C3][MAX30102] sample=0 contact=0 beat=0 red=%lu ir=%lu bpm_raw=%.1f hr=%.1f spo2=%.1f has_hr=%d has_spo2=%d healthy=%d\n",
          static_cast<unsigned long>(red), static_cast<unsigned long>(ir),
          bpm_candidate, last_heart_rate_bpm_, last_spo2_pct_, has_hr_,
          has_spo2_, healthy_);
    }
    return healthy_;
  }

  if (!contact_ok) {
    healthy_ = false;
    if (has_hr_) {
      inout_physiology->heart_rate_bpm = last_heart_rate_bpm_;
    }
    if (has_spo2_) {
      inout_physiology->spo2_pct = last_spo2_pct_;
    }
    if (should_log) {
      Serial.printf(
          "[C3][MAX30102] sample=1 contact=0 beat=%d red=%lu ir=%lu bpm_raw=%.1f hr=%.1f spo2=%.1f has_hr=%d has_spo2=%d healthy=%d\n",
          beat_detected, static_cast<unsigned long>(red),
          static_cast<unsigned long>(ir), bpm_candidate, last_heart_rate_bpm_,
          last_spo2_pct_, has_hr_, has_spo2_, healthy_);
    }
    return false;
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

  healthy_ = has_hr_ && has_spo2_;

  if (should_log) {
    Serial.printf(
        "[C3][MAX30102] sample=1 contact=1 beat=%d red=%lu ir=%lu bpm_raw=%.1f hr=%.1f spo2=%.1f has_hr=%d has_spo2=%d healthy=%d\n",
        beat_detected, static_cast<unsigned long>(red),
        static_cast<unsigned long>(ir), bpm_candidate, last_heart_rate_bpm_,
        last_spo2_pct_, has_hr_, has_spo2_, healthy_);
  }
  return healthy_;
}

bool Max30102Sensor::isHealthy() const { return healthy_; }

}  // namespace heatrisk
