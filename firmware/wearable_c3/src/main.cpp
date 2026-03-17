#include <Arduino.h>
#include <cstring>

#include "config.h"
#include "data_models.h"
#include "status_logic.h"

namespace {

uint32_t g_last_sample_ms = 0;
uint32_t g_seq = 0;

heatrisk::PhysiologyData g_physiology;
heatrisk::MotionData g_motion;
heatrisk::SensorHealth g_health;
heatrisk::OverrideCommand g_override;

void SeedDemoData() {
  // Phase 1 仅用于验证共享逻辑可编译，后续将由真实传感器驱动替换。
  g_physiology.body_temp_c = 36.7f;
  g_physiology.heart_rate_bpm = 88.0f;
  g_physiology.spo2_pct = 97.0f;
  g_motion.activity_score = 0.22f;
  strncpy(g_motion.activity_level, "low", sizeof(g_motion.activity_level) - 1);

  g_health.gps_ok = true;
  g_health.temp_ok = true;
  g_health.max30102_ok = true;
  g_health.imu_ok = true;
  g_health.oled_ok = true;
}

void TickSample() {
  const uint32_t now = millis();
  if (now - g_last_sample_ms < wearable_config::kSampleIntervalMs) {
    return;
  }
  g_last_sample_ms = now;
  ++g_seq;

  const auto local_eval = heatrisk::EvaluateLocalState(
      g_physiology, g_motion, g_health, wearable_config::kThresholds);
  const auto effective =
      heatrisk::ResolveEffectiveState(local_eval.local_state, g_override, now);

  Serial.printf(
      "[C3][Phase1] seq=%lu local=%s effective=%s alarm=%d warning_hits=%u "
      "danger_hit=%d\n",
      static_cast<unsigned long>(g_seq),
      heatrisk::StateLevelToString(local_eval.local_state),
      heatrisk::StateLevelToString(effective), local_eval.alarm_active,
      local_eval.warning_rule_hits, local_eval.danger_rule_hit);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  SeedDemoData();

  Serial.println("[C3] Wearable firmware skeleton ready (Phase 1).");
  Serial.println("[C3] TODO(Phase2): sensor drivers, OLED, alarms, HTTP client.");
}

void loop() { TickSample(); }
