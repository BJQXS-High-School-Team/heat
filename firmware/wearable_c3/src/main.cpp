#include <Arduino.h>
#include <Wire.h>

#include <cstring>

#include "alarms.h"
#include "config.h"
#include "display_oled.h"
#include "local_state.h"
#include "network_client.h"
#include "sensor_ds18b20.h"
#include "sensor_gps.h"
#include "sensor_mpu6050.h"
#include "sensors_max30102.h"

namespace {

HardwareSerial g_gps_serial(1);

heatrisk::Ds18b20Sensor g_temp_sensor(wearable_config::kPinDs18b20);
heatrisk::GpsSensor g_gps_sensor(g_gps_serial, wearable_config::kPinGpsRx,
                                 wearable_config::kPinGpsTx,
                                 wearable_config::kGpsBaud);
heatrisk::Mpu6050Sensor g_imu_sensor;
heatrisk::Max30102Sensor g_max30102_sensor;

heatrisk::OledDisplay g_display(wearable_config::kOledWidth,
                                wearable_config::kOledHeight,
                                wearable_config::kOledResetPin);
heatrisk::AlarmDriver g_alarm(wearable_config::kPinVibration,
                              wearable_config::kPinBuzzer);
heatrisk::WearableNetworkClient g_network;
heatrisk::LocalStateEngine g_state_engine(wearable_config::kThresholds);

heatrisk::WearableTelemetrySnapshot g_snapshot;
heatrisk::OverrideCommand g_override;
heatrisk::EffectiveDecision g_decision;

uint32_t g_last_sample_ms = 0;
uint32_t g_last_upload_ms = 0;
uint32_t g_last_display_ms = 0;
uint32_t g_last_state_log_ms = 0;
heatrisk::StateLevel g_last_logged_effective_state = heatrisk::StateLevel::kNormal;

void LogStateTransitionIfNeeded(uint32_t now_ms) {
  if (g_decision.status.effective_state == g_last_logged_effective_state &&
      (now_ms - g_last_state_log_ms) < 5000) {
    return;
  }
  g_last_state_log_ms = now_ms;
  g_last_logged_effective_state = g_decision.status.effective_state;
  Serial.printf(
      "[C3][STATE] local=%s effective=%s alarm=%d override=%d\n",
      heatrisk::StateLevelToString(g_decision.status.local_state),
      heatrisk::StateLevelToString(g_decision.status.effective_state),
      g_decision.status.alarm_active, g_decision.override_active);
}

void InitSnapshotBase() {
  g_snapshot.schema_version = heatrisk::kSchemaVersion;
  strncpy(g_snapshot.device_id, wearable_config::kDeviceId,
          sizeof(g_snapshot.device_id) - 1);
  g_snapshot.device_id[sizeof(g_snapshot.device_id) - 1] = '\0';
  strncpy(g_snapshot.device_type, wearable_config::kDeviceType,
          sizeof(g_snapshot.device_type) - 1);
  g_snapshot.device_type[sizeof(g_snapshot.device_type) - 1] = '\0';
}

void SetupSensors() {
  const bool temp_ok = g_temp_sensor.begin();
  const bool gps_ok = g_gps_sensor.begin();
  const bool imu_ok = g_imu_sensor.begin(&Wire);
  const bool max_ok = g_max30102_sensor.begin(&Wire);
  const bool oled_ok = g_display.begin(wearable_config::kOledI2cAddr);

  g_snapshot.sensor_health.temp_ok = temp_ok;
  g_snapshot.sensor_health.gps_ok = gps_ok;
  g_snapshot.sensor_health.imu_ok = imu_ok;
  g_snapshot.sensor_health.max30102_ok = max_ok;
  g_snapshot.sensor_health.oled_ok = oled_ok;

  Serial.printf("[C3][INIT] temp=%d gps=%d imu=%d max30102=%d oled=%d\n", temp_ok,
                gps_ok, imu_ok, max_ok, oled_ok);
}

void SetupNetwork() {
  g_network.begin(wearable_config::kWifiSsid, wearable_config::kWifiPassword,
                  wearable_config::kGatewayHost, wearable_config::kGatewayPort,
                  wearable_config::kUploadPath,
                  wearable_config::kWifiReconnectIntervalMs,
                  wearable_config::kHttpConnectTimeoutMs,
                  wearable_config::kHttpReadTimeoutMs);
}

void TickSample(uint32_t now_ms) {
  if (now_ms - g_last_sample_ms < wearable_config::kSampleIntervalMs) {
    return;
  }
  g_last_sample_ms = now_ms;

  float body_temp = g_snapshot.physiology.body_temp_c;
  g_temp_sensor.sample(&body_temp);
  g_snapshot.physiology.body_temp_c = body_temp;
  g_snapshot.sensor_health.temp_ok = g_temp_sensor.isHealthy();

  g_gps_sensor.sample(&g_snapshot.gps);
  g_snapshot.sensor_health.gps_ok = g_gps_sensor.isHealthy();

  g_imu_sensor.sample(&g_snapshot.motion);
  g_snapshot.sensor_health.imu_ok = g_imu_sensor.isHealthy();

  g_max30102_sensor.sample(&g_snapshot.physiology);
  g_snapshot.sensor_health.max30102_ok = g_max30102_sensor.isHealthy();

  g_snapshot.network.wifi_connected = g_network.isWifiConnected();
  g_snapshot.network.rssi = g_network.currentRssi();

  g_decision = g_state_engine.Evaluate(g_snapshot.physiology, g_snapshot.motion,
                                       g_snapshot.sensor_health, g_override,
                                       now_ms);
  g_snapshot.status = g_decision.status;

  g_alarm.setMode(g_decision.status.effective_state, g_decision.vibrate_enabled,
                  g_decision.buzzer_enabled);
  LogStateTransitionIfNeeded(now_ms);
}

void TickDisplay(uint32_t now_ms) {
  if (now_ms - g_last_display_ms < wearable_config::kDisplayIntervalMs) {
    return;
  }
  g_last_display_ms = now_ms;

  heatrisk::OledViewModel vm;
  vm.heart_rate_bpm = g_snapshot.physiology.heart_rate_bpm;
  vm.spo2_pct = g_snapshot.physiology.spo2_pct;
  vm.body_temp_c = g_snapshot.physiology.body_temp_c;
  vm.activity_score = g_snapshot.motion.activity_score;
  vm.gps_has_fix = g_snapshot.gps.has_fix;
  vm.gps_satellites = g_snapshot.gps.satellites;
  vm.local_state = g_snapshot.status.local_state;
  vm.effective_state = g_snapshot.status.effective_state;
  vm.alarm_active = g_snapshot.status.alarm_active;
  vm.wifi_connected = g_snapshot.network.wifi_connected;
  vm.override_active = g_decision.override_active;
  g_display.render(vm);
}

void TickUpload(uint32_t now_ms) {
  if (now_ms - g_last_upload_ms < wearable_config::kUploadIntervalMs) {
    return;
  }
  g_last_upload_ms = now_ms;

  g_snapshot.device_uptime_ms = now_ms;
  ++g_snapshot.seq;

  heatrisk::OverrideCommand incoming_override;
  const heatrisk::UploadResult result =
      g_network.uploadTelemetry(g_snapshot, &incoming_override);

  if (!result.request_sent) {
    Serial.println("[C3][HTTP] skipped upload (wifi disconnected)");
    return;
  }

  if (!result.http_ok) {
    Serial.printf("[C3][HTTP] upload failed, status=%d\n", result.http_status);
    return;
  }

  Serial.printf("[C3][HTTP] upload ok, status=%d\n", result.http_status);
  if (result.response_parsed) {
    g_override = incoming_override;
    Serial.printf("[C3][OVERRIDE] enabled=%d state=%s vibrate=%d buzzer=%d ttl=%lu reason=%s\n",
                  g_override.enabled, heatrisk::StateLevelToString(g_override.state),
                  g_override.vibrate, g_override.buzzer,
                  static_cast<unsigned long>(g_override.expires_in_s),
                  g_override.reason);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(120);
  Serial.println("[C3] boot");

  Wire.begin(wearable_config::kPinI2cSda, wearable_config::kPinI2cScl,
             wearable_config::kI2cFrequencyHz);
  g_alarm.begin();
  InitSnapshotBase();
  SetupSensors();
  SetupNetwork();
}

void loop() {
  const uint32_t now_ms = millis();

  g_gps_sensor.poll();
  g_network.tick(now_ms);
  g_alarm.tick(now_ms);

  TickSample(now_ms);
  TickDisplay(now_ms);
  TickUpload(now_ms);
}
