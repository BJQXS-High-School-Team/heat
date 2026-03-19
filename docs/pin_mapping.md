# Pin Mapping (Recommended Defaults)

说明：以下为建议默认值，最终以各端 `config.h` 为准。

## ESP32-C3 Wearable

- I2C SDA: `GPIO8`（OLED / MPU6050 / MAX30102）
- I2C SCL: `GPIO9`
- DS18B20 OneWire: `GPIO4`
- GPS UART RX（C3 接收）: `GPIO20`
- GPS UART TX（C3 发送）: `GPIO21`
- Buzzer: `GPIO2`
- Vibration Motor Driver: `GPIO3`

## ESP32-S3 Gateway

- I2C SDA: `GPIO8`（SHT35 + BH1750）
- I2C SCL: `GPIO9`

## Wiring Notes

- 振动马达建议使用 MOS 管驱动，不要直接 GPIO 拉载。
- 有源蜂鸣器建议串限流，并确认驱动电平。
- I2C 总线请统一电压域，必要时加上拉电阻。
