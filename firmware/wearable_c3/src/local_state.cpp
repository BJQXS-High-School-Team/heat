#include "local_state.h"

#include "status_logic.h"

namespace heatrisk {

LocalStateEngine::LocalStateEngine(const LocalThresholds& thresholds)
    : thresholds_(thresholds) {}

EffectiveDecision LocalStateEngine::Evaluate(
    const PhysiologyData& physiology, const MotionData& motion,
    const SensorHealth& sensor_health, const OverrideCommand& override_cmd,
    uint32_t now_ms) const {
  const LocalStateEvalResult local_eval =
      EvaluateLocalState(physiology, motion, sensor_health, thresholds_);

  EffectiveDecision out;
  out.status.local_state = local_eval.local_state;
  out.status.effective_state =
      ResolveEffectiveState(local_eval.local_state, override_cmd, now_ms);
  out.override_active = IsOverrideActive(override_cmd, now_ms);

  if (out.override_active) {
    out.vibrate_enabled = override_cmd.vibrate || (out.status.effective_state != StateLevel::kNormal);
    out.buzzer_enabled = override_cmd.buzzer || (out.status.effective_state != StateLevel::kNormal);
    out.status.alarm_active = out.vibrate_enabled || out.buzzer_enabled;
  } else {
    out.vibrate_enabled = (out.status.effective_state != StateLevel::kNormal);
    out.buzzer_enabled = (out.status.effective_state != StateLevel::kNormal);
    out.status.alarm_active = (out.status.effective_state != StateLevel::kNormal);
  }
  return out;
}

}  // namespace heatrisk
