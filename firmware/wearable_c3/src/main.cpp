#include <Arduino.h>
#include <WiFi.h>
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
uint32_t g_last_max30102_poll_ms = 0;
uint32_t g_last_state_log_ms = 0;
heatrisk::StateLevel g_last_logged_effective_state = heatrisk::StateLevel::kNormal;

char g_cmd_buffer[192] = {0};
size_t g_cmd_len = 0;
uint32_t g_cmd_last_input_ms = 0;
constexpr uint32_t kCmdAutoCommitMs = 1000;

const char* WifiStatusToString(const wl_status_t status) {
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

void PrintCommandHelp() {
  Serial.println("[C3][CMD] help:");
  Serial.println("[C3][CMD]   help | ?                    -> show help");
  Serial.println("[C3][CMD]   wifistatus                  -> show Wi-Fi/discovery state");
  Serial.println("[C3][CMD]   wifi_diag                   -> show detailed Wi-Fi diagnostics");
  Serial.println("[C3][CMD]   setgw <ipv4>                -> set gateway ip manually");
  Serial.println("[C3][CMD]   discover                    -> force immediate mDNS+UDP discovery");
  Serial.println("[C3][CMD]   rediscover                  -> clear cached gateway ip and rediscover");
  Serial.println("[C3][CMD] examples:");
  Serial.println("[C3][CMD]   wifistatus");
  Serial.println("[C3][CMD]   wifi_diag");
  Serial.println("[C3][CMD]   setgw 192.168.43.1");
  Serial.println("[C3][CMD]   discover");
  Serial.println("[C3][CMD]   rediscover");
  Serial.printf("[C3][CMD] configured_ssid=%s\n", wearable_config::kWifiSsid);
  Serial.println(
      "[C3][CMD] note: if your serial tool does not send Enter, command auto-commit after 1s.");
}

void PrintWifiStatusCommand() {
  const wl_status_t status = WiFi.status();
  const bool connected = (status == WL_CONNECTED);
  const String ip = connected ? WiFi.localIP().toString() : String("0.0.0.0");
  const int32_t rssi = connected ? WiFi.RSSI() : -127;
  Serial.printf(
      "[C3][CMD] wifistatus status=%s(%d) connected=%d ip=%s rssi=%ld cfg_ssid=%s gateway_ip=%s discovery_armed=%d\n",
      WifiStatusToString(status), static_cast<int>(status), connected,
      ip.c_str(), static_cast<long>(rssi), wearable_config::kWifiSsid,
      g_network.currentGatewayIp(), g_network.isDiscoveryArmed());
}

void HandleSerialCommand(const char* line) {
  if (line == nullptr || line[0] == '\0') {
    return;
  }

  char local[192];
  strncpy(local, line, sizeof(local) - 1);
  local[sizeof(local) - 1] = '\0';
  char* saveptr = nullptr;
  char* cmd = strtok_r(local, " \t", &saveptr);
  if (cmd == nullptr) {
    return;
  }

  if (strcmp(cmd, "wifistatus") == 0) {
    PrintWifiStatusCommand();
    return;
  }
  if (strcmp(cmd, "wifi_diag") == 0) {
    g_network.printWifiDiag();
    return;
  }
  if (strcmp(cmd, "setgw") == 0) {
    char* ip = strtok_r(nullptr, " \t", &saveptr);
    if (ip == nullptr) {
      Serial.println("[C3][CMD] usage: setgw <ipv4>");
      return;
    }
    if (g_network.setManualGatewayIp(ip)) {
      Serial.printf("[C3][CMD] setgw ok ip=%s\n", g_network.currentGatewayIp());
    } else {
      Serial.printf("[C3][CMD] setgw invalid ip=%s\n", ip);
    }
    return;
  }
  if (strcmp(cmd, "discover") == 0) {
    g_network.triggerGatewayDiscovery(false);
    return;
  }
  if (strcmp(cmd, "rediscover") == 0) {
    g_network.triggerGatewayDiscovery(true);
    return;
  }
  if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
    PrintCommandHelp();
    return;
  }

  Serial.printf("[C3][CMD] unknown command: %s\n", cmd);
}

void CommitCommandBuffer() {
  if (g_cmd_len == 0) {
    return;
  }
  g_cmd_buffer[g_cmd_len] = '\0';
  Serial.printf("[C3][CMD] recv: %s\n", g_cmd_buffer);
  HandleSerialCommand(g_cmd_buffer);
  g_cmd_len = 0;
}

void TickSerialCommandInput() {
  const uint32_t now_ms = millis();
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r' || c == ';') {
      CommitCommandBuffer();
      continue;
    }
    if (c == 0x08 || c == 0x7F) {  // Backspace / DEL
      if (g_cmd_len > 0) {
        --g_cmd_len;
      }
      continue;
    }
    if (static_cast<uint8_t>(c) >= 32 && g_cmd_len < sizeof(g_cmd_buffer) - 1) {
      g_cmd_buffer[g_cmd_len++] = c;
      g_cmd_last_input_ms = now_ms;
    }
  }

  // 某些串口工具不会发送换行，超时自动提交一条命令。
  if (g_cmd_len > 0 && (now_ms - g_cmd_last_input_ms) >= kCmdAutoCommitMs) {
    CommitCommandBuffer();
  }
}

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
  const bool gps_ok = wearable_config::kEnableGps ? g_gps_sensor.begin() : false;
  const bool imu_ok = g_imu_sensor.begin(&Wire);
  const bool max_ok = wearable_config::kEnableMax3010x
                          ? g_max30102_sensor.begin(
                                &Wire, wearable_config::kMax3010xLedBrightness,
                                wearable_config::kMax3010xSampleAverage,
                                wearable_config::kMax3010xLedMode,
                                wearable_config::kMax3010xSampleRate,
                                wearable_config::kMax3010xPulseWidth,
                                wearable_config::kMax3010xAdcRange,
                                wearable_config::kMax3010xPulseAmplitudeRed,
                                wearable_config::kMax3010xPulseAmplitudeIr)
                          : false;
  const bool oled_ok = g_display.begin(wearable_config::kOledI2cAddr);

  g_snapshot.sensor_health.temp_ok = temp_ok;
  g_snapshot.sensor_health.gps_ok = gps_ok;
  g_snapshot.sensor_health.imu_ok = imu_ok;
  g_snapshot.sensor_health.max30102_ok = max_ok;
  g_snapshot.sensor_health.oled_ok = oled_ok;

  Serial.printf(
      "[C3][INIT] temp=%d gps=%d imu=%d max3010x=%d oled=%d gps_en=%d "
      "max3010x_en=%d\n",
      temp_ok, gps_ok, imu_ok, max_ok, oled_ok, wearable_config::kEnableGps,
      wearable_config::kEnableMax3010x);

  if (!wearable_config::kEnableMax3010x) {
    Serial.println(
        "[C3][INIT] MAX30102 disabled by config for stability; HR/SpO2 rules are suspended.");
  }
}

void SetupNetwork() {
  g_network.begin(wearable_config::kWifiSsid, wearable_config::kWifiPassword,
                  wearable_config::kGatewayPort, wearable_config::kGatewayProbePort,
                  wearable_config::kUploadPath,
                  wearable_config::kGatewayMdnsHost,
                  wearable_config::kGatewayDiscoveryPort,
                  wearable_config::kGatewayDiscoveryListenPort,
                  wearable_config::kGatewayDiscoveryMagic,
                  wearable_config::kGatewayDiscoveryIntervalMs,
                  wearable_config::kGatewayProbeIntervalMs,
                  wearable_config::kGatewayProbeHostMin,
                  wearable_config::kGatewayProbeHostMax,
                  wearable_config::kGatewaySettleMs,
                  wearable_config::kWifiReconnectIntervalMs,
                  wearable_config::kHttpConnectTimeoutMs,
                  wearable_config::kHttpReadTimeoutMs);
}

void TickMax30102Fast(uint32_t now_ms) {
  if (!wearable_config::kEnableMax3010x) {
    g_snapshot.sensor_health.max30102_ok = false;
    return;
  }
  if (now_ms - g_last_max30102_poll_ms <
      wearable_config::kMax3010xPollIntervalMs) {
    return;
  }
  g_last_max30102_poll_ms = now_ms;

  g_max30102_sensor.sample(&g_snapshot.physiology);
  g_snapshot.sensor_health.max30102_ok = g_max30102_sensor.isHealthy();
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

  if (wearable_config::kEnableGps) {
    g_gps_sensor.sample(&g_snapshot.gps);
    g_snapshot.sensor_health.gps_ok = g_gps_sensor.isHealthy();
  } else {
    g_snapshot.gps.has_fix = false;
    g_snapshot.gps.satellites = 0;
    g_snapshot.sensor_health.gps_ok = false;
  }

  g_imu_sensor.sample(&g_snapshot.motion);
  g_snapshot.sensor_health.imu_ok = g_imu_sensor.isHealthy();

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
    if (!g_network.isWifiConnected()) {
      Serial.println("[C3][TCP] skipped upload (wifi disconnected)");
    } else if (!g_network.isWifiReadyForUplink(now_ms)) {
      Serial.println("[C3][TCP] skipped upload (wifi settling)");
    } else if (!g_network.hasGatewayEndpoint()) {
      Serial.println("[C3][TCP] skipped upload (gateway not discovered yet)");
    } else {
      Serial.println("[C3][TCP] skipped upload");
    }
    return;
  }

  if (!result.http_ok) {
    Serial.printf("[C3][TCP] upload failed, status=%d\n", result.http_status);
    return;
  }

  Serial.printf("[C3][TCP] upload ok, status=%d\n", result.http_status);
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
  Serial.printf("[C3][CFG] wifi use code config ssid=%s\n",
                wearable_config::kWifiSsid);
  Serial.println("[C3][DISCOVERY] auto mode enabled");
  PrintCommandHelp();

  Wire.begin(wearable_config::kPinI2cSda, wearable_config::kPinI2cScl,
             wearable_config::kI2cFrequencyHz);
  Wire.setTimeOut(wearable_config::kI2cTimeoutMs);
  g_alarm.begin();
  InitSnapshotBase();
  SetupSensors();
  SetupNetwork();
}

void loop() {
  const uint32_t now_ms = millis();

  TickSerialCommandInput();

  if (wearable_config::kEnableGps) {
    g_gps_sensor.poll();
  }
  g_network.tick(now_ms);
  TickMax30102Fast(now_ms);
  g_alarm.tick(now_ms);

  TickSample(now_ms);
  TickDisplay(now_ms);
  TickUpload(now_ms);
}
