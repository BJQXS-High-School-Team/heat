#pragma once

#include <Adafruit_SHT31.h>
#include <BH1750.h>
#include <Wire.h>

#include "data_models.h"

namespace heatrisk {

class GatewaySensors {
 public:
  bool begin(uint8_t sht35_addr, uint8_t bh1750_addr, TwoWire* wire = &Wire);
  void sample(GatewayAmbientData* out_ambient, SensorHealth* out_health);

 private:
  bool sht35_ready_ = false;
  bool bh1750_ready_ = false;
  Adafruit_SHT31 sht35_;
  BH1750 bh1750_;
  GatewayAmbientData last_good_;
};

}  // namespace heatrisk
