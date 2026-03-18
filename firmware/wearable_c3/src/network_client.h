#pragma once

#include <Arduino.h>

#include "data_models.h"

namespace heatrisk {

struct UploadResult {
  bool request_sent = false;
  bool http_ok = false;
  bool response_parsed = false;
  int http_status = 0;
};

class WearableNetworkClient {
 public:
  void begin(const char* ssid, const char* password, const char* gateway_host,
             uint16_t gateway_port, const char* upload_path,
             uint32_t reconnect_interval_ms, uint16_t http_connect_timeout_ms,
             uint16_t http_read_timeout_ms);

  void tick(uint32_t now_ms);
  bool isWifiConnected() const;
  int32_t currentRssi() const;

  UploadResult uploadTelemetry(const WearableTelemetrySnapshot& snapshot,
                               OverrideCommand* out_override);

 private:
  bool ParseOverrideFromResponse(const String& response_json,
                                 OverrideCommand* out_override);

  const char* ssid_ = nullptr;
  const char* password_ = nullptr;
  const char* host_ = nullptr;
  const char* path_ = nullptr;
  uint16_t port_ = 80;
  uint32_t reconnect_interval_ms_ = 5000;
  uint16_t http_connect_timeout_ms_ = 3000;
  uint16_t http_read_timeout_ms_ = 3000;
  uint32_t last_reconnect_attempt_ms_ = 0;
};

}  // namespace heatrisk
