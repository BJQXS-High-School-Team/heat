#include "wearable_tcp_server.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#include "json_keys.h"

namespace heatrisk {

namespace {

constexpr size_t kMaxLineLen = 2304;

}  // namespace

WearableTcpServer::WearableTcpServer(uint16_t port, uint32_t read_timeout_ms,
                                     DeviceRegistry* registry,
                                     CommandCache* command_cache,
                                     GatewayTelemetrySnapshot* host_snapshot)
    : port_(port),
      read_timeout_ms_(read_timeout_ms),
      server_(port),
      registry_(registry),
      command_cache_(command_cache),
      host_snapshot_(host_snapshot) {}

void WearableTcpServer::begin() {
  server_.begin();
  server_.setNoDelay(true);
  Serial.printf("[S3][TCP] wearable server started at :%u\n", port_);
}

void WearableTcpServer::tick() {
  WiFiClient client = server_.available();
  if (!client) {
    return;
  }
  handleClient(&client);
  client.stop();
}

bool WearableTcpServer::readJsonLine(WiFiClient* client, String* out_line) const {
  if (client == nullptr || out_line == nullptr) {
    return false;
  }
  out_line->remove(0);

  const uint32_t start_ms = millis();
  while (millis() - start_ms < read_timeout_ms_) {
    while (client->available() > 0) {
      const char ch = static_cast<char>(client->read());
      if (ch == '\n') {
        return out_line->length() > 0;
      }
      if (ch == '\r') {
        continue;
      }
      if (out_line->length() < kMaxLineLen) {
        out_line->concat(ch);
      } else {
        return false;
      }
    }
    if (!client->connected() && client->available() == 0) {
      break;
    }
    delay(1);
  }
  return out_line->length() > 0;
}

OverrideCommand WearableTcpServer::buildDefaultOverride() const {
  OverrideCommand cmd;
  cmd.enabled = false;
  cmd.state = StateLevel::kNormal;
  cmd.reason[0] = '\0';
  cmd.vibrate = false;
  cmd.buzzer = false;
  cmd.expires_in_s = 0;
  cmd.issued_at_ms = 0;
  return cmd;
}

void WearableTcpServer::writeOverrideJson(ArduinoJson::JsonObject obj,
                                          const OverrideCommand& command) const {
  obj[jsonkeys::kEnabled] = command.enabled;
  obj[jsonkeys::kState] = StateLevelToString(command.state);
  obj[jsonkeys::kReason] = command.reason;
  obj[jsonkeys::kVibrate] = command.vibrate;
  obj[jsonkeys::kBuzzer] = command.buzzer;
  obj[jsonkeys::kExpiresInS] = command.expires_in_s;
}

void WearableTcpServer::sendError(WiFiClient* client, const char* error_text) const {
  if (client == nullptr) {
    return;
  }
  StaticJsonDocument<192> doc;
  doc[jsonkeys::kOk] = false;
  doc["error"] = (error_text == nullptr) ? "unknown" : error_text;
  String out;
  serializeJson(doc, out);
  client->print(out);
  client->print('\n');
}

bool WearableTcpServer::parseWearablePayload(const String& body,
                                             WearableTelemetrySnapshot* out,
                                             String* out_error) const {
  if (out == nullptr || out_error == nullptr) {
    return false;
  }

  StaticJsonDocument<2048> doc;
  const DeserializationError err = deserializeJson(doc, body);
  if (err) {
    *out_error = "invalid_json";
    return false;
  }

  const char* device_id = doc[jsonkeys::kDeviceId] | "";
  const char* device_type = doc[jsonkeys::kDeviceType] | "";
  if (device_id[0] == '\0') {
    *out_error = "missing_device_id";
    return false;
  }
  if (strcmp(device_type, "wearable") != 0) {
    *out_error = "invalid_device_type";
    return false;
  }

  out->schema_version = doc[jsonkeys::kSchemaVersion] | 1;
  strncpy(out->device_id, device_id, sizeof(out->device_id) - 1);
  out->device_id[sizeof(out->device_id) - 1] = '\0';
  strncpy(out->device_type, device_type, sizeof(out->device_type) - 1);
  out->device_type[sizeof(out->device_type) - 1] = '\0';
  out->seq = doc[jsonkeys::kSeq] | 0;
  out->device_uptime_ms = doc[jsonkeys::kDeviceUptimeMs] | 0;

  JsonObject gps = doc[jsonkeys::kGps];
  out->gps.lat = gps[jsonkeys::kLat] | 0.0;
  out->gps.lng = gps[jsonkeys::kLng] | 0.0;
  out->gps.has_fix = gps[jsonkeys::kHasFix] | false;
  out->gps.satellites = gps[jsonkeys::kSatellites] | 0;

  JsonObject physiology = doc[jsonkeys::kPhysiology];
  out->physiology.body_temp_c = physiology[jsonkeys::kBodyTempC] | 0.0f;
  out->physiology.heart_rate_bpm = physiology[jsonkeys::kHeartRateBpm] | 0.0f;
  out->physiology.spo2_pct = physiology[jsonkeys::kSpo2Pct] | 0.0f;

  JsonObject motion = doc[jsonkeys::kMotion];
  out->motion.activity_score = motion[jsonkeys::kActivityScore] | 0.0f;
  const char* activity_level = motion[jsonkeys::kActivityLevel] | "unknown";
  strncpy(out->motion.activity_level, activity_level,
          sizeof(out->motion.activity_level) - 1);
  out->motion.activity_level[sizeof(out->motion.activity_level) - 1] = '\0';

  JsonObject status = doc[jsonkeys::kStatus];
  out->status.local_state =
      StateLevelFromString(status[jsonkeys::kLocalState] | "normal");
  out->status.effective_state =
      StateLevelFromString(status[jsonkeys::kEffectiveState] | "normal");
  out->status.alarm_active = status[jsonkeys::kAlarmActive] | false;

  JsonObject health = doc[jsonkeys::kSensorHealth];
  out->sensor_health.gps_ok = health[jsonkeys::kGpsOk] | false;
  out->sensor_health.temp_ok = health[jsonkeys::kTempOk] | false;
  out->sensor_health.max30102_ok = health[jsonkeys::kMax30102Ok] | false;
  out->sensor_health.imu_ok = health[jsonkeys::kImuOk] | false;
  out->sensor_health.oled_ok = health[jsonkeys::kOledOk] | false;

  JsonObject network = doc[jsonkeys::kNetwork];
  out->network.rssi = network[jsonkeys::kRssi] | -127;
  out->network.wifi_connected = network[jsonkeys::kWifiConnected] | false;

  return true;
}

void WearableTcpServer::handleClient(WiFiClient* client) {
  if (client == nullptr) {
    return;
  }

  String request_line;
  if (!readJsonLine(client, &request_line)) {
    sendError(client, "missing_body");
    Serial.println("[S3][TCP] upload missing_body");
    return;
  }

  WearableTelemetrySnapshot snapshot;
  String parse_error;
  if (!parseWearablePayload(request_line, &snapshot, &parse_error)) {
    sendError(client, parse_error.c_str());
    Serial.printf("[S3][TCP] upload parse_error=%s len=%u\n", parse_error.c_str(),
                  static_cast<unsigned>(request_line.length()));
    return;
  }

  const uint32_t now_ms = millis();
  if (!registry_->upsert(snapshot, now_ms)) {
    sendError(client, "registry_full");
    Serial.printf("[S3][TCP] upload registry_full device=%s\n", snapshot.device_id);
    return;
  }

  OverrideCommand override_cmd = buildDefaultOverride();
  if (!command_cache_->getForDevice(snapshot.device_id, now_ms, &override_cmd)) {
    override_cmd = buildDefaultOverride();
  }

  host_snapshot_->gateway_time_ms = now_ms;
  host_snapshot_->gateway_receive_epoch_ms = 0;

  StaticJsonDocument<512> resp;
  resp[jsonkeys::kOk] = true;
  resp[jsonkeys::kGatewayTimeMs] = now_ms;
  resp[jsonkeys::kGatewayReceiveEpochMs] = host_snapshot_->gateway_receive_epoch_ms;
  JsonObject override_json = resp.createNestedObject(jsonkeys::kOverride);
  writeOverrideJson(override_json, override_cmd);

  String out;
  serializeJson(resp, out);
  client->print(out);
  client->print('\n');
  Serial.printf("[S3][TCP] upload ok device=%s seq=%lu state=%s\n", snapshot.device_id,
                static_cast<unsigned long>(snapshot.seq),
                StateLevelToString(snapshot.status.local_state));
}

}  // namespace heatrisk

