# API Contracts

## POST /api/wearable/upload

用途：C3 向 S3 上传最新快照。  
Content-Type: `application/json`

### Request (schema_version = 1)

```json
{
  "schema_version": 1,
  "device_id": "wearable-c3-001",
  "device_type": "wearable",
  "seq": 123,
  "device_uptime_ms": 456789,
  "gps": {
    "lat": 39.123456,
    "lng": 116.123456,
    "has_fix": true,
    "satellites": 6
  },
  "physiology": {
    "body_temp_c": 36.8,
    "heart_rate_bpm": 92,
    "spo2_pct": 97
  },
  "motion": {
    "activity_score": 0.42,
    "activity_level": "low"
  },
  "status": {
    "local_state": "normal",
    "effective_state": "normal",
    "alarm_active": false
  },
  "sensor_health": {
    "gps_ok": true,
    "temp_ok": true,
    "max30102_ok": true,
    "imu_ok": true,
    "oled_ok": true
  },
  "network": {
    "rssi": -58
  }
}
```

### Response

```json
{
  "ok": true,
  "gateway_time_ms": 123456789,
  "gateway_receive_epoch_ms": 1710000000000,
  "override": {
    "enabled": false,
    "state": "normal",
    "reason": "",
    "vibrate": false,
    "buzzer": false,
    "expires_in_s": 0
  }
}
```

## GET /api/health

用途：网关基本健康状态。

返回字段建议：

- `ok`
- `wifi_connected`
- `mqtt_connected`
- `uptime_ms`
- `heap_free`

## GET /api/host

用途：获取网关侧环境数据（非可穿戴侧环境）。

返回字段建议：

- `gateway_id`
- `site_id`
- `gateway_ambient.temperature_c`
- `gateway_ambient.humidity_pct`
- `gateway_ambient.light_lux`

## GET /api/devices

用途：获取所有可穿戴设备快照列表。

## GET /api/device?id=<device_id>

用途：获取单个设备快照详情。
