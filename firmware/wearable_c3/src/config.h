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
static constexpr uint16_t kGatewayPort = 9080;  // C3->S3 TCP uplink port
static constexpr const char* kUploadPath = "";  // 保留占位（TCP 模式不使用）
static constexpr uint32_t kWifiReconnectIntervalMs = 5000;
static constexpr uint32_t kHttpConnectTimeoutMs = 2000;  // TCP connect timeout
static constexpr uint32_t kHttpReadTimeoutMs = 2000;     // TCP response timeout
// 自动发现 S3：优先 mDNS，其次 UDP 广播
static constexpr const char* kGatewayMdnsHost = "heatrisk-gateway";
static constexpr uint16_t kGatewayDiscoveryPort = 4210;
static constexpr uint16_t kGatewayDiscoveryListenPort = 4211;
static constexpr const char* kGatewayDiscoveryMagic = "HEATRISK_DISCOVER_V1";
static constexpr uint32_t kGatewayDiscoveryIntervalMs = 5000;

// ---------- Pin Mapping (ESP32-C3) ----------
// 当前联调阶段按现有接线保持 GPIO8/9；若改线后可再切回非 strapping 脚。
static constexpr uint8_t kPinI2cSda = 8;
static constexpr uint8_t kPinI2cScl = 9;
static constexpr uint8_t kPinDs18b20 = 4;
// 避免占用可能与下载/串口复用冲突的 20/21，引脚改为更安全的 GPIO6/7。
// 接线: GPS TX -> C3 RX(GPIO6), GPS RX -> C3 TX(GPIO7)
static constexpr uint8_t kPinGpsRx = 6;
static constexpr uint8_t kPinGpsTx = 7;
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

// ---------- Sensor Feature Switch ----------
// When false: skip MAX3010x init/read, keep other functions running.
// 临时建议：MAX30102 更换期间可设为 false，避免影响整机联调。
static constexpr bool kEnableMax3010x = false;
// 临时建议：如果 GPS 模块接入导致 Wi-Fi 不稳定，可先关闭 GPS 接口联调其余链路。
static constexpr bool kEnableGps = true;

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
