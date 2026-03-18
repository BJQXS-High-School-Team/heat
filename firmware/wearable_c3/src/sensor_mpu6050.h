#pragma once

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "data_models.h"

namespace heatrisk {

class Mpu6050Sensor {
 public:
  bool begin(TwoWire* wire = &Wire);
  bool sample(MotionData* out_motion);
  bool isHealthy() const;

 private:
  Adafruit_MPU6050 sensor_;
  bool healthy_ = false;
  MotionData last_good_;
};

}  // namespace heatrisk
