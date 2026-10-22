#pragma once

#include <stdint.h>

namespace gateway_config {

// ---------- Identity ----------
static constexpr const char* kSiteId = "site-a";
static constexpr const char* kGatewayId = "gateway-s3-001";

// ---------- Wi-Fi ----------
// TODO(Phase 3): 替换为真实网络信息
static constexpr const char* kWifiSsid = "blackmi";
static constexpr const char* kWifiPassword = "wxy358800";
static constexpr uint32_t kWifiReconnectIntervalMs = 5000;

// ---------- MQTT ----------
// TODO(Phase 4): 配置真实 Broker
static constexpr const char* kMqttHost = "bemfa.com";
static constexpr uint16_t kMqttPort = 9501;
static constexpr const char* kMqttUsername = "";
static constexpr const char* kMqttPassword = "";
static constexpr uint32_t kMqttReconnectIntervalMs = 5000;
static constexpr uint16_t kMqttPacketBufferSize = 1024;

// ---------- Bemfa MQTT Compatibility ----------
// When enabled:
// 1) S3 uses bemfa private key as MQTT client id.
// 2) Publish topics as <topic>/up.
// 3) Subscribe command topic as <topic> and <topic>/set.
// Topic naming should follow Bemfa requirements (letters+numbers).
static constexpr bool kMqttUseBemfa = true;
static constexpr const char* kBemfaClientId = "9edf1373d45a4cf5acb1661558e86d1c";
static constexpr const char* kBemfaTopicGateway = "heatriskgw004";
static constexpr const char* kBemfaTopicWearableTelemetry = "heatriskwb004";
static constexpr const char* kBemfaTopicWearableStatus = "heatriskst004";
static constexpr const char* kBemfaTopicCommand = "heatriskcmd004";

// ---------- Pin Mapping (ESP32-S3) ----------
static constexpr uint8_t kPinI2cSda = 8;
static constexpr uint8_t kPinI2cScl = 9;
static constexpr uint32_t kI2cFrequencyHz = 400000;
static constexpr uint8_t kSht35I2cAddr = 0x44;
static constexpr uint8_t kBh1750I2cAddr = 0x23;

// ---------- HTTP ----------
static constexpr uint16_t kHttpPort = 80;
static constexpr uint16_t kWearableTcpPort = 9080;
static constexpr uint32_t kWearableTcpReadTimeoutMs = 300;
static constexpr const char* kGatewayMdnsHost = "heatrisk-gateway";
static constexpr uint16_t kGatewayDiscoveryPort = 4210;
static constexpr const char* kGatewayDiscoveryMagic = "HEATRISK_DISCOVER_V1";

// ---------- Scheduling ----------
static constexpr uint32_t kSensorIntervalMs = 2000;
static constexpr uint32_t kMqttPublishIntervalMs = 2000;
static constexpr uint32_t kOfflineTimeoutMs = 10000;

// ---------- Device Registry ----------
static constexpr uint8_t kMaxWearables = 8;

}  // namespace gateway_config
