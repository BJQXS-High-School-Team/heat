#pragma once

#include <stdint.h>

#include "data_models.h"

namespace heatrisk {

// Evaluate local state using only locally available sensor data.
LocalStateEvalResult EvaluateLocalState(const PhysiologyData& physiology,
                                        const MotionData& motion,
                                        const SensorHealth& sensor_health,
                                        const LocalThresholds& thresholds);

// Check whether an override command is still active at now_ms.
bool IsOverrideActive(const OverrideCommand& command, uint32_t now_ms);

// Resolve effective state with priority: active override > local state.
StateLevel ResolveEffectiveState(StateLevel local_state,
                                 const OverrideCommand& command,
                                 uint32_t now_ms);

}  // namespace heatrisk
