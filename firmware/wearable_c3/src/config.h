#pragma once

#include <stdint.h>

#include "data_models.h"

namespace wearable_config {

// ---------- Device Identity ----------
static constexpr const char* kDeviceId = "wearable-c3-001";
static constexpr const char* kDeviceType = "wearable";

// ---------- Wi-Fi / Gateway ----------
// TODO(Phase 2): 替换为真实网络信息
static constexpr const char* kWifiSsid = "YOUR_WIFI_SSID";
static constexpr const char* kWifiPassword = "YOUR_WIFI_PASSWORD";
static constexpr const char* kGatewayHost = "192.168.1.50";
static constexpr uint16_t kGatewayPort = 80;
static constexpr const char* kUploadPath = "/api/wearable/upload";

// ---------- Pin Mapping (ESP32-C3) ----------
static constexpr uint8_t kPinI2cSda = 8;
static constexpr uint8_t kPinI2cScl = 9;
static constexpr uint8_t kPinDs18b20 = 4;
static constexpr uint8_t kPinGpsRx = 20;
static constexpr uint8_t kPinGpsTx = 21;
static constexpr uint8_t kPinBuzzer = 2;
static constexpr uint8_t kPinVibration = 3;

// ---------- Scheduling ----------
static constexpr uint32_t kSampleIntervalMs = 1000;
static constexpr uint32_t kUploadIntervalMs = 2000;
static constexpr uint32_t kDisplayIntervalMs = 500;

// ---------- Local Rule Thresholds ----------
static constexpr heatrisk::LocalThresholds kThresholds = {
    100.0f,  // heart_rate_warning_threshold
    120.0f,  // heart_rate_danger_threshold
    94.0f,   // spo2_warning_threshold
    90.0f,   // spo2_danger_threshold
    37.8f,   // body_temp_warning_threshold
    38.5f,   // body_temp_danger_threshold
    0.65f,   // activity_warning_threshold
    0.85f,   // activity_danger_threshold
    2        // warning_rules_for_danger
};

}  // namespace wearable_config
