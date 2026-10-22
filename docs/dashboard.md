# Dashboard

## 入口

- `GET /` 返回轻量单页 HTML（S3 直出）

## 刷新策略

前端每 1.5s 轮询：

- `/api/health`
- `/api/host`
- `/api/devices`

## 页面分区

1. Gateway Status
2. Gateway Ambient (Host-Side Only)
3. Wearables 列表

## 设备卡片字段

- `device_id`
- `online/offline`
- `last_seen_ms_ago`
- `heart_rate_bpm`
- `spo2_pct`
- `body_temp_c`
- `activity_score` / `activity_level`
- GPS 概要（fix/satellites/lat/lng）
- `local_state`
- `effective_state`
- `alarm_active`

## 色彩规则

- normal: green
- warning: yellow
- danger: red
- offline: gray

## 关键标注要求

页面必须明确标注：`Gateway Ambient (Host-Side Only)`。

严禁把 S3 侧 SHT35/BH1750 数据标注成 wearable ambient。
