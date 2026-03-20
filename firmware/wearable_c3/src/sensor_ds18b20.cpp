#include "sensor_ds18b20.h"

#include <Arduino.h>

namespace heatrisk {

Ds18b20Sensor::Ds18b20Sensor(uint8_t pin)
    : pin_(pin), one_wire_(pin_), sensor_(&one_wire_) {}

bool Ds18b20Sensor::begin() {
  sensor_.begin();
  sensor_.setWaitForConversion(false);
  sensor_.requestTemperatures();
  conversion_in_flight_ = true;
  conversion_start_ms_ = millis();
  return true;
}

bool Ds18b20Sensor::sample(float* out_temp_c) {
  if (out_temp_c == nullptr) {
    return false;
  }

  const uint32_t now_ms = millis();
  bool has_new_value = false;

  if (conversion_in_flight_ &&
      (now_ms - conversion_start_ms_ >= kConversionWaitMs)) {
    const float temp_c = sensor_.getTempCByIndex(0);
    const bool valid = (temp_c != DEVICE_DISCONNECTED_C) && (temp_c > -40.0f) &&
                       (temp_c < 85.0f);
    if (valid) {
      last_temp_c_ = temp_c;
      healthy_ = true;
      has_new_value = true;
    } else {
      healthy_ = false;
    }

    sensor_.requestTemperatures();
    conversion_start_ms_ = now_ms;
    conversion_in_flight_ = true;
  } else if (!conversion_in_flight_) {
    sensor_.requestTemperatures();
    conversion_start_ms_ = now_ms;
    conversion_in_flight_ = true;
  }

  *out_temp_c = last_temp_c_;
  return has_new_value;
}

bool Ds18b20Sensor::isHealthy() const { return healthy_; }

float Ds18b20Sensor::lastKnownTempC() const { return last_temp_c_; }

}  // namespace heatrisk
