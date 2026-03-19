from dataclasses import dataclass


@dataclass
class OverrideCommand:
    enabled: bool
    state: str
    expires_in_s: int
    issued_at_ms: int


def is_override_active(command: OverrideCommand, now_ms: int) -> bool:
    if not command.enabled:
        return False
    if command.expires_in_s == 0:
        return True
    return (now_ms - command.issued_at_ms) <= (command.expires_in_s * 1000)


class CommandCacheContract:
    def __init__(self) -> None:
        self.device_commands: dict[str, OverrideCommand] = {}
        self.global_command: OverrideCommand | None = None

    def set_for_device(self, device_id: str, command: OverrideCommand, now_ms: int) -> None:
        self.device_commands[device_id] = OverrideCommand(
            enabled=command.enabled,
            state=command.state,
            expires_in_s=command.expires_in_s,
            issued_at_ms=now_ms,
        )

    def set_global(self, command: OverrideCommand, now_ms: int) -> None:
        self.global_command = OverrideCommand(
            enabled=command.enabled,
            state=command.state,
            expires_in_s=command.expires_in_s,
            issued_at_ms=now_ms,
        )

    def get_for_device(self, device_id: str, now_ms: int) -> OverrideCommand | None:
        device_command = self.device_commands.get(device_id)
        if device_command and is_override_active(device_command, now_ms):
            return device_command

        if self.global_command and is_override_active(self.global_command, now_ms):
            return self.global_command

        return None

    def clear_expired(self, now_ms: int) -> None:
        expired = [
            device_id
            for device_id, command in self.device_commands.items()
            if not is_override_active(command, now_ms)
        ]
        for device_id in expired:
            del self.device_commands[device_id]

        if self.global_command and not is_override_active(self.global_command, now_ms):
            self.global_command = None


def test_device_command_before_ttl_expiry() -> None:
    cache = CommandCacheContract()
    cache.set_for_device("wearable-1", OverrideCommand(True, "danger", 30, 0), now_ms=1000)
    command = cache.get_for_device("wearable-1", now_ms=20000)
    assert command is not None
    assert command.state == "danger"


def test_device_command_expires_and_falls_back_to_global() -> None:
    cache = CommandCacheContract()
    cache.set_for_device("wearable-1", OverrideCommand(True, "danger", 3, 0), now_ms=0)
    cache.set_global(OverrideCommand(True, "warning", 30, 0), now_ms=500)

    command = cache.get_for_device("wearable-1", now_ms=5000)
    assert command is not None
    assert command.state == "warning"


def test_zero_ttl_means_non_expiring_command() -> None:
    cache = CommandCacheContract()
    cache.set_global(OverrideCommand(True, "warning", 0, 0), now_ms=0)

    command = cache.get_for_device("wearable-any", now_ms=3600 * 1000)
    assert command is not None
    assert command.state == "warning"


def test_disabled_command_is_not_active() -> None:
    cache = CommandCacheContract()
    cache.set_for_device("wearable-1", OverrideCommand(False, "danger", 30, 0), now_ms=100)
    assert cache.get_for_device("wearable-1", now_ms=1000) is None


def test_clear_expired_removes_device_and_global_commands() -> None:
    cache = CommandCacheContract()
    cache.set_for_device("wearable-1", OverrideCommand(True, "danger", 1, 0), now_ms=0)
    cache.set_global(OverrideCommand(True, "warning", 1, 0), now_ms=0)

    cache.clear_expired(now_ms=3000)
    assert "wearable-1" not in cache.device_commands
    assert cache.global_command is None
