#pragma once

#include <stddef.h>

#include <PubSubClient.h>
#include <WiFiClient.h>

#include "command_cache.h"
#include "data_models.h"
#include "device_registry.h"

namespace heatrisk {

class MqttBridge {
 public:
  MqttBridge(DeviceRegistry* registry, CommandCache* command_cache,
             GatewayTelemetrySnapshot* host_snapshot);

  void begin(const char* site_id, const char* gateway_id, const char* mqtt_host,
             uint16_t mqtt_port, const char* mqtt_username,
             const char* mqtt_password, uint32_t reconnect_interval_ms,
             uint32_t publish_interval_ms, uint16_t packet_buffer_size);

  void tick(uint32_t now_ms);
  bool isConnected();

 private:
  void tryConnect(uint32_t now_ms);
  void publishIfDue(uint32_t now_ms);
  void publishGatewayTelemetry();
  void publishWearableTelemetryAndStatus(uint32_t now_ms);

  void onMessage(char* topic, uint8_t* payload, unsigned int length);
  void applyCommandPayload(const char* topic, const char* payload_json,
                           uint32_t now_ms);

  static void StaticOnMessage(char* topic, uint8_t* payload, unsigned int length);
  static bool ParseStateStrict(const char* value, StateLevel* out_state);
  static void CopySafe(char* out, size_t out_size, const char* in);

  DeviceRegistry* registry_ = nullptr;
  CommandCache* command_cache_ = nullptr;
  GatewayTelemetrySnapshot* host_snapshot_ = nullptr;

  WiFiClient wifi_client_;
  PubSubClient mqtt_client_;

  char site_id_[kMaxSiteIdLen] = "";
  char gateway_id_[kMaxGatewayIdLen] = "";
  char mqtt_host_[96] = "";
  char mqtt_username_[64] = "";
  char mqtt_password_[64] = "";
  uint16_t mqtt_port_ = 1883;
  uint32_t reconnect_interval_ms_ = 5000;
  uint32_t publish_interval_ms_ = 2000;
  uint32_t last_reconnect_attempt_ms_ = 0;
  uint32_t last_publish_ms_ = 0;

  char command_wildcard_topic_[kMaxTopicLen] = "";
  char gateway_topic_[kMaxTopicLen] = "";
  char client_id_[64] = "";

  static MqttBridge* s_instance_;
};

}  // namespace heatrisk
