# Validation Checklist

## Build

- `pio run -e wearable_c3`
- `pio run -e gateway_s3`

## Runtime: Wearable C3

- Wi-Fi连接成功日志
- 传感器初始化日志（成功/失败）
- 状态切换日志（normal/warning/danger）
- HTTP上报成功/失败日志
- override应用/过期日志

## Runtime: Gateway S3

- Wi-Fi连接成功日志
- SHT45/BH1750初始化与采样日志
- `/api/wearable/upload`接收日志
- MQTT连接/断开/重连日志
- command接收/校验/缓存/过期日志

## API Shape

- `GET /api/health` 字段完整
- `GET /api/host` 明确 `gateway_ambient`
- `GET /api/devices` 返回列表
- `GET /api/device?id=...` 设备存在/不存在路径正确

## MQTT

- gateway telemetry topic有数据
- wearable telemetry/status topic有数据
- command topic消息可触发S3缓存并回传C3
