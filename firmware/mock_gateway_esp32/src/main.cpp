#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

namespace {

// 仅用于 C3 联调，不替代正式 S3 网关架构。
static constexpr const char* kWifiSsid = "blackmi";
static constexpr const char* kWifiPassword = "wxy358800";
static constexpr const char* kMdnsHost = "heatrisk-gateway";
static constexpr uint16_t kDiscoveryPort = 4210;
static constexpr const char* kDiscoveryMagic = "HEATRISK_DISCOVER_V1";
static constexpr const char* kDiscoveryAck = "HEATRISK_S3_ACK_V1";
static constexpr uint16_t kTcpPort = 9080;
static constexpr uint32_t kTcpReadTimeoutMs = 2000;

WiFiUDP g_udp;
WiFiServer g_tcp_server(kTcpPort);
WiFiClient g_client;
String g_tcp_line;
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

void StartWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(kWifiSsid, kWifiPassword);
  g_last_wifi_retry_ms = millis();
  g_last_wifi_status = WiFi.status();
  Serial.printf("[MOCK][WiFi] begin ssid=%s status=%s\n", kWifiSsid,
                WifiStatusToString(g_last_wifi_status));
}

void TickWifi(uint32_t now_ms) {
  const wl_status_t status = WiFi.status();
  if (status != g_last_wifi_status || (now_ms - g_last_wifi_log_ms) >= 5000) {
    g_last_wifi_status = status;
    g_last_wifi_log_ms = now_ms;
    if (status == WL_CONNECTED) {
      Serial.printf("[MOCK][WiFi] connected ip=%s rssi=%d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      Serial.printf("[MOCK][WiFi] status=%s(%d)\n", WifiStatusToString(status),
                    static_cast<int>(status));
    }
  }
  if (status == WL_CONNECTED) {
    return;
  }
  if (now_ms - g_last_wifi_retry_ms < 5000) {
    return;
  }
  g_last_wifi_retry_ms = now_ms;
  if (!WiFi.reconnect()) {
    WiFi.begin(kWifiSsid, kWifiPassword);
  }
}

void TickDiscovery() {
  const int packet_size = g_udp.parsePacket();
  if (packet_size <= 0) {
    return;
  }
  char payload[96];
  int n = g_udp.read(payload, sizeof(payload) - 1);
  if (n <= 0) {
    return;
  }
  payload[n] = '\0';
  if (strncmp(payload, kDiscoveryMagic, strlen(kDiscoveryMagic)) != 0) {
    return;
  }
  const IPAddress remote_ip = g_udp.remoteIP();
  const uint16_t remote_port = g_udp.remotePort();
  if (g_udp.beginPacket(remote_ip, remote_port)) {
    g_udp.write(reinterpret_cast<const uint8_t*>(kDiscoveryAck),
                strlen(kDiscoveryAck));
    g_udp.endPacket();
    Serial.printf("[MOCK][DISCOVERY] ack -> %s:%u\n",
                  remote_ip.toString().c_str(), remote_port);
  }
}

bool ReadLineWithTimeout(WiFiClient* client, uint32_t timeout_ms, String* out) {
  if (client == nullptr || out == nullptr) {
    return false;
  }
  out->remove(0);
  const uint32_t start_ms = millis();
  while ((millis() - start_ms) < timeout_ms) {
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

void SendAck(WiFiClient* client) {
  if (client == nullptr || !client->connected()) {
    return;
  }
  StaticJsonDocument<320> res;
  res["ok"] = true;
  res["gateway_time_ms"] = millis();
  JsonObject override_obj = res["override"].to<JsonObject>();
  override_obj["enabled"] = false;
  override_obj["state"] = "normal";
  override_obj["reason"] = "";
  override_obj["vibrate"] = false;
  override_obj["buzzer"] = false;
  override_obj["expires_in_s"] = 0;
  String line;
  serializeJson(res, line);
  client->print(line);
  client->print('\n');
}

void TickTcpServer() {
  if (!g_client || !g_client.connected()) {
    if (g_client) {
      g_client.stop();
    }
    g_client = g_tcp_server.available();
    if (g_client) {
      g_client.setTimeout(kTcpReadTimeoutMs);
      Serial.printf("[MOCK][TCP] client connected from %s:%u\n",
                    g_client.remoteIP().toString().c_str(),
                    g_client.remotePort());
    }
    return;
  }

  if (!ReadLineWithTimeout(&g_client, kTcpReadTimeoutMs, &g_tcp_line)) {
    if (!g_client.connected()) {
      g_client.stop();
    }
    return;
  }

  StaticJsonDocument<1536> doc;
  const auto err = deserializeJson(doc, g_tcp_line);
  if (err) {
    Serial.printf("[MOCK][TCP] invalid_json err=%s\n", err.c_str());
    SendAck(&g_client);
    return;
  }

  const char* device_id = doc["device_id"] | "";
  const uint32_t seq = doc["seq"] | 0;
  Serial.printf("[MOCK][TCP] rx device_id=%s seq=%lu bytes=%u\n", device_id,
                static_cast<unsigned long>(seq),
                static_cast<unsigned>(g_tcp_line.length()));
  SendAck(&g_client);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(120);
  Serial.println("[MOCK] boot");

  StartWifi();

  g_udp.begin(kDiscoveryPort);
  g_tcp_server.begin();
  g_tcp_server.setNoDelay(true);

  if (MDNS.begin(kMdnsHost)) {
    MDNS.addService("heatrisk", "tcp", kTcpPort);
    Serial.printf("[MOCK][mDNS] host=%s.local tcp=%u\n", kMdnsHost, kTcpPort);
  } else {
    Serial.println("[MOCK][mDNS] start failed");
  }

  Serial.printf("[MOCK] discovery_port=%u tcp_port=%u\n", kDiscoveryPort,
                kTcpPort);
}

void loop() {
  const uint32_t now_ms = millis();
  TickWifi(now_ms);
  if (WiFi.status() == WL_CONNECTED) {
    TickDiscovery();
    TickTcpServer();
  }
}

