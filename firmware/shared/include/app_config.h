#pragma once

#include <stdint.h>

namespace heatrisk {

// 全局协议版本
static constexpr uint16_t kSchemaVersion = 1;

// 为节省ESP内存，统一字符串上限（含 '\0'）
static constexpr uint16_t kMaxDeviceIdLen = 32;
static constexpr uint16_t kMaxSiteIdLen = 32;
static constexpr uint16_t kMaxGatewayIdLen = 32;
static constexpr uint16_t kMaxReasonLen = 96;
static constexpr uint16_t kMaxTopicLen = 128;

// S3 设备注册表上限，可在网关 config.h 覆盖
static constexpr uint8_t kDefaultMaxWearables = 8;

// 新鲜度窗口（毫秒）
static constexpr uint32_t kDefaultDeviceOfflineTimeoutMs = 10000;

// 任务调度默认周期（毫秒）
static constexpr uint32_t kDefaultWearableSampleIntervalMs = 1000;
static constexpr uint32_t kDefaultWearableUploadIntervalMs = 2000;
static constexpr uint32_t kDefaultGatewaySampleIntervalMs = 2000;
static constexpr uint32_t kDefaultGatewayMqttPublishIntervalMs = 2000;
static constexpr uint32_t kDefaultDashboardPollIntervalMs = 1500;

}  // namespace heatrisk
