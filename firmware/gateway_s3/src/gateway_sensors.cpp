#include "gateway_sensors.h"

#include <math.h>

namespace heatrisk {

bool GatewaySensors::begin(uint8_t sht35_addr, uint8_t bh1750_addr, TwoWire* wire) {
  if (wire == nullptr) {
    return false;
  }

  // SHT35 使用 Adafruit_SHT31 库驱动，常见地址为 0x44/0x45。
  // 该库在构造函数绑定 I2C 总线，这里使用默认 Wire。
  sht35_ready_ = sht35_.begin(sht35_addr);

  bh1750_ready_ = bh1750_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, bh1750_addr, wire);
  return sht35_ready_ || bh1750_ready_;
}

void GatewaySensors::sample(GatewayAmbientData* out_ambient, SensorHealth* out_health) {
  if (out_ambient == nullptr || out_health == nullptr) {
    return;
  }

  bool sht_ok = false;
  if (sht35_ready_) {
    const float temp_c = sht35_.readTemperature();
    const float humidity = sht35_.readHumidity();
    if (!isnan(temp_c) && !isnan(humidity)) {
      last_good_.temperature_c = temp_c;
      last_good_.humidity_pct = humidity;
      sht_ok = true;
    }
  }

  bool bh_ok = false;
  if (bh1750_ready_) {
    const float lux = bh1750_.readLightLevel();
    if (lux >= 0.0f) {
      last_good_.light_lux = lux;
      bh_ok = true;
    }
  }

  out_health->sht35_ok = sht_ok;
  out_health->bh1750_ok = bh_ok;
  *out_ambient = last_good_;
}

}  // namespace heatrisk
