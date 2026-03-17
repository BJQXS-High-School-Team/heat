#include "status_logic.h"

#include <stddef.h>

namespace heatrisk {

namespace {

void EvaluateRuleHigh(float value, float warning, float danger, bool sensor_ok,
                      bool* danger_hit, uint8_t* warning_hits) {
  if (!sensor_ok) {
    return;
  }
  if (value >= danger) {
    *danger_hit = true;
  } else if (value >= warning) {
    ++(*warning_hits);
  }
}

void EvaluateRuleLow(float value, float warning, float danger, bool sensor_ok,
                     bool* danger_hit, uint8_t* warning_hits) {
  if (!sensor_ok) {
    return;
  }
  if (value <= danger) {
    *danger_hit = true;
  } else if (value <= warning) {
    ++(*warning_hits);
  }
}

}  // namespace

LocalStateEvalResult EvaluateLocalState(const PhysiologyData& physiology,
                                        const MotionData& motion,
                                        const SensorHealth& sensor_health,
                                        const LocalThresholds& thresholds) {
  LocalStateEvalResult out;
  out.local_state = StateLevel::kNormal;
  out.warning_rule_hits = 0;
  out.danger_rule_hit = false;

  EvaluateRuleHigh(physiology.heart_rate_bpm, thresholds.heart_rate_warning_threshold,
                   thresholds.heart_rate_danger_threshold, sensor_health.max30102_ok,
                   &out.danger_rule_hit, &out.warning_rule_hits);

  EvaluateRuleLow(physiology.spo2_pct, thresholds.spo2_warning_threshold,
                  thresholds.spo2_danger_threshold, sensor_health.max30102_ok,
                  &out.danger_rule_hit, &out.warning_rule_hits);

  EvaluateRuleHigh(physiology.body_temp_c, thresholds.body_temp_warning_threshold,
                   thresholds.body_temp_danger_threshold, sensor_health.temp_ok,
                   &out.danger_rule_hit, &out.warning_rule_hits);

  EvaluateRuleHigh(motion.activity_score, thresholds.activity_warning_threshold,
                   thresholds.activity_danger_threshold, sensor_health.imu_ok,
                   &out.danger_rule_hit, &out.warning_rule_hits);

  if (out.danger_rule_hit ||
      out.warning_rule_hits >= thresholds.warning_rules_for_danger) {
    out.local_state = StateLevel::kDanger;
  } else if (out.warning_rule_hits > 0) {
    out.local_state = StateLevel::kWarning;
  } else {
    out.local_state = StateLevel::kNormal;
  }

  out.alarm_active = (out.local_state != StateLevel::kNormal);
  return out;
}

bool IsOverrideActive(const OverrideCommand& command, uint32_t now_ms) {
  if (!command.enabled) {
    return false;
  }
  if (command.expires_in_s == 0) {
    return true;
  }

  const uint32_t ttl_ms = command.expires_in_s * 1000UL;
  const uint32_t elapsed = now_ms - command.issued_at_ms;
  return elapsed <= ttl_ms;
}

StateLevel ResolveEffectiveState(StateLevel local_state,
                                 const OverrideCommand& command,
                                 uint32_t now_ms) {
  if (IsOverrideActive(command, now_ms)) {
    return command.state;
  }
  return local_state;
}

}  // namespace heatrisk
