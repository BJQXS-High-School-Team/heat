# MQTT Topics

## Topic 模板

- `heatrisk/site/{site_id}/gateway/{gateway_id}/telemetry`
- `heatrisk/site/{site_id}/wearable/{device_id}/telemetry`
- `heatrisk/site/{site_id}/wearable/{device_id}/status`
- `heatrisk/site/{site_id}/command/all`
- `heatrisk/site/{site_id}/command/{device_id}`

## 建议 QoS

- gateway/wearable telemetry: QoS 0
- wearable status: QoS 0 或 QoS 1（视平台需要）
- command: QoS 1

## Command Payload（schema_version = 1）

参考：`docs/sample_payloads/command_override.json`

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

## 校验规则

- `state` 仅允许 `normal|warning|danger`
- `expires_in_s >= 0`
- `command/all`：可用 `target_device_id` 指定单设备，也可省略表示全局命令
- `command/{device_id}`：topic 中设备号优先于 payload 中 `target_device_id`

## TTL 行为

- 命令进入 S3 缓存后按 `issued_at_ms + expires_in_s` 判定有效性
- 过期命令不会继续下发给 C3
- 当设备命令过期后，若全局命令仍有效，回退使用全局命令
