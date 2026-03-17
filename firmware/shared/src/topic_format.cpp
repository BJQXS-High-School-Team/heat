#include "topic_format.h"

#include <stdio.h>

namespace heatrisk {

namespace {

bool BuildTopic(char* out, size_t out_size, const char* fmt, const char* site_id,
                const char* id) {
  if (out == nullptr || out_size == 0 || site_id == nullptr || id == nullptr) {
    return false;
  }
  const int n = snprintf(out, out_size, fmt, site_id, id);
  return n > 0 && static_cast<size_t>(n) < out_size;
}

}  // namespace

bool BuildGatewayTelemetryTopic(char* out, size_t out_size, const char* site_id,
                                const char* gateway_id) {
  return BuildTopic(out, out_size, "heatrisk/site/%s/gateway/%s/telemetry", site_id,
                    gateway_id);
}

bool BuildWearableTelemetryTopic(char* out, size_t out_size, const char* site_id,
                                 const char* device_id) {
  return BuildTopic(out, out_size, "heatrisk/site/%s/wearable/%s/telemetry", site_id,
                    device_id);
}

bool BuildWearableStatusTopic(char* out, size_t out_size, const char* site_id,
                              const char* device_id) {
  return BuildTopic(out, out_size, "heatrisk/site/%s/wearable/%s/status", site_id,
                    device_id);
}

bool BuildCommandAllTopic(char* out, size_t out_size, const char* site_id) {
  if (out == nullptr || out_size == 0 || site_id == nullptr) {
    return false;
  }
  const int n = snprintf(out, out_size, "heatrisk/site/%s/command/all", site_id);
  return n > 0 && static_cast<size_t>(n) < out_size;
}

bool BuildCommandDeviceTopic(char* out, size_t out_size, const char* site_id,
                             const char* device_id) {
  return BuildTopic(out, out_size, "heatrisk/site/%s/command/%s", site_id, device_id);
}

}  // namespace heatrisk
