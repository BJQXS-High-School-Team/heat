"""
Phase 1: 状态逻辑契约测试（Python版本）。
注意：这是行为契约测试，C++实现将在后续阶段持续对齐该规则。
"""


def evaluate_local_state(
    heart_rate: float,
    spo2: float,
    body_temp: float,
    activity: float,
) -> str:
    warning_hits = 0
    danger_hit = False

    if heart_rate >= 120:
        danger_hit = True
    elif heart_rate >= 100:
        warning_hits += 1

    if spo2 <= 90:
        danger_hit = True
    elif spo2 <= 94:
        warning_hits += 1

    if body_temp >= 38.5:
        danger_hit = True
    elif body_temp >= 37.8:
        warning_hits += 1

    if activity >= 0.85:
        danger_hit = True
    elif activity >= 0.65:
        warning_hits += 1

    if danger_hit or warning_hits >= 2:
        return "danger"
    if warning_hits >= 1:
        return "warning"
    return "normal"


def test_normal_state() -> None:
    assert evaluate_local_state(88, 97, 36.7, 0.2) == "normal"


def test_warning_state_single_rule_hit() -> None:
    assert evaluate_local_state(102, 97, 36.7, 0.2) == "warning"


def test_danger_state_any_danger_rule_hit() -> None:
    assert evaluate_local_state(88, 89, 36.7, 0.2) == "danger"


def test_danger_state_multiple_warnings() -> None:
    assert evaluate_local_state(102, 93, 36.7, 0.2) == "danger"
