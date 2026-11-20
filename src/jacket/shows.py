"""The three payment shows and which one a payment gets. Steps are (packet, delay_after_seconds)."""
from dataclasses import dataclass

from .effects import Step, disco_rainbow
from .protocol import MAX_LEVEL, color, power

FADE_LEVELS = 12              # brightness steps per fade
MIN_LEVEL = 10                # the controller ignores brightness below this
SWEEP_HUE_STEP = 15           # degrees per step in the big show's rainbow sweep
WHITE = (0, 0)                # hue, saturation for white


@dataclass(frozen=True)
class ShowConfig:
    min_sats: int = 0
    medium_from: int = 1000
    big_from: int = 10000
    small_hue: int = 60       # yellow
    small_seconds: float = 2.0
    medium_seconds: float = 3.0
    big_seconds: float = 10.0

    def __post_init__(self):
        if not 0 <= self.min_sats <= self.medium_from < self.big_from:
            raise ValueError("need 0 <= minimum <= medium-from < big-from (sats)")
        if min(self.small_seconds, self.medium_seconds, self.big_seconds) <= 0:
            raise ValueError("show lengths must be positive")


def _fade(hue: int, sat: int, seconds: float, up: bool) -> list[Step]:
    levels = [MIN_LEVEL + (MAX_LEVEL - MIN_LEVEL) * i // (FADE_LEVELS - 1) for i in range(FADE_LEVELS)]
    if not up:
        levels.reverse()
    return [(color(hue, sat, v), seconds / FADE_LEVELS) for v in levels]


def small_pulse(hue: int, seconds: float = 2.0) -> tuple[Step, ...]:
    """Fade up, hold, fade down in one colour (25% / 50% / 25%), then off."""
    fade, hold = seconds * 0.25, seconds * 0.5
    up = _fade(hue, MAX_LEVEL, fade, up=True)
    up[-1] = (up[-1][0], up[-1][1] + hold)
    return (power(True), 0.0), *up, *_fade(hue, MAX_LEVEL, fade, up=False), (power(False), 0.0)


def _accelerating(count: int, seconds: float, slow_to_fast: float = 3.0) -> list[float]:
    """`count` delays summing to `seconds`, the first `slow_to_fast` times longer than the last."""
    raw = [slow_to_fast - (slow_to_fast - 1) * i / max(count - 1, 1) for i in range(count)]
    scale = seconds / sum(raw)
    return [r * scale for r in raw]


def _sweep(seconds: float) -> list[Step]:
    hues = list(range(0, 360 * 2, SWEEP_HUE_STEP))  # two laps around the wheel
    return [(color(h), d) for h, d in zip(hues, _accelerating(len(hues), seconds))]


def _strobe(seconds: float, flashes: int = 24, hue_jump: int = 135) -> list[Step]:
    steps: list[Step] = []
    for i, period in enumerate(_accelerating(flashes, seconds)):
        steps += [(color(i * hue_jump), 0.0), (power(True), period * 0.6), (power(False), period * 0.4)]
    return steps


def _white_finale(seconds: float, flashes: int = 6) -> list[Step]:
    flash_time, fade_time = seconds * 0.6, seconds * 0.4
    period = flash_time / flashes
    steps: list[Step] = [(color(*WHITE, MAX_LEVEL), 0.0)]
    for _ in range(flashes):
        steps += [(power(True), period * 0.4), (power(False), period * 0.6)]
    steps.append((power(True), 0.0))
    return steps + _fade(*WHITE, fade_time, up=False) + [(power(False), 0.0)]


def big_show(seconds: float = 10.0) -> tuple[Step, ...]:
    """Accelerating rainbow sweep (30%), accelerating disco strobe (40%), white flashes + fade (30%)."""
    return ((power(True), 0.0), *_sweep(seconds * 0.3), *_strobe(seconds * 0.4),
            *_white_finale(seconds * 0.3))


def pick_show(sats: int, cfg: ShowConfig) -> tuple[str, tuple[Step, ...]] | None:
    if sats < max(cfg.min_sats, 1):
        return None
    if sats >= cfg.big_from:
        return "big", big_show(cfg.big_seconds)
    if sats >= cfg.medium_from:
        return "medium", disco_rainbow(cfg.medium_seconds)
    return "small", small_pulse(cfg.small_hue, cfg.small_seconds)
