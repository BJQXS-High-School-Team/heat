#include <Arduino.h>

#include "config.h"
#include "topic_format.h"

namespace {

uint32_t g_last_log_ms = 0;
char g_topic_buffer[128];

void TickLog() {
  const uint32_t now = millis();
  if (now - g_last_log_ms < 3000) {
    return;
  }
  g_last_log_ms = now;

  const bool ok = heatrisk::BuildGatewayTelemetryTopic(
      g_topic_buffer, sizeof(g_topic_buffer), gateway_config::kSiteId,
      gateway_config::kGatewayId);
  if (ok) {
    Serial.printf("[S3][Phase1] telemetry_topic=%s\n", g_topic_buffer);
  } else {
    Serial.println("[S3][Phase1] failed to build telemetry topic");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("[S3] Gateway firmware skeleton ready (Phase 1).");
  Serial.println(
      "[S3] TODO(Phase3/4): sensors, HTTP API, dashboard, MQTT bridge, command cache.");
}

void loop() { TickLog(); }
