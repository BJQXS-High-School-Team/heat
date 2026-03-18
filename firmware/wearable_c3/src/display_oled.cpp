#include "display_oled.h"

#include <stdio.h>

namespace heatrisk {

namespace {

const char* BinaryStatusText(StateLevel effective_state) {
  return (effective_state == StateLevel::kNormal) ? "NORMAL" : "ABNORMAL";
}

}  // namespace

OledDisplay::OledDisplay(uint8_t width, uint8_t height, int8_t reset_pin)
    : display_(width, height, &Wire, reset_pin) {}

bool OledDisplay::begin(uint8_t i2c_addr) {
  healthy_ = display_.begin(SSD1306_SWITCHCAPVCC, i2c_addr);
  if (!healthy_) {
    return false;
  }
  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  display_.display();
  return true;
}

void OledDisplay::render(const OledViewModel& vm) {
  if (!healthy_) {
    return;
  }

  char line[40];
  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setCursor(0, 0);

  snprintf(line, sizeof(line), "HR:%.0f SpO2:%.0f", vm.heart_rate_bpm, vm.spo2_pct);
  display_.println(line);

  snprintf(line, sizeof(line), "Temp:%.1fC Act:%.2f", vm.body_temp_c,
           vm.activity_score);
  display_.println(line);

  snprintf(line, sizeof(line), "GPS:%s SAT:%u", vm.gps_has_fix ? "FIX" : "NO",
           vm.gps_satellites);
  display_.println(line);

  snprintf(line, sizeof(line), "L:%s E:%s",
           StateLevelToString(vm.local_state),
           StateLevelToString(vm.effective_state));
  display_.println(line);

  snprintf(line, sizeof(line), "%s %s %s",
           BinaryStatusText(vm.effective_state),
           vm.override_active ? "OVR" : "LOC",
           vm.wifi_connected ? "WIFI" : "NOWIFI");
  display_.println(line);

  if (vm.alarm_active) {
    display_.println("ALARM: ON");
  } else {
    display_.println("ALARM: OFF");
  }

  display_.display();
}

bool OledDisplay::isHealthy() const { return healthy_; }

}  // namespace heatrisk
