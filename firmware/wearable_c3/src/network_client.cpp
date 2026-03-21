#include "network_client.h"

#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstring>

#include "json_keys.h"

namespace heatrisk {

namespace {

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

void SafeCopy(char* out, size_t out_size, const char* in) {
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

bool IsAckPacket(const char* payload) {
  if (payload == nullptr) {
    return false;
  }
  return strstr(payload, "HEATRISK_S3_ACK_V1") == payload;
}

bool ReadLineWithTimeout(WiFiClient* client, uint32_t timeout_ms, String* out) {
  if (client == nullptr || out == nullptr) {
    return false;
  }
  out->remove(0);
  const uint32_t start_ms = millis();
  while (millis() - start_ms < timeout_ms) {
    while (client->available() > 0) {
      const char c = static_cast<char>(client->read());
      if (c == '\n') {
        return out->length() > 0;
      }
      if (c == '\r') {
        continue;
      }
      out->concat(c);
    }
    if (!client->connected() && client->available() == 0) {
      break;
    }
    delay(1);
  }
  return out->length() > 0;
}

void LogNearbyAps() {
  const int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    Serial.println("[C3][WIFI] scan: no networks");
    return;
  }
  Serial.printf("[C3][WIFI] scan: found=%d\n", n);
  const int max_show = (n < 6) ? n : 6;
  for (int i = 0; i < max_show; ++i) {
    Serial.printf("[C3][WIFI] ap[%d] ssid=%s rssi=%d ch=%d enc=%d\n", i,
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
                  static_cast<int>(WiFi.encryptionType(i)));
  }
  WiFi.scanDelete();
}

}  // namespace

void WearableNetworkClient::begin(const char* ssid, const char* password,
                                  uint16_t gateway_port,
                                  uint16_t gateway_probe_port,
                                  const char* upload_path,
                                  const char* gateway_mdns_host,
                                  uint16_t gateway_discovery_port,
                                  uint16_t gateway_discovery_listen_port,
                                  const char* gateway_discovery_magic,
                                  uint32_t gateway_discovery_interval_ms,
                                  uint32_t gateway_probe_interval_ms,
                                  uint8_t gateway_probe_host_min,
                                  uint8_t gateway_probe_host_max,
                                  uint32_t gateway_settle_ms,
                                  uint32_t reconnect_interval_ms,
                                  uint16_t http_connect_timeout_ms,
                                  uint16_t http_read_timeout_ms) {
  ssid_ = ssid;
  password_ = password;
  port_ = gateway_port;
  probe_port_ = gateway_probe_port;
  path_ = upload_path;
  SafeCopy(gateway_mdns_host_, sizeof(gateway_mdns_host_), gateway_mdns_host);
  gateway_discovery_port_ = gateway_discovery_port;
  gateway_discovery_listen_port_ = gateway_discovery_listen_port;
  SafeCopy(gateway_discovery_magic_, sizeof(gateway_discovery_magic_),
           gateway_discovery_magic);
  gateway_discovery_interval_ms_ = gateway_discovery_interval_ms;
  gateway_probe_interval_ms_ = gateway_probe_interval_ms;
  gateway_probe_host_min_ = gateway_probe_host_min;
  gateway_probe_host_max_ = gateway_probe_host_max;
  gateway_settle_ms_ = gateway_settle_ms;
  reconnect_interval_ms_ = reconnect_interval_ms;
  http_connect_timeout_ms_ = http_connect_timeout_ms;
  http_read_timeout_ms_ = http_read_timeout_ms;
  last_reconnect_attempt_ms_ = millis();
  last_status_log_ms_ = 0;
  last_status_change_ms_ = last_reconnect_attempt_ms_;
  last_scan_log_ms_ = 0;
  last_discovery_ms_ = 0;
  last_gateway_probe_ms_ = 0;
  reconnect_attempt_count_ = 0;
  connect_ok_count_ = 0;
  connect_failed_count_ = 0;
  no_ssid_count_ = 0;
  disconnect_count_ = 0;
  connection_lost_count_ = 0;
  upload_failure_streak_ = 0;
  gateway_ip_set_ms_ = 0;
  mdns_started_ = false;
  udp_started_ = false;
  discovery_armed_ = true;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.begin(ssid_, password_);
  last_wifi_status_ = WiFi.status();
  current_wifi_status_ = last_wifi_status_;
  wifi_connected_since_ms_ =
      (last_wifi_status_ == WL_CONNECTED) ? last_reconnect_attempt_ms_ : 0;

  Serial.printf("[C3][WIFI] begin ssid=%s status=%s\n", ssid_,
                WifiStatusToString(last_wifi_status_));
  Serial.println("[C3][DISCOVERY] armed=1 (auto)");
}

bool WearableNetworkClient::ProbeGatewayByTcp(const IPAddress& ip) {
  WiFiClient client;
  client.setTimeout(200);
  const bool connected = client.connect(ip, probe_port_, 250);
  if (!connected) {
    client.stop();
    return false;
  }
  client.stop();
  SetGatewayIp(ip, "probe");
  return true;
}

void WearableNetworkClient::ProbeSubnetCandidates(uint32_t now_ms) {
  if (hasGatewayEndpoint()) {
    return;
  }
  if (now_ms - last_gateway_probe_ms_ < gateway_probe_interval_ms_) {
    return;
  }
  last_gateway_probe_ms_ = now_ms;

  const IPAddress local_ip = WiFi.localIP();
  if (!local_ip) {
    return;
  }

  const uint8_t self_host = local_ip[3];
  const uint8_t preferred_hosts[2] = {
      static_cast<uint8_t>(self_host + 1), static_cast<uint8_t>(self_host - 1)};

  for (uint8_t host : preferred_hosts) {
    if (host < gateway_probe_host_min_ || host > gateway_probe_host_max_ ||
        host == self_host) {
      continue;
    }
    IPAddress candidate(local_ip[0], local_ip[1], local_ip[2], host);
    if (ProbeGatewayByTcp(candidate)) {
      Serial.printf("[C3][DISCOVERY] tcp probe hit=%s\n",
                    candidate.toString().c_str());
      return;
    }
  }

  for (uint8_t host = gateway_probe_host_min_; host <= gateway_probe_host_max_;
       ++host) {
    if (host == self_host) {
      continue;
    }
    IPAddress candidate(local_ip[0], local_ip[1], local_ip[2], host);
    if (ProbeGatewayByTcp(candidate)) {
      Serial.printf("[C3][DISCOVERY] tcp probe hit=%s\n",
                    candidate.toString().c_str());
      return;
    }
  }
}

void WearableNetworkClient::SetGatewayIp(const IPAddress& ip,
                                         const char* source) {
  char ip_buf[16];
  snprintf(ip_buf, sizeof(ip_buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  if (strcmp(gateway_ip_, ip_buf) == 0) {
    return;
  }
  SafeCopy(gateway_ip_, sizeof(gateway_ip_), ip_buf);
  gateway_ip_set_ms_ = millis();
  Serial.printf("[C3][DISCOVERY] gateway ip=%s via=%s\n", gateway_ip_,
                source == nullptr ? "unknown" : source);
}

void WearableNetworkClient::ClearGatewayIp() {
  gateway_ip_[0] = '\0';
  gateway_ip_set_ms_ = 0;
}

bool WearableNetworkClient::hasGatewayEndpoint() const {
  return gateway_ip_[0] != '\0';
}

const char* WearableNetworkClient::currentGatewayIp() const {
  return hasGatewayEndpoint() ? gateway_ip_ : "";
}

void WearableNetworkClient::printWifiDiag() {
  const wl_status_t status = WiFi.status();
  const bool connected = (status == WL_CONNECTED);
  const String ip = connected ? WiFi.localIP().toString() : String("0.0.0.0");
  const int32_t rssi = connected ? WiFi.RSSI() : -127;

  Serial.printf(
      "[C3][DIAG] status=%s(%d) connected=%d ip=%s rssi=%ld ssid=%s gateway_ip=%s\n",
      WifiStatusToString(status), static_cast<int>(status), connected,
      ip.c_str(), static_cast<long>(rssi), ssid_ == nullptr ? "" : ssid_,
      currentGatewayIp());
  Serial.printf(
      "[C3][DIAG] counters reconnect=%lu connect_ok=%lu connect_failed=%lu "
      "no_ssid=%lu disconnected=%lu conn_lost=%lu\n",
      static_cast<unsigned long>(reconnect_attempt_count_),
      static_cast<unsigned long>(connect_ok_count_),
      static_cast<unsigned long>(connect_failed_count_),
      static_cast<unsigned long>(no_ssid_count_),
      static_cast<unsigned long>(disconnect_count_),
      static_cast<unsigned long>(connection_lost_count_));
  Serial.printf("[C3][DIAG] last_status_change_ms=%lu uptime_ms=%lu\n",
                static_cast<unsigned long>(last_status_change_ms_),
                static_cast<unsigned long>(millis()));
  Serial.printf("[C3][DIAG] discovery_armed=%d\n", discovery_armed_);
  if (connected) {
    Serial.println(
        "[C3][DIAG] skip AP scan while connected to avoid disrupting Wi-Fi");
  } else {
    Serial.println("[C3][DIAG] scanning nearby APs...");
    LogNearbyAps();
  }
}

void WearableNetworkClient::triggerGatewayDiscovery(
    bool clear_cached_gateway_ip) {
  if (clear_cached_gateway_ip) {
    ClearGatewayIp();
  }
  discovery_armed_ = true;
  last_discovery_ms_ = 0;  // 触发下一次 tick 立即尝试发现
  Serial.printf("[C3][DISCOVERY] armed=1 clear_ip=%d\n", clear_cached_gateway_ip);
}

bool WearableNetworkClient::isDiscoveryArmed() const { return discovery_armed_; }

bool WearableNetworkClient::setManualGatewayIp(const char* ip_string) {
  if (ip_string == nullptr || ip_string[0] == '\0') {
    return false;
  }
  IPAddress ip;
  if (!ip.fromString(ip_string)) {
    return false;
  }
  SetGatewayIp(ip, "manual");
  // 手动指定后默认关闭自动发现，避免被 mDNS/UDP 结果覆盖。
  discovery_armed_ = false;
  return true;
}

void WearableNetworkClient::PollDiscoveryReplies() {
  if (!udp_started_) {
    return;
  }
  while (true) {
    const int packet_size = discovery_udp_.parsePacket();
    if (packet_size <= 0) {
      return;
    }
    char payload[128];
    int n = discovery_udp_.read(payload, sizeof(payload) - 1);
    if (n <= 0) {
      continue;
    }
    payload[n] = '\0';
    if (!IsAckPacket(payload)) {
      continue;
    }
    const IPAddress remote_ip = discovery_udp_.remoteIP();
    SetGatewayIp(remote_ip, "udp");
  }
}

void WearableNetworkClient::TryDiscoverGateway(uint32_t now_ms) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!mdns_started_) {
    if (MDNS.begin("wearable-c3")) {
      mdns_started_ = true;
      Serial.println("[C3][DISCOVERY] mDNS client ready");
    }
  }

  if (!udp_started_) {
    if (discovery_udp_.begin(gateway_discovery_listen_port_)) {
      udp_started_ = true;
      Serial.printf("[C3][DISCOVERY] udp listen=%u\n",
                    gateway_discovery_listen_port_);
    }
  }

  PollDiscoveryReplies();
  if (hasGatewayEndpoint()) {
    return;
  }
  if (now_ms - last_discovery_ms_ < gateway_discovery_interval_ms_) {
    return;
  }
  last_discovery_ms_ = now_ms;

  if (mdns_started_ && gateway_mdns_host_[0] != '\0') {
    const IPAddress ip = MDNS.queryHost(gateway_mdns_host_, 300);
    if (ip) {
      SetGatewayIp(ip, "mdns");
      return;
    }
  }

  if (udp_started_ && gateway_discovery_magic_[0] != '\0') {
    const IPAddress broadcast_ip(255, 255, 255, 255);
    if (discovery_udp_.beginPacket(broadcast_ip, gateway_discovery_port_)) {
      discovery_udp_.write(
          reinterpret_cast<const uint8_t*>(gateway_discovery_magic_),
          strlen(gateway_discovery_magic_));
      discovery_udp_.endPacket();
      Serial.printf("[C3][DISCOVERY] udp discover sent port=%u\n",
                    gateway_discovery_port_);
    }
  }

  PollDiscoveryReplies();
  ProbeSubnetCandidates(now_ms);
}

void WearableNetworkClient::tick(uint32_t now_ms) {
  const wl_status_t status = WiFi.status();
  current_wifi_status_ = status;
  if (status != last_wifi_status_) {
    last_status_change_ms_ = now_ms;
    switch (status) {
      case WL_CONNECTED:
        ++connect_ok_count_;
        wifi_connected_since_ms_ = now_ms;
        break;
      case WL_CONNECT_FAILED:
        ++connect_failed_count_;
        wifi_connected_since_ms_ = 0;
        break;
      case WL_NO_SSID_AVAIL:
        ++no_ssid_count_;
        wifi_connected_since_ms_ = 0;
        break;
      case WL_DISCONNECTED:
        ++disconnect_count_;
        wifi_connected_since_ms_ = 0;
        break;
      case WL_CONNECTION_LOST:
        ++connection_lost_count_;
        wifi_connected_since_ms_ = 0;
        break;
      default:
        break;
    }
  }
  if (status != last_wifi_status_ || (now_ms - last_status_log_ms_ >= 5000)) {
    last_wifi_status_ = status;
    last_status_log_ms_ = now_ms;
    if (status == WL_CONNECTED) {
      Serial.printf("[C3][WIFI] connected ip=%s rssi=%d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      Serial.printf("[C3][WIFI] status=%s(%d)\n", WifiStatusToString(status),
                    static_cast<int>(status));
    }
  }

  if (status == WL_CONNECTED) {
    if (discovery_armed_) {
      TryDiscoverGateway(now_ms);
    }
    return;
  }

  if (mdns_started_) {
    MDNS.end();
    mdns_started_ = false;
  }
  if (udp_started_) {
    discovery_udp_.stop();
    udp_started_ = false;
  }

  if (status == WL_IDLE_STATUS || status == WL_SCAN_COMPLETED) {
    return;
  }

  if (now_ms - last_reconnect_attempt_ms_ >= reconnect_interval_ms_) {
    last_reconnect_attempt_ms_ = now_ms;
    ++reconnect_attempt_count_;
    // 连续重连失败时，重置 Wi-Fi 协议栈，避免长期卡死在异常状态。
    if ((reconnect_attempt_count_ % 3) == 0) {
      Serial.println("[C3][WIFI] hard restart wifi stack");
      WiFi.disconnect(false, false);
      WiFi.mode(WIFI_OFF);
      delay(30);
      WiFi.mode(WIFI_STA);
      WiFi.setSleep(false);
      WiFi.setAutoReconnect(true);
      WiFi.setTxPower(WIFI_POWER_19_5dBm);
      WiFi.begin(ssid_, password_);
    } else if (!WiFi.reconnect()) {
      WiFi.begin(ssid_, password_);
    }
    Serial.println("[C3][WIFI] reconnect attempt");
  }
}

bool WearableNetworkClient::isWifiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WearableNetworkClient::isWifiReadyForUplink(uint32_t now_ms) const {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  if (wifi_connected_since_ms_ == 0) {
    return false;
  }
  return (now_ms - wifi_connected_since_ms_) >= wifi_settle_ms_;
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
  if (!isWifiReadyForUplink(millis())) {
    return result;
  }
  if (!hasGatewayEndpoint()) {
    return result;
  }
  if (gateway_ip_set_ms_ == 0 ||
      (millis() - gateway_ip_set_ms_) < gateway_settle_ms_) {
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
  String response_payload;
  String last_error_msg;
  IPAddress gateway_ip;
  if (!gateway_ip.fromString(gateway_ip_)) {
    result.http_status = -1002;
    return result;
  }

  for (int attempt = 1; attempt <= 3; ++attempt) {
    WiFiClient client;
    client.setTimeout(http_read_timeout_ms_);
    const bool connected = client.connect(gateway_ip, port_, http_connect_timeout_ms_);
    if (!connected) {
      result.http_status = -1001;
      last_error_msg = "connect_failed";
      Serial.printf("[C3][TCP] connect_err attempt=%d target=%s:%u\n", attempt,
                    gateway_ip_, port_);
      if (attempt < 3) {
        delay(120);
      }
      continue;
    }

    result.request_sent = true;
    const size_t wrote = client.print(body);
    client.print('\n');
    if (wrote == 0) {
      result.http_status = -1003;
      last_error_msg = "write_failed";
      client.stop();
      if (attempt < 3) {
        delay(120);
      }
      continue;
    }

    if (!ReadLineWithTimeout(&client, http_read_timeout_ms_, &response_payload)) {
      result.http_status = -1004;
      last_error_msg = "read_timeout";
      Serial.printf("[C3][TCP] read_timeout attempt=%d target=%s:%u\n", attempt,
                    gateway_ip_, port_);
      client.stop();
      if (attempt < 3) {
        delay(120);
      }
      continue;
    }
    client.stop();

    result.http_status = 200;
    result.http_ok = true;
    upload_failure_streak_ = 0;
    break;
  }

  if (result.http_ok) {
    result.response_parsed =
        ParseOverrideFromResponse(response_payload, out_override);
  } else if (result.request_sent || last_error_msg.length() > 0) {
    ++upload_failure_streak_;
    Serial.printf("[C3][TCP] upload_err code=%d msg=%s streak=%u\n",
                  result.http_status, last_error_msg.c_str(),
                  upload_failure_streak_);
    if (upload_failure_streak_ >= 3) {
      // 热点/局域网偶发抖动时，保留最近一次已知网关地址，避免频繁退回未发现状态。
      // 同时重新触发发现链路，用于旁路校验/刷新网关地址。
      Serial.println("[C3][DISCOVERY] re-arm discovery after TCP errors");
      discovery_armed_ = true;
      last_discovery_ms_ = 0;
      last_gateway_probe_ms_ = 0;
      upload_failure_streak_ = 0;
    }
  }

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
