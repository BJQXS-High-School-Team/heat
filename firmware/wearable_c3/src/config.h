#pragma once

#include <stdint.h>

#include "data_models.h"

namespace wearable_config {

// ---------- Device Identity ----------
static constexpr const char* kDeviceId = "wearable-c3-001";
static constexpr const char* kDeviceType = "wearable";

// ---------- Wi-Fi / Gateway ----------
// TODO(Phase 2): 替换为真实网络信息
static constexpr const char* kWifiSsid = "blackmi";
static constexpr const char* kWifiPassword = "wxy358800";
static constexpr const char* kGatewayHost = "172.20.10.1";
static constexpr uint16_t kGatewayPort = 80;
static constexpr const char* kUploadPath = "/api/wearable/upload";
static constexpr uint32_t kWifiReconnectIntervalMs = 5000;
static constexpr uint32_t kHttpConnectTimeoutMs = 3000;
static constexpr uint32_t kHttpReadTimeoutMs = 3000;

// ---------- Pin Mapping (ESP32-C3) ----------
static constexpr uint8_t kPinI2cSda = 8;
static constexpr uint8_t kPinI2cScl = 9;
static constexpr uint8_t kPinDs18b20 = 4;
static constexpr uint8_t kPinGpsRx = 20;
static constexpr uint8_t kPinGpsTx = 21;
static constexpr uint8_t kPinBuzzer = 2;
static constexpr uint8_t kPinVibration = 3;
static constexpr uint32_t kI2cFrequencyHz = 400000;
static constexpr uint32_t kGpsBaud = 9600;

// ---------- OLED ----------
static constexpr uint8_t kOledI2cAddr = 0x3C;
static constexpr uint8_t kOledWidth = 128;
static constexpr uint8_t kOledHeight = 64;
static constexpr int8_t kOledResetPin = -1;

// ---------- Scheduling ----------
static constexpr uint32_t kSampleIntervalMs = 1000;
static constexpr uint32_t kUploadIntervalMs = 2000;
static constexpr uint32_t kDisplayIntervalMs = 500;

// ---------- Local Rule Thresholds ----------
static const heatrisk::LocalThresholds kThresholds = []() {
  heatrisk::LocalThresholds t;
  t.heart_rate_warning_threshold = 100.0f;
  t.heart_rate_danger_threshold = 120.0f;
  t.spo2_warning_threshold = 94.0f;
  t.spo2_danger_threshold = 90.0f;
  t.body_temp_warning_threshold = 37.8f;
  t.body_temp_danger_threshold = 38.5f;
  t.activity_warning_threshold = 0.65f;
  t.activity_danger_threshold = 0.85f;
  t.warning_rules_for_danger = 2;
  return t;
}();

// ---------- Alarm Patterns ----------
static constexpr uint32_t kWarningPatternPeriodMs = 1500;
static constexpr uint32_t kWarningVibrationOnMs = 250;
static constexpr uint32_t kWarningBuzzerOnMs = 80;

static constexpr uint32_t kDangerPatternPeriodMs = 700;
static constexpr uint32_t kDangerVibrationOnMs = 350;
static constexpr uint32_t kDangerBuzzerOnMs = 120;

}  // namespace wearable_config
