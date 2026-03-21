#pragma once

#include <stddef.h>
#include <stdint.h>

namespace heatrisk {

struct Max30102AlgoResult {
  float heart_rate_bpm = 0.0f;
  float spo2_pct = 0.0f;
  bool heart_rate_valid = false;
  bool spo2_valid = false;
};

bool ComputeMax30102Metrics(const uint32_t* ir_buffer, const uint32_t* red_buffer,
                            size_t length, Max30102AlgoResult* out_result);

}  // namespace heatrisk
