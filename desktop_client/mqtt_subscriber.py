import json
import threading

import paho.mqtt.client as mqtt

from models import ClientConfig, GatewaySnapshot, RuntimeState, WearableSnapshot
from topic_utils import command_all_topic, command_device_topic


class HeatRiskMqttClient:
    def __init__(self, config: ClientConfig, raw_log: bool = False) -> None:
        self._config = config
        self._raw_log = raw_log
        self._state = RuntimeState()
        self._lock = threading.Lock()

        self._client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        if config.username:
            self._client.username_pw_set(config.username, config.password)

        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message
        self._client.on_disconnect = self._on_disconnect

    def connect(self) -> None:
        self._client.connect(
            self._config.broker_host,
            self._config.broker_port,
            keepalive=self._config.keepalive_s,
        )
        self._client.loop_start()

    def disconnect(self) -> None:
        self._client.loop_stop()
        self._client.disconnect()

    def _on_connect(self, client: mqtt.Client, _userdata, _flags, reason_code, _properties) -> None:
        if reason_code != 0:
            print(f"[desktop] MQTT connect failed: {reason_code}")
            return
        print("[desktop] MQTT connected")
        site = self._config.site_id
        client.subscribe(f"heatrisk/site/{site}/gateway/+/telemetry", qos=0)
        client.subscribe(f"heatrisk/site/{site}/wearable/+/telemetry", qos=0)
        client.subscribe(f"heatrisk/site/{site}/wearable/+/status", qos=0)

    def _on_disconnect(self, _client: mqtt.Client, _userdata, _flags, reason_code, _properties) -> None:
        print(f"[desktop] MQTT disconnected: {reason_code}")

    def _on_message(self, _client: mqtt.Client, _userdata, msg: mqtt.MQTTMessage) -> None:
        topic = msg.topic
        payload_raw = msg.payload.decode("utf-8", errors="replace")
        if self._raw_log:
            print(f"[mqtt] {topic} => {payload_raw}")

        try:
            payload = json.loads(payload_raw)
        except json.JSONDecodeError:
            return

        parts = topic.split("/")
        if len(parts) < 6:
            return
        # heatrisk/site/{site}/{kind}/{id}/{metric}
        kind = parts[3]
        target_id = parts[4]
        metric = parts[5]
        tail = parts[-1]

        with self._lock:
            self._state.last_message_topic = topic
            if kind == "gateway":
                snap = self._state.gateways.get(target_id)
                if snap is None:
                    snap = GatewaySnapshot(gateway_id=target_id)
                    self._state.gateways[target_id] = snap
                snap.payload = payload
                return

            if kind == "wearable":
                snap = self._state.wearables.get(target_id)
                if snap is None:
                    snap = WearableSnapshot(device_id=target_id)
                    self._state.wearables[target_id] = snap
                if tail == "telemetry" or metric == "telemetry":
                    snap.telemetry = payload
                elif tail == "status" or metric == "status":
                    snap.status = payload

    def render_summary(self) -> str:
        with self._lock:
            lines = []
            lines.append("========== Desktop HeatRisk Monitor ==========")
            lines.append(
                f"Last topic: {self._state.last_message_topic or 'N/A'}"
            )

            if not self._state.gateways:
                lines.append("Gateway: no data")
            else:
                lines.append("Gateway:")
                for gateway_id in sorted(self._state.gateways):
                    payload = self._state.gateways[gateway_id].payload
                    ambient = payload.get("gateway_ambient", {})
                    lines.append(
                        f"  - {gateway_id} | temp={ambient.get('temperature_c','?')}C "
                        f"humi={ambient.get('humidity_pct','?')}% "
                        f"lux={ambient.get('light_lux','?')}"
                    )

            if not self._state.wearables:
                lines.append("Wearables: no data")
            else:
                lines.append("Wearables:")
                for device_id in sorted(self._state.wearables):
                    snap = self._state.wearables[device_id]
                    t = snap.telemetry
                    s = snap.status
                    physiology = t.get("physiology", {})
                    status = (s.get("status") or t.get("status") or {})
                    lines.append(
                        f"  - {device_id} | HR={physiology.get('heart_rate_bpm','?')} "
                        f"SpO2={physiology.get('spo2_pct','?')} "
                        f"Temp={physiology.get('body_temp_c','?')} "
                        f"local={status.get('local_state','?')} "
                        f"effective={status.get('effective_state','?')} "
                        f"alarm={status.get('alarm_active','?')}"
                    )
            return "\n".join(lines)

    def publish_override(
        self,
        target_device_id: str,
        enabled: bool,
        state: str,
        reason: str,
        vibrate: bool,
        buzzer: bool,
        expires_in_s: int,
    ) -> None:
        if target_device_id:
            topic = command_device_topic(self._config.site_id, target_device_id)
        else:
            topic = command_all_topic(self._config.site_id)

        payload = {
            "schema_version": 1,
            "target_device_id": target_device_id,
            "enabled": enabled,
            "state": state,
            "reason": reason,
            "vibrate": vibrate,
            "buzzer": buzzer,
            "expires_in_s": expires_in_s,
        }
        result = self._client.publish(topic, json.dumps(payload), qos=1)
        result.wait_for_publish()
        print(f"[desktop] command published topic={topic} rc={result.rc}")
