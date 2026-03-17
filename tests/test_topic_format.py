from desktop_client.topic_utils import (
    command_all_topic,
    command_device_topic,
    gateway_telemetry_topic,
    wearable_status_topic,
    wearable_telemetry_topic,
)


def test_topic_templates() -> None:
    assert (
        gateway_telemetry_topic("site-a", "gw-1")
        == "heatrisk/site/site-a/gateway/gw-1/telemetry"
    )
    assert (
        wearable_telemetry_topic("site-a", "wearable-1")
        == "heatrisk/site/site-a/wearable/wearable-1/telemetry"
    )
    assert (
        wearable_status_topic("site-a", "wearable-1")
        == "heatrisk/site/site-a/wearable/wearable-1/status"
    )
    assert command_all_topic("site-a") == "heatrisk/site/site-a/command/all"
    assert (
        command_device_topic("site-a", "wearable-1")
        == "heatrisk/site/site-a/command/wearable-1"
    )
