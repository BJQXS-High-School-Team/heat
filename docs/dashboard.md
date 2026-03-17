# Dashboard

## Entry

- `GET /` 返回轻量单页HTML（轮询 `/api/*`）

## Polling Strategy

- 每 `1000~2000ms` 轮询一次：
  - `/api/health`
  - `/api/host`
  - `/api/devices`

## UI Blocks

1. 网关状态区
2. 网关环境区（明确标注 gateway ambient）
3. 可穿戴设备列表区

## Device Card Fields

- `device_id`
- `online/offline`
- `last_seen_ms_ago`
- `heart_rate_bpm`
- `spo2_pct`
- `body_temp_c`
- `activity_score`
- `gps`（fix/satellites/lat/lng）
- `local_state`
- `effective_state`
- `alarm_active`

## Color Rules

- normal: green
- warning: yellow
- danger: red
- offline: gray

## Important Label Requirement

所有 SHT45/BH1750 数据均使用 `Gateway Ambient` 标题，不得显示为 wearable ambient。
