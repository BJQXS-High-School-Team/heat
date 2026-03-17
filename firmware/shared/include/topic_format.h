#pragma once

#include <stdbool.h>
#include <stddef.h>

namespace heatrisk {

bool BuildGatewayTelemetryTopic(char* out, size_t out_size, const char* site_id,
                                const char* gateway_id);
bool BuildWearableTelemetryTopic(char* out, size_t out_size, const char* site_id,
                                 const char* device_id);
bool BuildWearableStatusTopic(char* out, size_t out_size, const char* site_id,
                              const char* device_id);
bool BuildCommandAllTopic(char* out, size_t out_size, const char* site_id);
bool BuildCommandDeviceTopic(char* out, size_t out_size, const char* site_id,
                             const char* device_id);

}  // namespace heatrisk
