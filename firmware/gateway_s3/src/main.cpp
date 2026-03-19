#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include <string.h>

#include "command_cache.h"
#include "config.h"
#include "device_registry.h"
#include "gateway_sensors.h"
#include "http_server.h"
#include "mqtt_bridge.h"

namespace {

heatrisk::GatewaySensors g_gateway_sensors;
heatrisk::DeviceRegistry g_registry(gateway_config::kMaxWearables);
heatrisk::CommandCache g_command_cache(gateway_config::kMaxWearables);
heatrisk::GatewayTelemetrySnapshot g_host_snapshot;
heatrisk::GatewayHttpServer g_http_server(gateway_config::kHttpPort, &g_registry,
                                          &g_command_cache, &g_host_snapshot);
heatrisk::MqttBridge g_mqtt_bridge(&g_registry, &g_command_cache, &g_host_snapshot);

uint32_t g_last_sensor_ms = 0;
uint32_t g_last_wifi_retry_ms = 0;

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
  WiFi.begin(gateway_config::kWifiSsid, gateway_config::kWifiPassword);
  g_last_wifi_retry_ms = millis();
}

void TickWifi(uint32_t now_ms) {
  if (WiFi.status() == WL_CONNECTED) {
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
  WiFi.disconnect();
  WiFi.begin(gateway_config::kWifiSsid, gateway_config::kWifiPassword);
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
  g_mqtt_bridge.tick(now_ms);
  g_http_server.tick();
}
