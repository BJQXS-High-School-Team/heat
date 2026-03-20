#pragma once

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

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
  void begin(const char* ssid, const char* password, uint16_t gateway_port,
             const char* upload_path, const char* gateway_mdns_host,
             uint16_t gateway_discovery_port,
             uint16_t gateway_discovery_listen_port,
             const char* gateway_discovery_magic,
             uint32_t gateway_discovery_interval_ms,
             uint32_t reconnect_interval_ms, uint16_t http_connect_timeout_ms,
             uint16_t http_read_timeout_ms);

  void tick(uint32_t now_ms);
  bool isWifiConnected() const;
  bool isWifiReadyForUplink(uint32_t now_ms) const;
  int32_t currentRssi() const;
  bool hasGatewayEndpoint() const;
  const char* currentGatewayIp() const;
  void printWifiDiag();
  void triggerGatewayDiscovery(bool clear_cached_gateway_ip);
  bool isDiscoveryArmed() const;
  bool setManualGatewayIp(const char* ip_string);

  UploadResult uploadTelemetry(const WearableTelemetrySnapshot& snapshot,
                               OverrideCommand* out_override);

 private:
  bool ParseOverrideFromResponse(const String& response_json,
                                 OverrideCommand* out_override);
  void TryDiscoverGateway(uint32_t now_ms);
  void PollDiscoveryReplies();
  void SetGatewayIp(const IPAddress& ip, const char* source);
  void ClearGatewayIp();

  const char* ssid_ = nullptr;
  const char* password_ = nullptr;
  const char* path_ = nullptr;
  char gateway_ip_[16] = {0};
  char gateway_mdns_host_[48] = {0};
  char gateway_discovery_magic_[48] = {0};
  uint16_t port_ = 80;
  uint16_t gateway_discovery_port_ = 4210;
  uint16_t gateway_discovery_listen_port_ = 4211;
  uint32_t gateway_discovery_interval_ms_ = 5000;
  uint32_t reconnect_interval_ms_ = 5000;
  uint16_t http_connect_timeout_ms_ = 3000;
  uint16_t http_read_timeout_ms_ = 3000;
  uint32_t last_reconnect_attempt_ms_ = 0;
  wl_status_t last_wifi_status_ = WL_IDLE_STATUS;
  wl_status_t current_wifi_status_ = WL_IDLE_STATUS;
  uint32_t last_status_log_ms_ = 0;
  uint32_t last_status_change_ms_ = 0;
  uint32_t last_scan_log_ms_ = 0;
  uint32_t last_discovery_ms_ = 0;
  uint32_t reconnect_attempt_count_ = 0;
  uint32_t connect_ok_count_ = 0;
  uint32_t connect_failed_count_ = 0;
  uint32_t no_ssid_count_ = 0;
  uint32_t disconnect_count_ = 0;
  uint32_t connection_lost_count_ = 0;
  uint8_t upload_failure_streak_ = 0;
  uint32_t wifi_connected_since_ms_ = 0;
  uint32_t wifi_settle_ms_ = 1500;
  bool mdns_started_ = false;
  bool udp_started_ = false;
  bool discovery_armed_ = false;
  WiFiUDP discovery_udp_;
};

}  // namespace heatrisk
