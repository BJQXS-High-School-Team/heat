# MQTT Topics

## Topic Templates

- `heatrisk/site/{site_id}/gateway/{gateway_id}/telemetry`
- `heatrisk/site/{site_id}/wearable/{device_id}/telemetry`
- `heatrisk/site/{site_id}/wearable/{device_id}/status`
- `heatrisk/site/{site_id}/command/all`
- `heatrisk/site/{site_id}/command/{device_id}`

## QoS / Retain (建议)

- telemetry: QoS 0, retain false
- status: QoS 1, retain true（可选）
- command: QoS 1, retain false

## Command Payload (schema_version = 1)

```json
{
  "schema_version": 1,
  "target_device_id": "wearable-c3-001",
  "enabled": true,
  "state": "danger",
  "reason": "cloud_ai_override",
  "vibrate": true,
  "buzzer": true,
  "expires_in_s": 30
}
```

## Validation Rules

- `state` must be one of: `normal|warning|danger`
- `expires_in_s` must be `>= 0`
- `target_device_id` 可为空仅当主题是 `command/all`
- 命令生效前需要 schema/version/字段完整性校验
