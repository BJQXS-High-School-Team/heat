import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SAMPLE_DIR = ROOT / "docs" / "sample_payloads"


def _load_json(name: str) -> dict:
    return json.loads((SAMPLE_DIR / name).read_text(encoding="utf-8"))


def test_wearable_upload_shape_has_required_keys() -> None:
    payload = _load_json("wearable_upload.json")
    assert payload["schema_version"] == 1
    assert payload["device_type"] == "wearable"
    assert "gps" in payload
    assert "physiology" in payload
    assert "motion" in payload
    assert "status" in payload
    assert "sensor_health" in payload
    assert "network" in payload


def test_gateway_telemetry_uses_gateway_ambient_label() -> None:
    payload = _load_json("gateway_telemetry.json")
    assert payload["schema_version"] == 1
    assert "gateway_ambient" in payload
    assert "sensor_health" in payload
    assert "network" in payload
    assert "temperature_c" in payload["gateway_ambient"]
    assert "humidity_pct" in payload["gateway_ambient"]
    assert "light_lux" in payload["gateway_ambient"]
    assert "sht35_ok" in payload["sensor_health"]
    assert "bh1750_ok" in payload["sensor_health"]


def test_command_payload_shape() -> None:
    payload = _load_json("command_override.json")
    assert payload["schema_version"] == 1
    assert payload["state"] in {"normal", "warning", "danger"}
    assert isinstance(payload["enabled"], bool)
    assert isinstance(payload["expires_in_s"], int)
    assert payload["expires_in_s"] >= 0


def test_upload_response_shape() -> None:
    payload = _load_json("api_upload_response.json")
    assert payload["ok"] is True
    assert "gateway_time_ms" in payload
    assert "gateway_receive_epoch_ms" in payload
    override = payload["override"]
    assert override["state"] in {"normal", "warning", "danger"}
    assert isinstance(override["enabled"], bool)
    assert isinstance(override["expires_in_s"], int)
