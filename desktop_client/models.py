from dataclasses import dataclass, field
from typing import Dict, Optional


@dataclass
class ClientConfig:
    broker_host: str = "broker.example.com"
    broker_port: int = 1883
    username: str = ""
    password: str = ""
    site_id: str = "site-a"
    keepalive_s: int = 30
    bemfa_mode: bool = False
    bemfa_topic_gateway: str = "heatriskgw004"
    bemfa_topic_wearable_telemetry: str = "heatriskwb004"
    bemfa_topic_wearable_status: str = "heatriskst004"
    bemfa_topic_command: str = "heatriskcmd004"


@dataclass
class GatewaySnapshot:
    gateway_id: str
    payload: dict = field(default_factory=dict)


@dataclass
class WearableSnapshot:
    device_id: str
    telemetry: dict = field(default_factory=dict)
    status: dict = field(default_factory=dict)


@dataclass
class RuntimeState:
    gateways: Dict[str, GatewaySnapshot] = field(default_factory=dict)
    wearables: Dict[str, WearableSnapshot] = field(default_factory=dict)
    last_message_topic: Optional[str] = None
