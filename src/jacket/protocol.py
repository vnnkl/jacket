"""Hello Fairy BLE packet builders. Frame: AA TYPE LEN payload... SUM (SUM = low byte of the sum)."""

SERVICE = "49535343-fe7d-4ae5-8fa9-9fafd205e455"
WRITE_CHAR = "49535343-8841-43f4-a8d4-ecbe34729bb3"
NOTIFY_CHAR = "49535343-1e4d-4bd9-ba61-23c647249616"

MAX_LEVEL = 1000  # saturation / brightness scale used by the controller


def frame(kind: int, payload: bytes = b"") -> bytes:
    body = bytes([0xAA, kind, len(payload)]) + payload
    return body + bytes([sum(body) & 0xFF])


def power(on: bool) -> bytes:
    return frame(0x02, bytes([1 if on else 0]))


def _level(name: str, value: int) -> bytes:
    if not 0 <= value <= MAX_LEVEL:
        raise ValueError(f"{name} must be 0-{MAX_LEVEL}, got {value}")
    return value.to_bytes(2, "big")


def color(hue: int, saturation: int = MAX_LEVEL, brightness: int = MAX_LEVEL) -> bytes:
    """Whole-jacket solid colour. hue in degrees (wrapped to 0-359)."""
    payload = bytes([1]) + (hue % 360).to_bytes(2, "big")
    return frame(0x03, payload + _level("saturation", saturation) + _level("brightness", brightness))


def scene(number: int, brightness: int = MAX_LEVEL) -> bytes:
    """Built-in preset effect."""
    if not 0 <= number <= 255:
        raise ValueError(f"scene must be 0-255, got {number}")
    return frame(0x03, bytes([2, number]) + _level("brightness", brightness))
