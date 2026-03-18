# Pin Mapping (Recommended Defaults)

> 说明：以下为建议默认值，最终以 `config.h` 为准，可按实际接线调整。

## ESP32-C3 Wearable

- I2C SDA: `GPIO8`（OLED / MPU6050 / MAX30102）
- I2C SCL: `GPIO9`
- DS18B20 OneWire: `GPIO4`
- GPS UART RX (C3接收): `GPIO20`
- GPS UART TX (C3发送): `GPIO21`
- Buzzer: `GPIO2`
- Vibration motor driver: `GPIO3`

## ESP32-S3 Gateway

- I2C SDA: `GPIO8`（SHT35 + BH1750）
- I2C SCL: `GPIO9`

## Wiring Notes

- 建议振动马达使用三极管/MOS驱动，不直接由GPIO拉载。
- 有源蜂鸣器建议串限流并确认驱动电平。
- I2C 总线请根据模块电压和板卡电平匹配。
