#include "sensor_mpu6050.h"

#include <math.h>
#include <string.h>

namespace heatrisk {

namespace {

float Clamp01(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

void FillActivityLevel(float score, char* out_level, size_t out_size) {
  if (out_level == nullptr || out_size == 0) {
    return;
  }
  const char* level = "low";
  if (score >= 0.66f) {
    level = "high";
  } else if (score >= 0.33f) {
    level = "medium";
  }
  strncpy(out_level, level, out_size - 1);
  out_level[out_size - 1] = '\0';
}

}  // namespace

bool Mpu6050Sensor::begin(TwoWire* wire) {
  if (wire == nullptr) {
    return false;
  }
  if (!sensor_.begin(0x68, wire)) {
    healthy_ = false;
    return false;
  }

  sensor_.setAccelerometerRange(MPU6050_RANGE_8_G);
  sensor_.setGyroRange(MPU6050_RANGE_500_DEG);
  sensor_.setFilterBandwidth(MPU6050_BAND_21_HZ);
  healthy_ = true;
  return true;
}

bool Mpu6050Sensor::sample(MotionData* out_motion) {
  if (out_motion == nullptr) {
    return false;
  }

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  sensor_.getEvent(&accel, &gyro, &temp);

  const bool finite = isfinite(accel.acceleration.x) &&
                      isfinite(accel.acceleration.y) &&
                      isfinite(accel.acceleration.z) && isfinite(gyro.gyro.x) &&
                      isfinite(gyro.gyro.y) && isfinite(gyro.gyro.z);
  if (!finite) {
    healthy_ = false;
    *out_motion = last_good_;
    return false;
  }

  const float accel_mag = sqrtf(accel.acceleration.x * accel.acceleration.x +
                                accel.acceleration.y * accel.acceleration.y +
                                accel.acceleration.z * accel.acceleration.z);
  const float gyro_mag =
      sqrtf(gyro.gyro.x * gyro.gyro.x + gyro.gyro.y * gyro.gyro.y + gyro.gyro.z * gyro.gyro.z);

  // 运动强度是趋势指标，不是医疗指标。
  const float accel_dynamic = fabsf(accel_mag - 9.81f);
  const float accel_norm = Clamp01(accel_dynamic / 6.0f);
  const float gyro_norm = Clamp01(gyro_mag / 6.0f);
  const float raw_score = 0.7f * accel_norm + 0.3f * gyro_norm;
  const float smooth_score = (last_good_.activity_score * 0.6f) + (raw_score * 0.4f);

  last_good_.activity_score = Clamp01(smooth_score);
  FillActivityLevel(last_good_.activity_score, last_good_.activity_level,
                    sizeof(last_good_.activity_level));

  healthy_ = true;
  *out_motion = last_good_;
  return true;
}

bool Mpu6050Sensor::isHealthy() const { return healthy_; }

}  // namespace heatrisk
