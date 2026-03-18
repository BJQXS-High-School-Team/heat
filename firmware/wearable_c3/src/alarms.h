#pragma once

#include <Arduino.h>

#include "data_models.h"

namespace heatrisk {

class AlarmDriver {
 public:
  AlarmDriver(uint8_t vibration_pin, uint8_t buzzer_pin);

  void begin();
  void setMode(StateLevel effective_state, bool vibrate_enabled,
               bool buzzer_enabled);
  void tick(uint32_t now_ms);

 private:
  uint8_t vibration_pin_;
  uint8_t buzzer_pin_;
  StateLevel state_ = StateLevel::kNormal;
  bool vibrate_enabled_ = false;
  bool buzzer_enabled_ = false;
};

}  // namespace heatrisk
