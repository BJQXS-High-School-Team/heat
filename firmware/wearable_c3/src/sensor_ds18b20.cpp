#include "sensor_ds18b20.h"

namespace heatrisk {

Ds18b20Sensor::Ds18b20Sensor(uint8_t pin)
    : pin_(pin), one_wire_(pin_), sensor_(&one_wire_) {}

bool Ds18b20Sensor::begin() {
  sensor_.begin();
  sensor_.setWaitForConversion(true);
  return true;
}

bool Ds18b20Sensor::sample(float* out_temp_c) {
  if (out_temp_c == nullptr) {
    return false;
  }

  sensor_.requestTemperatures();
  const float temp_c = sensor_.getTempCByIndex(0);
  const bool valid = (temp_c != DEVICE_DISCONNECTED_C) && (temp_c > -40.0f) &&
                     (temp_c < 85.0f);

  if (valid) {
    last_temp_c_ = temp_c;
  }

  healthy_ = valid;
  *out_temp_c = last_temp_c_;
  return valid;
}

bool Ds18b20Sensor::isHealthy() const { return healthy_; }

float Ds18b20Sensor::lastKnownTempC() const { return last_temp_c_; }

}  // namespace heatrisk
