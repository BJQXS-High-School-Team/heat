#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_config.h"

namespace heatrisk {

enum class StateLevel : uint8_t {
  kNormal = 0,
  kWarning = 1,
  kDanger = 2
};

inline const char* StateLevelToString(StateLevel value) {
  switch (value) {
    case StateLevel::kNormal:
      return "normal";
    case StateLevel::kWarning:
      return "warning";
    case StateLevel::kDanger:
      return "danger";
    default:
      return "normal";
  }
}

inline StateLevel StateLevelFromString(const char* value) {
  if (value == nullptr) {
    return StateLevel::kNormal;
  }
  if (strcmp(value, "danger") == 0) {
    return StateLevel::kDanger;
  }
  if (strcmp(value, "warning") == 0) {
    return StateLevel::kWarning;
  }
  return StateLevel::kNormal;
}

struct GpsData {
  double lat = 0.0;
  double lng = 0.0;
  bool has_fix = false;
  uint8_t satellites = 0;
};

struct PhysiologyData {
  float body_temp_c = 0.0f;
  float heart_rate_bpm = 0.0f;
  float spo2_pct = 0.0f;
};

struct MotionData {
  float activity_score = 0.0f;
  char activity_level[12] = "unknown";
};

struct SensorHealth {
  bool gps_ok = false;
  bool temp_ok = false;
  bool max30102_ok = false;
  bool imu_ok = false;
  bool oled_ok = false;
  bool sht35_ok = false;
  bool bh1750_ok = false;
};

struct WearableStatus {
  StateLevel local_state = StateLevel::kNormal;
  StateLevel effective_state = StateLevel::kNormal;
  bool alarm_active = false;
};

struct NetworkState {
  int32_t rssi = 0;
  bool wifi_connected = false;
  bool mqtt_connected = false;
};

struct WearableTelemetrySnapshot {
  uint16_t schema_version = kSchemaVersion;
  char device_id[kMaxDeviceIdLen] = "";
  char device_type[16] = "wearable";
  uint32_t seq = 0;
  uint32_t device_uptime_ms = 0;
  GpsData gps;
  PhysiologyData physiology;
  MotionData motion;
  WearableStatus status;
  SensorHealth sensor_health;
  NetworkState network;
};

struct GatewayAmbientData {
  float temperature_c = 0.0f;
  float humidity_pct = 0.0f;
  float light_lux = 0.0f;
};

struct GatewayTelemetrySnapshot {
  uint16_t schema_version = kSchemaVersion;
  char site_id[kMaxSiteIdLen] = "";
  char gateway_id[kMaxGatewayIdLen] = "";
  uint32_t gateway_time_ms = 0;
  uint64_t gateway_receive_epoch_ms = 0;
  GatewayAmbientData gateway_ambient;
  SensorHealth sensor_health;
  NetworkState network;
};

struct OverrideCommand {
  bool enabled = false;
  StateLevel state = StateLevel::kNormal;
  char reason[kMaxReasonLen] = "";
  bool vibrate = false;
  bool buzzer = false;
  uint32_t expires_in_s = 0;
  uint32_t issued_at_ms = 0;  // 使用 S3 本地 millis 作为下发时间基准
};

struct LocalThresholds {
  float heart_rate_warning_threshold = 100.0f;
  float heart_rate_danger_threshold = 120.0f;
  float spo2_warning_threshold = 94.0f;
  float spo2_danger_threshold = 90.0f;
  float body_temp_warning_threshold = 37.8f;
  float body_temp_danger_threshold = 38.5f;
  float activity_warning_threshold = 0.65f;
  float activity_danger_threshold = 0.85f;
  uint8_t warning_rules_for_danger = 2;
};

struct LocalStateEvalResult {
  StateLevel local_state = StateLevel::kNormal;
  bool alarm_active = false;
  uint8_t warning_rule_hits = 0;
  bool danger_rule_hit = false;
};

}  // namespace heatrisk
