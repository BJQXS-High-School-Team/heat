#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>

namespace heatrisk {

class GatewayDiscoveryService {
 public:
  void begin(const char* mdns_host, uint16_t http_port, uint16_t udp_port,
             const char* discovery_magic);
  void tick();

 private:
  void ensureServices();
  void stopServices();
  void handleUdpDiscovery();

  char mdns_host_[48] = {0};
  char discovery_magic_[48] = {0};
  uint16_t http_port_ = 80;
  uint16_t udp_port_ = 4210;
  wl_status_t last_wifi_status_ = WL_IDLE_STATUS;
  bool mdns_started_ = false;
  bool udp_started_ = false;
  WiFiUDP udp_;
};

}  // namespace heatrisk
