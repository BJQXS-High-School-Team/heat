import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SAMPLE_DIR = ROOT / "docs" / "sample_payloads"


def _load_json(name: str) -> dict:
    return json.loads((SAMPLE_DIR / name).read_text(encoding="utf-8"))


def test_api_health_shape() -> None:
    payload = _load_json("api_health.json")
    assert payload["ok"] is True
    assert "uptime_ms" in payload
    assert "wifi_connected" in payload
    assert "mqtt_connected" in payload
    assert "heap_free" in payload
    assert "device_count" in payload


def test_api_host_shape() -> None:
    payload = _load_json("api_host.json")
    assert payload["ok"] is True
    assert "gateway_ambient" in payload
    assert "sensor_health" in payload
    assert "network" in payload
    assert "temperature_c" in payload["gateway_ambient"]
    assert "humidity_pct" in payload["gateway_ambient"]
    assert "light_lux" in payload["gateway_ambient"]


def test_api_devices_shape() -> None:
    payload = _load_json("api_devices.json")
    assert payload["ok"] is True
    assert isinstance(payload["devices"], list)
    assert payload["device_count"] == len(payload["devices"])

    device = payload["devices"][0]
    assert "device_id" in device
    assert "physiology" in device
    assert "motion" in device
    assert "status" in device
    assert "gps" in device


def test_api_device_shape() -> None:
    payload = _load_json("api_device_ok.json")
    assert payload["ok"] is True
    assert "device" in payload
    device = payload["device"]
    assert "device_id" in device
    assert "sensor_health" in device
    assert "network" in device
