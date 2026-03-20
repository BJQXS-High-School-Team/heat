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
  bool conversion_in_flight_ = false;
  uint32_t conversion_start_ms_ = 0;
  static constexpr uint16_t kConversionWaitMs = 750;
};

}  // namespace heatrisk
