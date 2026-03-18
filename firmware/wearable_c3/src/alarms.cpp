#include "alarms.h"

#include "config.h"

namespace heatrisk {

AlarmDriver::AlarmDriver(uint8_t vibration_pin, uint8_t buzzer_pin)
    : vibration_pin_(vibration_pin), buzzer_pin_(buzzer_pin) {}

void AlarmDriver::begin() {
  pinMode(vibration_pin_, OUTPUT);
  pinMode(buzzer_pin_, OUTPUT);
  digitalWrite(vibration_pin_, LOW);
  digitalWrite(buzzer_pin_, LOW);
}

void AlarmDriver::setMode(StateLevel effective_state, bool vibrate_enabled,
                          bool buzzer_enabled) {
  state_ = effective_state;
  vibrate_enabled_ = vibrate_enabled;
  buzzer_enabled_ = buzzer_enabled;
}

void AlarmDriver::tick(uint32_t now_ms) {
  uint32_t period_ms = 1;
  uint32_t vibrate_on_ms = 0;
  uint32_t buzzer_on_ms = 0;

  switch (state_) {
    case StateLevel::kWarning:
      period_ms = wearable_config::kWarningPatternPeriodMs;
      vibrate_on_ms = wearable_config::kWarningVibrationOnMs;
      buzzer_on_ms = wearable_config::kWarningBuzzerOnMs;
      break;
    case StateLevel::kDanger:
      period_ms = wearable_config::kDangerPatternPeriodMs;
      vibrate_on_ms = wearable_config::kDangerVibrationOnMs;
      buzzer_on_ms = wearable_config::kDangerBuzzerOnMs;
      break;
    case StateLevel::kNormal:
    default:
      period_ms = 1;
      vibrate_on_ms = 0;
      buzzer_on_ms = 0;
      break;
  }

  const uint32_t phase_ms = (period_ms > 0) ? (now_ms % period_ms) : 0;
  const bool vibrate_on = vibrate_enabled_ && (phase_ms < vibrate_on_ms);
  const bool buzzer_on = buzzer_enabled_ && (phase_ms < buzzer_on_ms);

  digitalWrite(vibration_pin_, vibrate_on ? HIGH : LOW);
  digitalWrite(buzzer_pin_, buzzer_on ? HIGH : LOW);
}

}  // namespace heatrisk
