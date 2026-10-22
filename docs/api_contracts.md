# API Contracts

## 1) POST /api/wearable/upload

用途：C3 向 S3 上传最新快照。

- Content-Type: `application/json`
- 请求样例：`docs/sample_payloads/wearable_upload.json`
- 响应样例：`docs/sample_payloads/api_upload_response.json`

### 请求关键字段

- `schema_version`
- `device_id`
- `device_type`（必须为 `wearable`）
- `seq`、`device_uptime_ms`
- `gps`、`physiology`、`motion`、`status`、`sensor_health`、`network`

### 响应关键字段

- `ok`
- `gateway_time_ms`
- `gateway_receive_epoch_ms`（当前默认 0，未来可接入 NTP）
- `override`:
  - `enabled`
  - `state`
  - `reason`
  - `vibrate`
  - `buzzer`
  - `expires_in_s`

## 2) GET /api/health

用途：网关运行健康检查。

样例：`docs/sample_payloads/api_health.json`

关键字段：

- `ok`
- `uptime_ms`
- `wifi_connected`
- `mqtt_connected`
- `heap_free`
- `device_count`

## 3) GET /api/host

用途：获取网关侧主机快照（包含 gateway ambient）。

样例：`docs/sample_payloads/api_host.json`

关键字段：

- `ok`
- `schema_version`
- `site_id`
- `gateway_id`
- `gateway_time_ms`
- `gateway_ambient.temperature_c`
- `gateway_ambient.humidity_pct`
- `gateway_ambient.light_lux`
- `sensor_health.sht35_ok`
- `sensor_health.bh1750_ok`

## 4) GET /api/devices

用途：获取全部可穿戴快照列表。

样例：`docs/sample_payloads/api_devices.json`

关键字段：

- `ok`
- `gateway_time_ms`
- `device_count`
- `devices[]`

## 5) GET /api/device?id=<device_id>

用途：获取单个设备快照。

样例：`docs/sample_payloads/api_device_ok.json`

关键字段：

- `ok`
- `device`（字段结构与 `devices[]` 项一致）
