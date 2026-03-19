#include "mqtt_bridge.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include <stdio.h>
#include <string.h>

#include "json_keys.h"
#include "topic_format.h"

namespace heatrisk {

MqttBridge* MqttBridge::s_instance_ = nullptr;

MqttBridge::MqttBridge(DeviceRegistry* registry, CommandCache* command_cache,
                       GatewayTelemetrySnapshot* host_snapshot)
    : registry_(registry),
      command_cache_(command_cache),
      host_snapshot_(host_snapshot),
      mqtt_client_(wifi_client_) {}

void MqttBridge::CopySafe(char* out, size_t out_size, const char* in) {
  if (out == nullptr || out_size == 0) {
    return;
  }
  if (in == nullptr) {
    out[0] = '\0';
    return;
  }
  strncpy(out, in, out_size - 1);
  out[out_size - 1] = '\0';
}

bool MqttBridge::ParseStateStrict(const char* value, StateLevel* out_state) {
  if (value == nullptr || out_state == nullptr) {
    return false;
  }
  if (strcmp(value, "normal") == 0) {
    *out_state = StateLevel::kNormal;
    return true;
  }
  if (strcmp(value, "warning") == 0) {
    *out_state = StateLevel::kWarning;
    return true;
  }
  if (strcmp(value, "danger") == 0) {
    *out_state = StateLevel::kDanger;
    return true;
  }
  return false;
}

void MqttBridge::begin(const char* site_id, const char* gateway_id,
                       const char* mqtt_host, uint16_t mqtt_port,
                       const char* mqtt_username, const char* mqtt_password,
                       uint32_t reconnect_interval_ms,
                       uint32_t publish_interval_ms,
                       uint16_t packet_buffer_size, bool bemfa_mode,
                       const char* bemfa_client_id,
                       const char* bemfa_topic_gateway,
                       const char* bemfa_topic_wearable_telemetry,
                       const char* bemfa_topic_wearable_status,
                       const char* bemfa_topic_command) {
  CopySafe(site_id_, sizeof(site_id_), site_id);
  CopySafe(gateway_id_, sizeof(gateway_id_), gateway_id);
  CopySafe(mqtt_host_, sizeof(mqtt_host_), mqtt_host);
  CopySafe(mqtt_username_, sizeof(mqtt_username_), mqtt_username);
  CopySafe(mqtt_password_, sizeof(mqtt_password_), mqtt_password);
  bemfa_mode_ = bemfa_mode;
  CopySafe(bemfa_client_id_, sizeof(bemfa_client_id_), bemfa_client_id);
  CopySafe(bemfa_topic_gateway_, sizeof(bemfa_topic_gateway_),
           bemfa_topic_gateway);
  CopySafe(bemfa_topic_wearable_telemetry_,
           sizeof(bemfa_topic_wearable_telemetry_),
           bemfa_topic_wearable_telemetry);
  CopySafe(bemfa_topic_wearable_status_, sizeof(bemfa_topic_wearable_status_),
           bemfa_topic_wearable_status);
  CopySafe(bemfa_topic_command_, sizeof(bemfa_topic_command_),
           bemfa_topic_command);

  mqtt_port_ = mqtt_port;
  reconnect_interval_ms_ = reconnect_interval_ms;
  publish_interval_ms_ = publish_interval_ms;
  last_reconnect_attempt_ms_ = 0;
  last_publish_ms_ = 0;

  mqtt_client_.setServer(mqtt_host_, mqtt_port_);
  mqtt_client_.setBufferSize(packet_buffer_size);
  mqtt_client_.setKeepAlive(30);
  mqtt_client_.setCallback(StaticOnMessage);
  s_instance_ = this;

  if (bemfa_mode_) {
    CopySafe(gateway_topic_, sizeof(gateway_topic_), bemfa_topic_gateway_);
    CopySafe(command_wildcard_topic_, sizeof(command_wildcard_topic_),
             bemfa_topic_command_);
  } else {
    BuildGatewayTelemetryTopic(gateway_topic_, sizeof(gateway_topic_), site_id_,
                               gateway_id_);
    snprintf(command_wildcard_topic_, sizeof(command_wildcard_topic_),
             "heatrisk/site/%s/command/+", site_id_);
  }

  const uint32_t chip_id = static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFFFF);
  snprintf(client_id_, sizeof(client_id_), "s3-%s-%08lx", gateway_id_,
           static_cast<unsigned long>(chip_id));
}

bool MqttBridge::isConnected() { return mqtt_client_.connected(); }

bool MqttBridge::BuildPublishTopic(const char* base_topic, bool use_set,
                                   char* out, size_t out_size) const {
  if (out == nullptr || out_size == 0 || base_topic == nullptr ||
      base_topic[0] == '\0') {
    return false;
  }
  if (!bemfa_mode_) {
    CopySafe(out, out_size, base_topic);
    return true;
  }

  const int n = snprintf(out, out_size, "%s/%s", base_topic,
                         use_set ? "set" : "up");
  return n > 0 && static_cast<size_t>(n) < out_size;
}

bool MqttBridge::IsBemfaCommandTopic(const char* topic) const {
  if (!bemfa_mode_ || topic == nullptr || bemfa_topic_command_[0] == '\0') {
    return false;
  }
  if (strcmp(topic, bemfa_topic_command_) == 0) {
    return true;
  }
  char set_topic[96];
  const int n =
      snprintf(set_topic, sizeof(set_topic), "%s/set", bemfa_topic_command_);
  return n > 0 && strcmp(topic, set_topic) == 0;
}

void MqttBridge::tryConnect(uint32_t now_ms) {
  if (mqtt_client_.connected()) {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (now_ms - last_reconnect_attempt_ms_ < reconnect_interval_ms_) {
    return;
  }
  last_reconnect_attempt_ms_ = now_ms;

  bool ok = false;
  if (bemfa_mode_ && bemfa_client_id_[0] != '\0') {
    ok = mqtt_client_.connect(bemfa_client_id_);
  } else if (mqtt_username_[0] != '\0') {
    ok = mqtt_client_.connect(client_id_, mqtt_username_, mqtt_password_);
  } else {
    ok = mqtt_client_.connect(client_id_);
  }

  if (!ok) {
    Serial.printf("[S3][MQTT] connect failed, state=%d\n", mqtt_client_.state());
    return;
  }

  bool sub_ok = mqtt_client_.subscribe(command_wildcard_topic_, 1);
  if (bemfa_mode_) {
    char set_topic[96];
    const int n = snprintf(set_topic, sizeof(set_topic), "%s/set",
                           command_wildcard_topic_);
    if (n > 0 && static_cast<size_t>(n) < sizeof(set_topic)) {
      sub_ok = mqtt_client_.subscribe(set_topic, 1) && sub_ok;
    }
  }
  Serial.printf("[S3][MQTT] connected, subscribe=%d topic=%s\n", sub_ok,
                command_wildcard_topic_);
}

void MqttBridge::publishGatewayTelemetry() {
  if (!mqtt_client_.connected()) {
    return;
  }

  StaticJsonDocument<768> doc;
  doc[jsonkeys::kSchemaVersion] = host_snapshot_->schema_version;
  doc[jsonkeys::kSiteId] = host_snapshot_->site_id;
  doc[jsonkeys::kGatewayId] = host_snapshot_->gateway_id;
  doc[jsonkeys::kGatewayTimeMs] = host_snapshot_->gateway_time_ms;
  doc[jsonkeys::kGatewayReceiveEpochMs] = host_snapshot_->gateway_receive_epoch_ms;

  JsonObject ambient = doc.createNestedObject(jsonkeys::kGatewayAmbient);
  ambient[jsonkeys::kTemperatureC] = host_snapshot_->gateway_ambient.temperature_c;
  ambient[jsonkeys::kHumidityPct] = host_snapshot_->gateway_ambient.humidity_pct;
  ambient[jsonkeys::kLightLux] = host_snapshot_->gateway_ambient.light_lux;

  JsonObject health = doc.createNestedObject(jsonkeys::kSensorHealth);
  health[jsonkeys::kSht35Ok] = host_snapshot_->sensor_health.sht35_ok;
  health[jsonkeys::kBh1750Ok] = host_snapshot_->sensor_health.bh1750_ok;

  JsonObject network = doc.createNestedObject(jsonkeys::kNetwork);
  network[jsonkeys::kWifiConnected] = host_snapshot_->network.wifi_connected;
  network[jsonkeys::kMqttConnected] = host_snapshot_->network.mqtt_connected;
  network[jsonkeys::kRssi] = host_snapshot_->network.rssi;

  String payload;
  serializeJson(doc, payload);
  char publish_topic[96];
  if (!BuildPublishTopic(gateway_topic_, false, publish_topic,
                         sizeof(publish_topic))) {
    return;
  }
  mqtt_client_.publish(publish_topic, payload.c_str(), false);
}

void MqttBridge::publishWearableTelemetryAndStatus(uint32_t now_ms) {
  if (!mqtt_client_.connected()) {
    return;
  }

  char telemetry_topic[kMaxTopicLen];
  char status_topic[kMaxTopicLen];
  for (uint8_t i = 0; i < registry_->capacity(); ++i) {
    const DeviceRecord* record = registry_->at(i);
    if (record == nullptr) {
      continue;
    }

    if (bemfa_mode_) {
      CopySafe(telemetry_topic, sizeof(telemetry_topic),
               bemfa_topic_wearable_telemetry_);
      CopySafe(status_topic, sizeof(status_topic), bemfa_topic_wearable_status_);
      if (telemetry_topic[0] == '\0' || status_topic[0] == '\0') {
        continue;
      }
    } else {
      if (!BuildWearableTelemetryTopic(telemetry_topic, sizeof(telemetry_topic),
                                       site_id_, record->device_id) ||
          !BuildWearableStatusTopic(status_topic, sizeof(status_topic), site_id_,
                                    record->device_id)) {
        continue;
      }
    }

    StaticJsonDocument<1024> telemetry;
    telemetry[jsonkeys::kSchemaVersion] = record->snapshot.schema_version;
    telemetry[jsonkeys::kSiteId] = site_id_;
    telemetry[jsonkeys::kGatewayId] = gateway_id_;
    telemetry[jsonkeys::kGatewayTimeMs] = now_ms;
    telemetry[jsonkeys::kDeviceId] = record->snapshot.device_id;
    telemetry[jsonkeys::kSeq] = record->snapshot.seq;
    telemetry[jsonkeys::kDeviceUptimeMs] = record->snapshot.device_uptime_ms;

    JsonObject gps = telemetry.createNestedObject(jsonkeys::kGps);
    gps[jsonkeys::kLat] = record->snapshot.gps.lat;
    gps[jsonkeys::kLng] = record->snapshot.gps.lng;
    gps[jsonkeys::kHasFix] = record->snapshot.gps.has_fix;
    gps[jsonkeys::kSatellites] = record->snapshot.gps.satellites;

    JsonObject physiology = telemetry.createNestedObject(jsonkeys::kPhysiology);
    physiology[jsonkeys::kBodyTempC] = record->snapshot.physiology.body_temp_c;
    physiology[jsonkeys::kHeartRateBpm] =
        record->snapshot.physiology.heart_rate_bpm;
    physiology[jsonkeys::kSpo2Pct] = record->snapshot.physiology.spo2_pct;

    JsonObject motion = telemetry.createNestedObject(jsonkeys::kMotion);
    motion[jsonkeys::kActivityScore] = record->snapshot.motion.activity_score;
    motion[jsonkeys::kActivityLevel] = record->snapshot.motion.activity_level;

    JsonObject status = telemetry.createNestedObject(jsonkeys::kStatus);
    status[jsonkeys::kLocalState] =
        StateLevelToString(record->snapshot.status.local_state);
    status[jsonkeys::kEffectiveState] =
        StateLevelToString(record->snapshot.status.effective_state);
    status[jsonkeys::kAlarmActive] = record->snapshot.status.alarm_active;

    String telemetry_payload;
    serializeJson(telemetry, telemetry_payload);
    char telemetry_publish_topic[96];
    if (BuildPublishTopic(telemetry_topic, false, telemetry_publish_topic,
                          sizeof(telemetry_publish_topic))) {
      mqtt_client_.publish(telemetry_publish_topic, telemetry_payload.c_str(),
                           false);
    }

    StaticJsonDocument<512> status_doc;
    status_doc[jsonkeys::kSchemaVersion] = record->snapshot.schema_version;
    status_doc[jsonkeys::kSiteId] = site_id_;
    status_doc[jsonkeys::kGatewayId] = gateway_id_;
    status_doc[jsonkeys::kDeviceId] = record->snapshot.device_id;
    JsonObject st = status_doc.createNestedObject(jsonkeys::kStatus);
    st[jsonkeys::kLocalState] = StateLevelToString(record->snapshot.status.local_state);
    st[jsonkeys::kEffectiveState] =
        StateLevelToString(record->snapshot.status.effective_state);
    st[jsonkeys::kAlarmActive] = record->snapshot.status.alarm_active;
    JsonObject fresh = status_doc.createNestedObject("freshness");
    fresh["online"] = record->online;
    fresh["last_seen_ms_ago"] = now_ms - record->last_seen_ms;

    String status_payload;
    serializeJson(status_doc, status_payload);
    char status_publish_topic[96];
    if (BuildPublishTopic(status_topic, false, status_publish_topic,
                          sizeof(status_publish_topic))) {
      mqtt_client_.publish(status_publish_topic, status_payload.c_str(), false);
    }
  }
}

void MqttBridge::publishIfDue(uint32_t now_ms) {
  if (!mqtt_client_.connected()) {
    return;
  }
  if (now_ms - last_publish_ms_ < publish_interval_ms_) {
    return;
  }
  last_publish_ms_ = now_ms;
  publishGatewayTelemetry();
  publishWearableTelemetryAndStatus(now_ms);
}

void MqttBridge::tick(uint32_t now_ms) {
  tryConnect(now_ms);
  if (mqtt_client_.connected()) {
    mqtt_client_.loop();
    publishIfDue(now_ms);
  }
  if (host_snapshot_ != nullptr) {
    host_snapshot_->network.mqtt_connected = mqtt_client_.connected();
  }
}

void MqttBridge::StaticOnMessage(char* topic, uint8_t* payload,
                                 unsigned int length) {
  if (s_instance_ == nullptr) {
    return;
  }
  s_instance_->onMessage(topic, payload, length);
}

void MqttBridge::onMessage(char* topic, uint8_t* payload, unsigned int length) {
  if (topic == nullptr || payload == nullptr || length == 0) {
    return;
  }
  String payload_str;
  payload_str.reserve(length + 1);
  for (unsigned int i = 0; i < length; ++i) {
    payload_str += static_cast<char>(payload[i]);
  }
  applyCommandPayload(topic, payload_str.c_str(), millis());
}

void MqttBridge::applyCommandPayload(const char* topic, const char* payload_json,
                                     uint32_t now_ms) {
  if (topic == nullptr || payload_json == nullptr) {
    return;
  }

  if (bemfa_mode_ && IsBemfaCommandTopic(topic)) {
    StaticJsonDocument<768> doc;
    const DeserializationError err = deserializeJson(doc, payload_json);
    if (err) {
      Serial.println("[S3][MQTT] invalid bemfa command JSON");
      return;
    }

    const char* state_str = doc[jsonkeys::kState] | "normal";
    StateLevel state;
    if (!ParseStateStrict(state_str, &state)) {
      Serial.printf("[S3][MQTT] invalid state=%s\n", state_str);
      return;
    }

    OverrideCommand cmd;
    cmd.enabled = doc[jsonkeys::kEnabled] | false;
    cmd.state = state;
    cmd.vibrate = doc[jsonkeys::kVibrate] | false;
    cmd.buzzer = doc[jsonkeys::kBuzzer] | false;
    cmd.expires_in_s = doc[jsonkeys::kExpiresInS] | 0;
    if (cmd.expires_in_s > 3600) {
      cmd.expires_in_s = 3600;
    }
    cmd.issued_at_ms = now_ms;
    CopySafe(cmd.reason, sizeof(cmd.reason), doc[jsonkeys::kReason] | "");

    const char* target = doc[jsonkeys::kTargetDeviceId] | "";
    if (target[0] != '\0') {
      command_cache_->setForDevice(target, cmd, now_ms);
      Serial.printf("[S3][MQTT] bemfa cmd set for %s\n", target);
    } else {
      command_cache_->setGlobal(cmd, now_ms);
      Serial.println("[S3][MQTT] bemfa cmd set global");
    }
    return;
  }

  const char* marker = strstr(topic, "/command/");
  if (marker == nullptr) {
    return;
  }
  const char* topic_target = marker + strlen("/command/");
  if (topic_target[0] == '\0') {
    return;
  }

  StaticJsonDocument<768> doc;
  const DeserializationError err = deserializeJson(doc, payload_json);
  if (err) {
    Serial.println("[S3][MQTT] invalid command JSON");
    return;
  }

  const char* state_str = doc[jsonkeys::kState] | "normal";
  StateLevel state;
  if (!ParseStateStrict(state_str, &state)) {
    Serial.printf("[S3][MQTT] invalid state=%s\n", state_str);
    return;
  }

  OverrideCommand cmd;
  cmd.enabled = doc[jsonkeys::kEnabled] | false;
  cmd.state = state;
  cmd.vibrate = doc[jsonkeys::kVibrate] | false;
  cmd.buzzer = doc[jsonkeys::kBuzzer] | false;
  cmd.expires_in_s = doc[jsonkeys::kExpiresInS] | 0;
  if (cmd.expires_in_s > 3600) {
    cmd.expires_in_s = 3600;
  }
  cmd.issued_at_ms = now_ms;
  CopySafe(cmd.reason, sizeof(cmd.reason), doc[jsonkeys::kReason] | "");

  const char* payload_target = doc[jsonkeys::kTargetDeviceId] | "";
  if (strcmp(topic_target, "all") == 0) {
    if (payload_target[0] != '\0') {
      command_cache_->setForDevice(payload_target, cmd, now_ms);
      Serial.printf("[S3][MQTT] cmd set for %s (from all)\n", payload_target);
    } else {
      command_cache_->setGlobal(cmd, now_ms);
      Serial.println("[S3][MQTT] cmd set global");
    }
  } else {
    command_cache_->setForDevice(topic_target, cmd, now_ms);
    Serial.printf("[S3][MQTT] cmd set for %s (from topic)\n", topic_target);
  }
}

}  // namespace heatrisk
