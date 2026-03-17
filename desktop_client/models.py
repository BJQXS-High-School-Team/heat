from dataclasses import dataclass


@dataclass
class ClientConfig:
    broker_host: str = "broker.example.com"
    broker_port: int = 1883
    site_id: str = "site-a"
