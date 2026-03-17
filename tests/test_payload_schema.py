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
    assert "gateway_ambient" in payload
    assert "temperature_c" in payload["gateway_ambient"]
    assert "humidity_pct" in payload["gateway_ambient"]
    assert "light_lux" in payload["gateway_ambient"]
