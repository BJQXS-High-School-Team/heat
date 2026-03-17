#pragma once

#include <stdint.h>

#include "data_models.h"

namespace heatrisk {

// 评估本地状态（仅使用本地可用传感数据）
LocalStateEvalResult EvaluateLocalState(const PhysiologyData& physiology,
                                        const MotionData& motion,
                                        const SensorHealth& sensor_health,
                                        const LocalThresholds& thresholds);

// override 是否在当前时刻有效
bool IsOverrideActive(const OverrideCommand& command, uint32_t now_ms);

// 根据优先级生成有效状态：override（有效）> 本地状态
StateLevel ResolveEffectiveState(StateLevel local_state,
                                 const OverrideCommand& command,
                                 uint32_t now_ms);

}  // namespace heatrisk
