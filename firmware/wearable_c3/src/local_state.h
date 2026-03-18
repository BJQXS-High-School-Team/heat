#pragma once

#include "data_models.h"

namespace heatrisk {

struct EffectiveDecision {
  WearableStatus status;
  bool override_active = false;
  bool vibrate_enabled = false;
  bool buzzer_enabled = false;
};

class LocalStateEngine {
 public:
  explicit LocalStateEngine(const LocalThresholds& thresholds);

  EffectiveDecision Evaluate(const PhysiologyData& physiology,
                             const MotionData& motion,
                             const SensorHealth& sensor_health,
                             const OverrideCommand& override_cmd,
                             uint32_t now_ms) const;

 private:
  LocalThresholds thresholds_;
};

}  // namespace heatrisk
