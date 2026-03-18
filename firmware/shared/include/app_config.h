#pragma once

#include <stdint.h>

namespace heatrisk {

// Protocol/version constants
static constexpr uint16_t kSchemaVersion = 1;

// Bounded string sizes (including '\0')
static constexpr uint16_t kMaxDeviceIdLen = 32;
static constexpr uint16_t kMaxSiteIdLen = 32;
static constexpr uint16_t kMaxGatewayIdLen = 32;
static constexpr uint16_t kMaxReasonLen = 96;
static constexpr uint16_t kMaxTopicLen = 128;

// Default system limits
static constexpr uint8_t kDefaultMaxWearables = 8;
static constexpr uint32_t kDefaultDeviceOfflineTimeoutMs = 10000;

// Default scheduler intervals (ms)
static constexpr uint32_t kDefaultWearableSampleIntervalMs = 1000;
static constexpr uint32_t kDefaultWearableUploadIntervalMs = 2000;
static constexpr uint32_t kDefaultGatewaySampleIntervalMs = 2000;
static constexpr uint32_t kDefaultGatewayMqttPublishIntervalMs = 2000;
static constexpr uint32_t kDefaultDashboardPollIntervalMs = 1500;

}  // namespace heatrisk
