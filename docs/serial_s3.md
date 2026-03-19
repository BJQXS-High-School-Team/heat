# ESP32-S3 Serial Diagram

This diagram shows serial lines related to the gateway S3 board.

## 1) S3 USB Debug Serial

```mermaid
flowchart LR
    PC["PC / Serial Monitor"]
    USB["USB Cable"]
    S3["ESP32-S3"]

    PC --> USB --> S3
```

Use this port for:
- firmware upload
- runtime logs (`[S3] ...`)
- troubleshooting Wi-Fi / HTTP API / MQTT bridge / command cache

## 2) S3 Sensor Bus (Not UART)

S3 sensors (`SHT35`, `BH1750`) are connected by I2C, not UART.

Default pins from:
- [gateway config](C:/Users/fanxi/.codex/worktrees/e0c0/heat/firmware/gateway_s3/src/config.h)

```mermaid
flowchart LR
    S3["ESP32-S3"]
    SHT35["SHT35"]
    BH1750["BH1750"]

    S3 ---|"GPIO8 SDA"| SHT35
    S3 ---|"GPIO9 SCL"| SHT35
    S3 ---|"GPIO8 SDA"| BH1750
    S3 ---|"GPIO9 SCL"| BH1750
```

## 3) C3 to S3 Reminder

```mermaid
flowchart LR
    C3["ESP32-C3"]
    S3["ESP32-S3"]

    C3 -- "Wi-Fi HTTP JSON" --> S3
```

There is no C3-to-S3 UART data link in the current architecture.
