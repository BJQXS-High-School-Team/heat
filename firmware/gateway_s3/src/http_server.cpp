#include "http_server.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#include "dashboard_page.h"
#include "json_keys.h"

namespace heatrisk {

GatewayHttpServer::GatewayHttpServer(uint16_t port, DeviceRegistry* registry,
                                     CommandCache* command_cache,
                                     GatewayTelemetrySnapshot* host_snapshot)
    : server_(port),
      registry_(registry),
      command_cache_(command_cache),
      host_snapshot_(host_snapshot) {}

void GatewayHttpServer::begin() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/health", HTTP_GET, [this]() { handleHealth(); });
  server_.on("/api/host", HTTP_GET, [this]() { handleHost(); });
  server_.on("/api/devices", HTTP_GET, [this]() { handleDevices(); });
  server_.on("/api/device", HTTP_GET, [this]() { handleDevice(); });
  server_.on("/api/wearable/upload", HTTP_POST, [this]() { handleWearableUpload(); });
  server_.begin();
}

void GatewayHttpServer::tick() { server_.handleClient(); }

OverrideCommand GatewayHttpServer::buildDefaultOverride() const {
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

void GatewayHttpServer::writeOverrideJson(ArduinoJson::JsonObject obj,
                                          const OverrideCommand& command) {
  obj[jsonkeys::kEnabled] = command.enabled;
  obj[jsonkeys::kState] = StateLevelToString(command.state);
  obj[jsonkeys::kReason] = command.reason;
  obj[jsonkeys::kVibrate] = command.vibrate;
  obj[jsonkeys::kBuzzer] = command.buzzer;
  obj[jsonkeys::kExpiresInS] = command.expires_in_s;
}

void GatewayHttpServer::writeDeviceJson(ArduinoJson::JsonObject obj,
                                        const DeviceRecord& record,
                                        uint32_t now_ms) {
  obj[jsonkeys::kDeviceId] = record.device_id;
  obj["online"] = record.online;
  obj["last_seen_ms_ago"] = now_ms - record.last_seen_ms;
  obj[jsonkeys::kSeq] = record.snapshot.seq;
  obj[jsonkeys::kDeviceUptimeMs] = record.snapshot.device_uptime_ms;

  JsonObject gps = obj.createNestedObject(jsonkeys::kGps);
  gps[jsonkeys::kLat] = record.snapshot.gps.lat;
  gps[jsonkeys::kLng] = record.snapshot.gps.lng;
  gps[jsonkeys::kHasFix] = record.snapshot.gps.has_fix;
  gps[jsonkeys::kSatellites] = record.snapshot.gps.satellites;

  JsonObject physiology = obj.createNestedObject(jsonkeys::kPhysiology);
  physiology[jsonkeys::kBodyTempC] = record.snapshot.physiology.body_temp_c;
  physiology[jsonkeys::kHeartRateBpm] = record.snapshot.physiology.heart_rate_bpm;
  physiology[jsonkeys::kSpo2Pct] = record.snapshot.physiology.spo2_pct;

  JsonObject motion = obj.createNestedObject(jsonkeys::kMotion);
  motion[jsonkeys::kActivityScore] = record.snapshot.motion.activity_score;
  motion[jsonkeys::kActivityLevel] = record.snapshot.motion.activity_level;

  JsonObject status = obj.createNestedObject(jsonkeys::kStatus);
  status[jsonkeys::kLocalState] =
      StateLevelToString(record.snapshot.status.local_state);
  status[jsonkeys::kEffectiveState] =
      StateLevelToString(record.snapshot.status.effective_state);
  status[jsonkeys::kAlarmActive] = record.snapshot.status.alarm_active;

  JsonObject sensor_health = obj.createNestedObject(jsonkeys::kSensorHealth);
  sensor_health[jsonkeys::kGpsOk] = record.snapshot.sensor_health.gps_ok;
  sensor_health[jsonkeys::kTempOk] = record.snapshot.sensor_health.temp_ok;
  sensor_health[jsonkeys::kMax30102Ok] = record.snapshot.sensor_health.max30102_ok;
  sensor_health[jsonkeys::kImuOk] = record.snapshot.sensor_health.imu_ok;
  sensor_health[jsonkeys::kOledOk] = record.snapshot.sensor_health.oled_ok;

  JsonObject network = obj.createNestedObject(jsonkeys::kNetwork);
  network[jsonkeys::kRssi] = record.snapshot.network.rssi;
  network[jsonkeys::kWifiConnected] = record.snapshot.network.wifi_connected;
}

void GatewayHttpServer::handleRoot() {
  server_.send(200, "text/html; charset=utf-8", kDashboardHtml);
}

void GatewayHttpServer::handleHealth() {
  StaticJsonDocument<384> doc;
  doc[jsonkeys::kOk] = true;
  doc["uptime_ms"] = millis();
  doc[jsonkeys::kWifiConnected] = host_snapshot_->network.wifi_connected;
  doc[jsonkeys::kMqttConnected] = host_snapshot_->network.mqtt_connected;
  doc["heap_free"] = ESP.getFreeHeap();
  doc["device_count"] = registry_->count();

  String out;
  serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void GatewayHttpServer::handleHost() {
  StaticJsonDocument<512> doc;
  doc[jsonkeys::kOk] = true;
  doc[jsonkeys::kSchemaVersion] = host_snapshot_->schema_version;
  doc[jsonkeys::kSiteId] = host_snapshot_->site_id;
  doc[jsonkeys::kGatewayId] = host_snapshot_->gateway_id;
  doc[jsonkeys::kGatewayTimeMs] = host_snapshot_->gateway_time_ms;
  doc[jsonkeys::kGatewayReceiveEpochMs] = host_snapshot_->gateway_receive_epoch_ms;

  JsonObject ambient = doc.createNestedObject(jsonkeys::kGatewayAmbient);
  ambient[jsonkeys::kTemperatureC] = host_snapshot_->gateway_ambient.temperature_c;
  ambient[jsonkeys::kHumidityPct] = host_snapshot_->gateway_ambient.humidity_pct;
  ambient[jsonkeys::kLightLux] = host_snapshot_->gateway_ambient.light_lux;

  JsonObject sensor_health = doc.createNestedObject(jsonkeys::kSensorHealth);
  sensor_health[jsonkeys::kSht35Ok] = host_snapshot_->sensor_health.sht35_ok;
  sensor_health[jsonkeys::kBh1750Ok] = host_snapshot_->sensor_health.bh1750_ok;

  JsonObject network = doc.createNestedObject(jsonkeys::kNetwork);
  network[jsonkeys::kWifiConnected] = host_snapshot_->network.wifi_connected;
  network[jsonkeys::kMqttConnected] = host_snapshot_->network.mqtt_connected;
  network[jsonkeys::kRssi] = host_snapshot_->network.rssi;

  String out;
  serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void GatewayHttpServer::handleDevices() {
  const uint32_t now_ms = millis();
  DynamicJsonDocument doc(10240);
  doc[jsonkeys::kOk] = true;
  doc["gateway_time_ms"] = now_ms;
  doc["device_count"] = registry_->count();

  JsonArray arr = doc.createNestedArray("devices");
  for (uint8_t i = 0; i < registry_->capacity(); ++i) {
    const DeviceRecord* record = registry_->at(i);
    if (record == nullptr) {
      continue;
    }
    JsonObject item = arr.createNestedObject();
    writeDeviceJson(item, *record, now_ms);
  }

  String out;
  serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

void GatewayHttpServer::handleDevice() {
  if (!server_.hasArg("id")) {
    server_.send(400, "application/json", "{\"ok\":false,\"error\":\"missing id\"}");
    return;
  }

  const String id = server_.arg("id");
  const DeviceRecord* record = registry_->find(id.c_str());
  if (record == nullptr) {
    server_.send(404, "application/json", "{\"ok\":false,\"error\":\"device not found\"}");
    return;
  }

  StaticJsonDocument<1024> doc;
  doc[jsonkeys::kOk] = true;
  JsonObject device = doc.createNestedObject("device");
  writeDeviceJson(device, *record, millis());

  String out;
  serializeJson(doc, out);
  server_.send(200, "application/json", out);
}

bool GatewayHttpServer::parseWearablePayload(const String& body,
                                             WearableTelemetrySnapshot* out,
                                             String* out_error) {
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

void GatewayHttpServer::handleWearableUpload() {
  if (!server_.hasArg("plain")) {
    Serial.println("[S3][HTTP] upload missing_body");
    server_.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_body\"}");
    return;
  }

  WearableTelemetrySnapshot snapshot;
  String parse_error;
  if (!parseWearablePayload(server_.arg("plain"), &snapshot, &parse_error)) {
    Serial.printf("[S3][HTTP] upload parse_error=%s len=%u\n", parse_error.c_str(),
                  static_cast<unsigned>(server_.arg("plain").length()));
    StaticJsonDocument<256> err_doc;
    err_doc[jsonkeys::kOk] = false;
    err_doc["error"] = parse_error;
    String out;
    serializeJson(err_doc, out);
    server_.send(400, "application/json", out);
    return;
  }

  const uint32_t now_ms = millis();
  const bool upsert_ok = registry_->upsert(snapshot, now_ms);
  if (!upsert_ok) {
    Serial.printf("[S3][HTTP] upload registry_full device=%s\n", snapshot.device_id);
    server_.send(503, "application/json",
                 "{\"ok\":false,\"error\":\"registry_full\"}");
    return;
  }

  OverrideCommand override_cmd = buildDefaultOverride();
  if (!command_cache_->getForDevice(snapshot.device_id, now_ms, &override_cmd)) {
    override_cmd = buildDefaultOverride();
  }

  host_snapshot_->gateway_time_ms = now_ms;
  host_snapshot_->gateway_receive_epoch_ms = 0;  // Phase 3: 无实时时钟/NTP

  StaticJsonDocument<512> resp;
  resp[jsonkeys::kOk] = true;
  resp[jsonkeys::kGatewayTimeMs] = now_ms;
  resp[jsonkeys::kGatewayReceiveEpochMs] = host_snapshot_->gateway_receive_epoch_ms;
  JsonObject override_json = resp.createNestedObject(jsonkeys::kOverride);
  writeOverrideJson(override_json, override_cmd);

  String out;
  serializeJson(resp, out);
  Serial.printf("[S3][HTTP] upload ok device=%s seq=%lu state=%s\n", snapshot.device_id,
                static_cast<unsigned long>(snapshot.seq),
                StateLevelToString(snapshot.status.local_state));
  server_.send(200, "application/json", out);
}

}  // namespace heatrisk
