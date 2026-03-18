#pragma once

#include <ArduinoJson.h>
#include <WebServer.h>

#include "command_cache.h"
#include "data_models.h"
#include "device_registry.h"

namespace heatrisk {

class GatewayHttpServer {
 public:
  GatewayHttpServer(uint16_t port, DeviceRegistry* registry,
                    CommandCache* command_cache,
                    GatewayTelemetrySnapshot* host_snapshot);

  void begin();
  void tick();

 private:
  void handleRoot();
  void handleHealth();
  void handleHost();
  void handleDevices();
  void handleDevice();
  void handleWearableUpload();

  bool parseWearablePayload(const String& body, WearableTelemetrySnapshot* out,
                            String* out_error);
  void writeOverrideJson(ArduinoJson::JsonObject obj,
                         const OverrideCommand& command);
  void writeDeviceJson(ArduinoJson::JsonObject obj, const DeviceRecord& record,
                       uint32_t now_ms);
  OverrideCommand buildDefaultOverride() const;

  WebServer server_;
  DeviceRegistry* registry_ = nullptr;
  CommandCache* command_cache_ = nullptr;
  GatewayTelemetrySnapshot* host_snapshot_ = nullptr;
};

}  // namespace heatrisk
