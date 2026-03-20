#include "gateway_discovery.h"

#include <Arduino.h>
#include <ESPmDNS.h>

#include <cstring>

namespace heatrisk {

namespace {

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

}  // namespace

void GatewayDiscoveryService::begin(const char* mdns_host, uint16_t http_port,
                                    uint16_t udp_port,
                                    const char* discovery_magic) {
  SafeCopy(mdns_host_, sizeof(mdns_host_), mdns_host);
  SafeCopy(discovery_magic_, sizeof(discovery_magic_), discovery_magic);
  http_port_ = http_port;
  udp_port_ = udp_port;
  last_wifi_status_ = WiFi.status();
}

void GatewayDiscoveryService::ensureServices() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (!mdns_started_ && mdns_host_[0] != '\0') {
    if (MDNS.begin(mdns_host_)) {
      MDNS.addService("http", "tcp", http_port_);
      mdns_started_ = true;
      Serial.printf("[S3][DISCOVERY] mDNS ready host=%s.local\n", mdns_host_);
    } else {
      Serial.println("[S3][DISCOVERY] mDNS start failed");
    }
  }
  if (!udp_started_) {
    if (udp_.begin(udp_port_)) {
      udp_started_ = true;
      Serial.printf("[S3][DISCOVERY] UDP listen=%u\n", udp_port_);
    } else {
      Serial.println("[S3][DISCOVERY] UDP begin failed");
    }
  }
}

void GatewayDiscoveryService::stopServices() {
  if (udp_started_) {
    udp_.stop();
    udp_started_ = false;
  }
  if (mdns_started_) {
    MDNS.end();
    mdns_started_ = false;
  }
}

void GatewayDiscoveryService::handleUdpDiscovery() {
  if (!udp_started_ || discovery_magic_[0] == '\0') {
    return;
  }
  while (true) {
    const int packet_size = udp_.parsePacket();
    if (packet_size <= 0) {
      return;
    }

    char payload[96];
    const int n = udp_.read(payload, sizeof(payload) - 1);
    if (n <= 0) {
      continue;
    }
    payload[n] = '\0';

    if (strcmp(payload, discovery_magic_) != 0) {
      continue;
    }

    char ack[128];
    snprintf(ack, sizeof(ack), "HEATRISK_S3_ACK_V1 port=%u mdns=%s", http_port_,
             mdns_host_);
    udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
    udp_.write(reinterpret_cast<const uint8_t*>(ack), strlen(ack));
    udp_.endPacket();
    Serial.printf("[S3][DISCOVERY] reply to %s:%u\n",
                  udp_.remoteIP().toString().c_str(), udp_.remotePort());
  }
}

void GatewayDiscoveryService::tick() {
  const wl_status_t status = WiFi.status();
  if (status != last_wifi_status_) {
    last_wifi_status_ = status;
    if (status != WL_CONNECTED) {
      stopServices();
    }
  }
  ensureServices();
  handleUdpDiscovery();
}

}  // namespace heatrisk
