"""Effects as pure data: a tuple of (packet, delay_after_seconds)."""
from .protocol import color, power

Step = tuple[bytes, float]

DISCO_HUE_JUMP = 135   # big jumps around the wheel read as "disco", not a smooth fade
DISCO_ON_SECONDS = 0.12
DISCO_OFF_SECONDS = 0.05


def disco_rainbow(seconds: float = 3.0) -> tuple[Step, ...]:
    """Strobing colour jumps around the rainbow for `seconds`, ending lit."""
    if seconds <= 0:
        raise ValueError("seconds must be positive")
    period = DISCO_ON_SECONDS + DISCO_OFF_SECONDS
    flashes = max(1, round(seconds / period))
    on = seconds / flashes * (DISCO_ON_SECONDS / period)
    off = seconds / flashes - on
    steps: list[Step] = []
    for i in range(flashes):
        steps += [(color(i * DISCO_HUE_JUMP), 0.0), (power(True), on), (power(False), off)]
    steps[-1] = (power(True), off)
    return tuple(steps)
