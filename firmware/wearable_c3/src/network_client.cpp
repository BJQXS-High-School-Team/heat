#include "network_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <cstring>

#include "json_keys.h"

namespace heatrisk {

void WearableNetworkClient::begin(const char* ssid, const char* password,
                                  const char* gateway_host,
                                  uint16_t gateway_port,
                                  const char* upload_path,
                                  uint32_t reconnect_interval_ms,
                                  uint16_t http_connect_timeout_ms,
                                  uint16_t http_read_timeout_ms) {
  ssid_ = ssid;
  password_ = password;
  host_ = gateway_host;
  port_ = gateway_port;
  path_ = upload_path;
  reconnect_interval_ms_ = reconnect_interval_ms;
  http_connect_timeout_ms_ = http_connect_timeout_ms;
  http_read_timeout_ms_ = http_read_timeout_ms;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_, password_);
  last_reconnect_attempt_ms_ = millis();
}

void WearableNetworkClient::tick(uint32_t now_ms) {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  if (now_ms - last_reconnect_attempt_ms_ < reconnect_interval_ms_) {
    return;
  }
  last_reconnect_attempt_ms_ = now_ms;
  WiFi.disconnect();
  WiFi.begin(ssid_, password_);
}

bool WearableNetworkClient::isWifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

int32_t WearableNetworkClient::currentRssi() const {
  if (WiFi.status() != WL_CONNECTED) {
    return -127;
  }
  return WiFi.RSSI();
}

UploadResult WearableNetworkClient::uploadTelemetry(
    const WearableTelemetrySnapshot& snapshot, OverrideCommand* out_override) {
  UploadResult result;
  if (WiFi.status() != WL_CONNECTED || out_override == nullptr) {
    return result;
  }

  StaticJsonDocument<1536> doc;
  doc[jsonkeys::kSchemaVersion] = snapshot.schema_version;
  doc[jsonkeys::kDeviceId] = snapshot.device_id;
  doc[jsonkeys::kDeviceType] = snapshot.device_type;
  doc[jsonkeys::kSeq] = snapshot.seq;
  doc[jsonkeys::kDeviceUptimeMs] = snapshot.device_uptime_ms;

  JsonObject gps = doc.createNestedObject(jsonkeys::kGps);
  gps[jsonkeys::kLat] = snapshot.gps.lat;
  gps[jsonkeys::kLng] = snapshot.gps.lng;
  gps[jsonkeys::kHasFix] = snapshot.gps.has_fix;
  gps[jsonkeys::kSatellites] = snapshot.gps.satellites;

  JsonObject physiology = doc.createNestedObject(jsonkeys::kPhysiology);
  physiology[jsonkeys::kBodyTempC] = snapshot.physiology.body_temp_c;
  physiology[jsonkeys::kHeartRateBpm] = snapshot.physiology.heart_rate_bpm;
  physiology[jsonkeys::kSpo2Pct] = snapshot.physiology.spo2_pct;

  JsonObject motion = doc.createNestedObject(jsonkeys::kMotion);
  motion[jsonkeys::kActivityScore] = snapshot.motion.activity_score;
  motion[jsonkeys::kActivityLevel] = snapshot.motion.activity_level;

  JsonObject status = doc.createNestedObject(jsonkeys::kStatus);
  status[jsonkeys::kLocalState] = StateLevelToString(snapshot.status.local_state);
  status[jsonkeys::kEffectiveState] =
      StateLevelToString(snapshot.status.effective_state);
  status[jsonkeys::kAlarmActive] = snapshot.status.alarm_active;

  JsonObject sensor_health = doc.createNestedObject(jsonkeys::kSensorHealth);
  sensor_health[jsonkeys::kGpsOk] = snapshot.sensor_health.gps_ok;
  sensor_health[jsonkeys::kTempOk] = snapshot.sensor_health.temp_ok;
  sensor_health[jsonkeys::kMax30102Ok] = snapshot.sensor_health.max30102_ok;
  sensor_health[jsonkeys::kImuOk] = snapshot.sensor_health.imu_ok;
  sensor_health[jsonkeys::kOledOk] = snapshot.sensor_health.oled_ok;

  JsonObject network = doc.createNestedObject(jsonkeys::kNetwork);
  network[jsonkeys::kRssi] = snapshot.network.rssi;
  network[jsonkeys::kWifiConnected] = snapshot.network.wifi_connected;

  String body;
  serializeJson(doc, body);

  String url = "http://" + String(host_) + ":" + String(port_) + String(path_);
  HTTPClient http;
  http.setConnectTimeout(http_connect_timeout_ms_);
  http.setTimeout(http_read_timeout_ms_);
  if (!http.begin(url)) {
    return result;
  }

  http.addHeader("Content-Type", "application/json");
  result.request_sent = true;
  result.http_status = http.POST(body);
  result.http_ok = (result.http_status >= 200 && result.http_status < 300);

  if (result.http_ok) {
    const String response = http.getString();
    result.response_parsed = ParseOverrideFromResponse(response, out_override);
  }

  http.end();
  return result;
}

bool WearableNetworkClient::ParseOverrideFromResponse(
    const String& response_json, OverrideCommand* out_override) {
  if (out_override == nullptr) {
    return false;
  }

  StaticJsonDocument<768> doc;
  const DeserializationError err = deserializeJson(doc, response_json);
  if (err) {
    return false;
  }

  const JsonObject override_obj = doc[jsonkeys::kOverride];
  if (override_obj.isNull()) {
    return false;
  }

  out_override->enabled = override_obj[jsonkeys::kEnabled] | false;
  out_override->state =
      StateLevelFromString(override_obj[jsonkeys::kState] | "normal");
  out_override->vibrate = override_obj[jsonkeys::kVibrate] | false;
  out_override->buzzer = override_obj[jsonkeys::kBuzzer] | false;
  out_override->expires_in_s = override_obj[jsonkeys::kExpiresInS] | 0;
  out_override->issued_at_ms = millis();

  const char* reason = override_obj[jsonkeys::kReason] | "";
  strncpy(out_override->reason, reason, sizeof(out_override->reason) - 1);
  out_override->reason[sizeof(out_override->reason) - 1] = '\0';
  return true;
}

}  // namespace heatrisk
