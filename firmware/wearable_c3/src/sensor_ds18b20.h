#pragma once

#include <stdint.h>

#include <DallasTemperature.h>
#include <OneWire.h>

namespace heatrisk {

class Ds18b20Sensor {
 public:
  explicit Ds18b20Sensor(uint8_t pin);

  bool begin();
  bool sample(float* out_temp_c);
  bool isHealthy() const;
  float lastKnownTempC() const;

 private:
  uint8_t pin_;
  OneWire one_wire_;
  DallasTemperature sensor_;
  bool healthy_ = false;
  float last_temp_c_ = 0.0f;
};

}  // namespace heatrisk
