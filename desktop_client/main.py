import argparse
import time

from models import ClientConfig
from mqtt_subscriber import HeatRiskMqttClient


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="HeatRisk desktop MQTT monitor and command tool"
    )
    parser.add_argument("--host", default="broker.example.com", help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument("--username", default="", help="MQTT username")
    parser.add_argument("--password", default="", help="MQTT password")
    parser.add_argument("--site-id", default="site-a", help="Site id")
    parser.add_argument("--keepalive", type=int, default=30, help="MQTT keepalive seconds")
    parser.add_argument("--refresh", type=float, default=2.0, help="Monitor refresh seconds")
    parser.add_argument("--raw-log", action="store_true", help="Print raw MQTT messages")
    parser.add_argument(
        "--bemfa-mode",
        action="store_true",
        help="Use Bemfa topic mode (<topic>/up and <topic>/set)",
    )
    parser.add_argument(
        "--bemfa-topic-gateway",
        default="heatriskgw004",
        help="Bemfa gateway telemetry topic",
    )
    parser.add_argument(
        "--bemfa-topic-wearable-telemetry",
        default="heatriskwb004",
        help="Bemfa wearable telemetry topic",
    )
    parser.add_argument(
        "--bemfa-topic-wearable-status",
        default="heatriskst004",
        help="Bemfa wearable status topic",
    )
    parser.add_argument(
        "--bemfa-topic-command",
        default="heatriskcmd004",
        help="Bemfa command topic",
    )

    parser.add_argument(
        "--publish-command",
        action="store_true",
        help="Publish one override command and exit",
    )
    parser.add_argument("--target-device-id", default="", help="Target wearable id")
    parser.add_argument(
        "--state",
        default="danger",
        choices=["normal", "warning", "danger"],
        help="Override state",
    )
    parser.add_argument("--disable", action="store_true", help="Publish disabled command")
    parser.add_argument("--reason", default="desktop_manual_override", help="Command reason")
    parser.add_argument("--vibrate", action="store_true", help="Command vibrate flag")
    parser.add_argument("--buzzer", action="store_true", help="Command buzzer flag")
    parser.add_argument("--expires-in-s", type=int, default=30, help="Command TTL seconds")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    cfg = ClientConfig(
        broker_host=args.host,
        broker_port=args.port,
        username=args.username,
        password=args.password,
        site_id=args.site_id,
        keepalive_s=args.keepalive,
        bemfa_mode=args.bemfa_mode,
        bemfa_topic_gateway=args.bemfa_topic_gateway,
        bemfa_topic_wearable_telemetry=args.bemfa_topic_wearable_telemetry,
        bemfa_topic_wearable_status=args.bemfa_topic_wearable_status,
        bemfa_topic_command=args.bemfa_topic_command,
    )

    client = HeatRiskMqttClient(cfg, raw_log=args.raw_log)
    client.connect()
    time.sleep(0.8)

    try:
        if args.publish_command:
            client.publish_override(
                target_device_id=args.target_device_id,
                enabled=not args.disable,
                state=args.state,
                reason=args.reason,
                vibrate=args.vibrate,
                buzzer=args.buzzer,
                expires_in_s=max(0, args.expires_in_s),
            )
            return

        while True:
            print(client.render_summary())
            print()
            time.sleep(max(0.5, args.refresh))
    except KeyboardInterrupt:
        pass
    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
