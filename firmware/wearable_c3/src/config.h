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
static constexpr uint16_t kGatewayProbePort = 80;  // 用于发现探测，避免占用上传端口
static constexpr const char* kUploadPath = "";  // 保留占位（TCP 模式不使用）
static constexpr uint32_t kWifiReconnectIntervalMs = 5000;
static constexpr uint32_t kHttpConnectTimeoutMs = 4000;  // TCP connect timeout
static constexpr uint32_t kHttpReadTimeoutMs = 3000;     // TCP response timeout
// 自动发现 S3：优先 mDNS，其次 UDP 广播
static constexpr const char* kGatewayMdnsHost = "heatrisk-gateway";
static constexpr uint16_t kGatewayDiscoveryPort = 4210;
static constexpr uint16_t kGatewayDiscoveryListenPort = 4211;
static constexpr const char* kGatewayDiscoveryMagic = "HEATRISK_DISCOVER_V1";
static constexpr uint32_t kGatewayDiscoveryIntervalMs = 5000;
static constexpr uint32_t kGatewayProbeIntervalMs = 3000;
static constexpr uint8_t kGatewayProbeHostMin = 1;
static constexpr uint8_t kGatewayProbeHostMax = 12;
static constexpr uint32_t kGatewaySettleMs = 2500;

// ---------- Pin Mapping (ESP32-C3) ----------
// C3 端 I2C 改到更安全的 GPIO5/10，降低多模块并联时的稳态风险。
static constexpr uint8_t kPinI2cSda = 5;
static constexpr uint8_t kPinI2cScl = 10;
static constexpr uint8_t kPinDs18b20 = 4;
// 避免占用可能与下载/串口复用冲突的 20/21，引脚改为更安全的 GPIO6/7。
// 注意：这里的 Rx/Tx 是“ESP32-C3 这一侧”的串口命名。
// 接线应交叉：GPS TX -> C3 RX(GPIO6), GPS RX -> C3 TX(GPIO7)
static constexpr uint8_t kPinGpsRx = 6;
static constexpr uint8_t kPinGpsTx = 7;
static constexpr uint8_t kPinBuzzer = 2;
static constexpr uint8_t kPinVibration = 3;
// 多个 I2C 模块并联时优先稳态，先降到 100kHz。
static constexpr uint32_t kI2cFrequencyHz = 100000;
static constexpr uint32_t kI2cTimeoutMs = 20;
static constexpr uint32_t kGpsBaud = 9600;

// ---------- OLED ----------
static constexpr uint8_t kOledI2cAddr = 0x3C;
static constexpr uint8_t kOledWidth = 128;
static constexpr uint8_t kOledHeight = 64;
static constexpr int8_t kOledResetPin = -1;

// ---------- Scheduling ----------
static constexpr uint32_t kSampleIntervalMs = 1000;
static constexpr uint32_t kUploadIntervalMs = 2000;
static constexpr uint32_t kDisplayIntervalMs = 1000;
static constexpr uint32_t kMax3010xPollIntervalMs = 20;

// ---------- Sensor Feature Switch ----------
// When false: skip MAX3010x init/read, keep other functions running.
// 当前使用非阻塞 FIFO 读取路径重新验证 MAX30102 对系统稳态的影响。
// 若现场仍然出现联网/上传被拖慢，可临时改回 false 继续联调其它模块。
static constexpr bool kEnableMax3010x = true;
// 临时建议：如果 GPS 模块接入导致 Wi-Fi 不稳定，可先关闭 GPS 接口联调其余链路。
static constexpr bool kEnableGps =true;

// ---------- MAX30102 ----------
// 当前网络链路已恢复稳定，调高 MAX30102 采样参数以便更容易形成可检测心率波形。
static constexpr uint8_t kMax3010xLedBrightness = 0x24;
static constexpr uint8_t kMax3010xSampleAverage = 1;
static constexpr uint8_t kMax3010xLedMode = 2;
static constexpr uint16_t kMax3010xSampleRate = 100;
static constexpr uint16_t kMax3010xPulseWidth = 411;
static constexpr uint16_t kMax3010xAdcRange = 4096;
static constexpr uint8_t kMax3010xPulseAmplitudeRed = 0x24;
static constexpr uint8_t kMax3010xPulseAmplitudeIr = 0x24;

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
