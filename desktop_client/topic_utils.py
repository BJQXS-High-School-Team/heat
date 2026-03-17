def gateway_telemetry_topic(site_id: str, gateway_id: str) -> str:
    return f"heatrisk/site/{site_id}/gateway/{gateway_id}/telemetry"


def wearable_telemetry_topic(site_id: str, device_id: str) -> str:
    return f"heatrisk/site/{site_id}/wearable/{device_id}/telemetry"


def wearable_status_topic(site_id: str, device_id: str) -> str:
    return f"heatrisk/site/{site_id}/wearable/{device_id}/status"


def command_all_topic(site_id: str) -> str:
    return f"heatrisk/site/{site_id}/command/all"


def command_device_topic(site_id: str, device_id: str) -> str:
    return f"heatrisk/site/{site_id}/command/{device_id}"
