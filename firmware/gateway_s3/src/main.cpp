#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include <string.h>

#include "command_cache.h"
#include "config.h"
#include "device_registry.h"
#include "gateway_discovery.h"
#include "gateway_sensors.h"
#include "http_server.h"
#include "mqtt_bridge.h"
#include "wearable_tcp_server.h"

namespace {

heatrisk::GatewaySensors g_gateway_sensors;
heatrisk::GatewayDiscoveryService g_discovery_service;
heatrisk::DeviceRegistry g_registry(gateway_config::kMaxWearables);
heatrisk::CommandCache g_command_cache(gateway_config::kMaxWearables);
heatrisk::GatewayTelemetrySnapshot g_host_snapshot;
heatrisk::GatewayHttpServer g_http_server(gateway_config::kHttpPort, &g_registry,
                                          &g_command_cache, &g_host_snapshot);
heatrisk::WearableTcpServer g_wearable_tcp_server(
    gateway_config::kWearableTcpPort, gateway_config::kWearableTcpReadTimeoutMs,
    &g_registry, &g_command_cache, &g_host_snapshot);
heatrisk::MqttBridge g_mqtt_bridge(&g_registry, &g_command_cache, &g_host_snapshot);

uint32_t g_last_sensor_ms = 0;
uint32_t g_last_wifi_retry_ms = 0;
wl_status_t g_last_wifi_status = WL_IDLE_STATUS;
uint32_t g_last_wifi_log_ms = 0;

const char* WifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD:
      return "NO_SHIELD";
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID";
    case WL_SCAN_COMPLETED:
      return "SCAN_COMPLETED";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

void InitHostSnapshot() {
  g_host_snapshot.schema_version = heatrisk::kSchemaVersion;
  strncpy(g_host_snapshot.site_id, gateway_config::kSiteId,
          sizeof(g_host_snapshot.site_id) - 1);
  g_host_snapshot.site_id[sizeof(g_host_snapshot.site_id) - 1] = '\0';

  strncpy(g_host_snapshot.gateway_id, gateway_config::kGatewayId,
          sizeof(g_host_snapshot.gateway_id) - 1);
  g_host_snapshot.gateway_id[sizeof(g_host_snapshot.gateway_id) - 1] = '\0';
}

void StartWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(gateway_config::kWifiSsid, gateway_config::kWifiPassword);
  g_last_wifi_retry_ms = millis();
  g_last_wifi_status = WiFi.status();
  g_last_wifi_log_ms = 0;
  Serial.printf("[S3][WiFi] begin ssid=%s status=%s\n", gateway_config::kWifiSsid,
                WifiStatusToString(g_last_wifi_status));
}

void TickWifi(uint32_t now_ms) {
  const wl_status_t status = WiFi.status();
  if (status != g_last_wifi_status || (now_ms - g_last_wifi_log_ms >= 5000)) {
    g_last_wifi_status = status;
    g_last_wifi_log_ms = now_ms;
    if (status == WL_CONNECTED) {
      Serial.printf("[S3][WiFi] connected ip=%s rssi=%d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      Serial.printf("[S3][WiFi] status=%s(%d)\n", WifiStatusToString(status),
                    static_cast<int>(status));
    }
  }

  if (status == WL_CONNECTED) {
    g_host_snapshot.network.wifi_connected = true;
    g_host_snapshot.network.rssi = WiFi.RSSI();
    return;
  }

  g_host_snapshot.network.wifi_connected = false;
  g_host_snapshot.network.rssi = -127;
  if (now_ms - g_last_wifi_retry_ms < gateway_config::kWifiReconnectIntervalMs) {
    return;
  }
  g_last_wifi_retry_ms = now_ms;
  Serial.println("[S3][WiFi] reconnect...");
  if (!WiFi.reconnect()) {
    WiFi.begin(gateway_config::kWifiSsid, gateway_config::kWifiPassword);
  }
}

void TickSensors(uint32_t now_ms) {
  if (now_ms - g_last_sensor_ms < gateway_config::kSensorIntervalMs) {
    return;
  }
  g_last_sensor_ms = now_ms;

  g_gateway_sensors.sample(&g_host_snapshot.gateway_ambient,
                           &g_host_snapshot.sensor_health);
  g_host_snapshot.gateway_time_ms = now_ms;

  Serial.printf(
      "[S3][SENSORS] gateway_temp=%.2fC gateway_humi=%.2f%% gateway_lux=%.1f "
      "sht35_ok=%d bh1750_ok=%d\n",
      g_host_snapshot.gateway_ambient.temperature_c,
      g_host_snapshot.gateway_ambient.humidity_pct,
      g_host_snapshot.gateway_ambient.light_lux,
      g_host_snapshot.sensor_health.sht35_ok,
      g_host_snapshot.sensor_health.bh1750_ok);
}

void TickMaintenance(uint32_t now_ms) {
  g_registry.refreshOnlineStatus(now_ms, gateway_config::kOfflineTimeoutMs);
  g_command_cache.clearExpired(now_ms);
  g_host_snapshot.network.mqtt_connected = g_mqtt_bridge.isConnected();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(120);
  Serial.println("[S3] boot");

  Wire.begin(gateway_config::kPinI2cSda, gateway_config::kPinI2cScl,
             gateway_config::kI2cFrequencyHz);
  InitHostSnapshot();
  StartWifi();

  const bool sensor_init_ok =
      g_gateway_sensors.begin(gateway_config::kSht35I2cAddr,
                              gateway_config::kBh1750I2cAddr, &Wire);
  Serial.printf("[S3][INIT] sensors_ready=%d\n", sensor_init_ok);

  g_http_server.begin();
  Serial.printf("[S3][HTTP] server started at :%u\n", gateway_config::kHttpPort);
  g_wearable_tcp_server.begin();
  g_discovery_service.begin(gateway_config::kGatewayMdnsHost,
                            gateway_config::kHttpPort,
                            gateway_config::kGatewayDiscoveryPort,
                            gateway_config::kGatewayDiscoveryMagic);

  g_mqtt_bridge.begin(gateway_config::kSiteId, gateway_config::kGatewayId,
                      gateway_config::kMqttHost, gateway_config::kMqttPort,
                      gateway_config::kMqttUsername,
                      gateway_config::kMqttPassword,
                      gateway_config::kMqttReconnectIntervalMs,
                      gateway_config::kMqttPublishIntervalMs,
                      gateway_config::kMqttPacketBufferSize,
                      gateway_config::kMqttUseBemfa,
                      gateway_config::kBemfaClientId,
                      gateway_config::kBemfaTopicGateway,
                      gateway_config::kBemfaTopicWearableTelemetry,
                      gateway_config::kBemfaTopicWearableStatus,
                      gateway_config::kBemfaTopicCommand);
}

void loop() {
  const uint32_t now_ms = millis();
  TickWifi(now_ms);
  TickSensors(now_ms);
  TickMaintenance(now_ms);
  g_discovery_service.tick();
  g_mqtt_bridge.tick(now_ms);
  g_wearable_tcp_server.tick();
  g_http_server.tick();
}
