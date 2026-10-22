# Validation Checklist

## 1. Build

- `platformio run -e wearable_c3`
- `platformio run -e gateway_s3`

## 2. Runtime - Wearable C3

- Wi-Fi 连接/重连日志正常
- 传感器初始化成功/失败日志可见
- 状态切换日志（normal/warning/danger）
- HTTP 上传成功/失败日志
- override 接收/应用/过期行为符合预期

## 3. Runtime - Gateway S3

- Wi-Fi 连接日志
- SHT35/BH1750 初始化与采样日志
- `/api/wearable/upload` 正常接收并返回 JSON
- MQTT 连接/断开/重连日志
- command 接收、校验、缓存、过期日志

## 4. API Contract

- `GET /api/health` 字段完整
- `GET /api/host` 明确 `gateway_ambient`
- `GET /api/devices` 返回设备数组
- `GET /api/device?id=...` 存在与不存在路径返回正确
- `POST /api/wearable/upload` 错误 JSON 被优雅拒绝

## 5. MQTT

- 网关 telemetry topic 持续上行
- wearable telemetry/status topic 持续上行
- command topic 可触发 S3 缓存并在 C3 下次上传时回传

## 6. Desktop Client

- 可显示网关环境数据
- 可显示可穿戴最新状态
- 可发布 override 命令用于联调

## 7. Automated Tests

- `pytest -q tests`
- 必须覆盖：状态逻辑、payload schema、topic 格式、命令 TTL、dashboard API 形状
