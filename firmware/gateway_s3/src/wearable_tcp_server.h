#pragma once

#include <ArduinoJson.h>
#include <WiFi.h>

#include "command_cache.h"
#include "data_models.h"
#include "device_registry.h"

namespace heatrisk {

class WearableTcpServer {
 public:
  WearableTcpServer(uint16_t port, uint32_t read_timeout_ms,
                    DeviceRegistry* registry, CommandCache* command_cache,
                    GatewayTelemetrySnapshot* host_snapshot);

  void begin();
  void tick();

 private:
  bool readJsonLine(WiFiClient* client, String* out_line) const;
  bool parseWearablePayload(const String& body, WearableTelemetrySnapshot* out,
                            String* out_error) const;
  void writeOverrideJson(ArduinoJson::JsonObject obj,
                         const OverrideCommand& command) const;
  OverrideCommand buildDefaultOverride() const;
  void sendError(WiFiClient* client, const char* error_text) const;
  void handleClient(WiFiClient* client);

  uint16_t port_ = 0;
  uint32_t read_timeout_ms_ = 1500;
  WiFiServer server_;
  DeviceRegistry* registry_ = nullptr;
  CommandCache* command_cache_ = nullptr;
  GatewayTelemetrySnapshot* host_snapshot_ = nullptr;
};

}  // namespace heatrisk
