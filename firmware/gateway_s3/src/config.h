#pragma once

#include <stdint.h>

namespace gateway_config {

// ---------- Identity ----------
static constexpr const char* kSiteId = "site-a";
static constexpr const char* kGatewayId = "gateway-s3-001";

// ---------- Wi-Fi ----------
// TODO(Phase 3): 替换为真实网络信息
static constexpr const char* kWifiSsid = "YOUR_WIFI_SSID";
static constexpr const char* kWifiPassword = "YOUR_WIFI_PASSWORD";
static constexpr uint32_t kWifiReconnectIntervalMs = 5000;

// ---------- MQTT ----------
// TODO(Phase 4): 配置真实 Broker
static constexpr const char* kMqttHost = "broker.example.com";
static constexpr uint16_t kMqttPort = 1883;
static constexpr const char* kMqttUsername = "mqtt_user";
static constexpr const char* kMqttPassword = "mqtt_password";
static constexpr uint32_t kMqttReconnectIntervalMs = 5000;
static constexpr uint16_t kMqttPacketBufferSize = 1024;

// ---------- Pin Mapping (ESP32-S3) ----------
static constexpr uint8_t kPinI2cSda = 8;
static constexpr uint8_t kPinI2cScl = 9;
static constexpr uint32_t kI2cFrequencyHz = 400000;
static constexpr uint8_t kSht35I2cAddr = 0x44;
static constexpr uint8_t kBh1750I2cAddr = 0x23;

// ---------- HTTP ----------
static constexpr uint16_t kHttpPort = 80;

// ---------- Scheduling ----------
static constexpr uint32_t kSensorIntervalMs = 2000;
static constexpr uint32_t kMqttPublishIntervalMs = 2000;
static constexpr uint32_t kOfflineTimeoutMs = 10000;

// ---------- Device Registry ----------
static constexpr uint8_t kMaxWearables = 8;

}  // namespace gateway_config
