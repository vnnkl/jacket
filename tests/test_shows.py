import pytest
from jacket.shows import ShowConfig, pick_show, small_pulse, big_show
from jacket.effects import disco_rainbow
from jacket.protocol import power, color

def total(steps):
    return sum(d for _, d in steps)

def test_small_pulse_is_one_colour_and_ends_off():
    steps = small_pulse(hue=60, seconds=2.0)
    assert abs(total(steps) - 2.0) < 0.01
    hues = {p[4:6] for p, _ in steps if p[1] == 0x03}
    assert hues == {(60).to_bytes(2, "big")}
    assert steps[-1][0] == power(False)

def test_small_pulse_fades_up_and_down():
    levels = [int.from_bytes(p[8:10], "big") for p, _ in small_pulse(60, 2.0) if p[1] == 0x03]
    peak = levels.index(max(levels))
    assert max(levels) == 1000
    assert levels[:peak + 1] == sorted(levels[:peak + 1])
    assert levels[peak:] == sorted(levels[peak:], reverse=True)

def test_big_show_length_and_ending():
    steps = big_show(10.0)
    assert abs(total(steps) - 10.0) < 0.05
    assert steps[-1][0] == power(False)

def test_big_show_has_rainbow_strobe_and_white_finale():
    pkts = [p for p, _ in big_show(10.0)]
    hues = {p[4:6] for p in pkts if p[1] == 0x03 and p[6:8] != b"\x00\x00"}
    assert len(hues) >= 20                       # rainbow sweep
    assert pkts.count(power(False)) >= 10        # strobing
    assert color(0, 0, 1000) in pkts             # white flashes

def test_big_show_speeds_up():
    steps = big_show(10.0)
    sweep = [d for p, d in steps[:40] if d > 0]
    assert sweep[0] > sweep[-1]

CFG = ShowConfig()

@pytest.mark.parametrize("sats,expected", [(1, "small"), (999, "small"), (1000, "medium"),
                                           (9999, "medium"), (10000, "big"), (5_000_000, "big")])
def test_bands(sats, expected):
    assert pick_show(sats, CFG)[0] == expected

def test_minimum_amount():
    cfg = ShowConfig(min_sats=100)
    assert pick_show(99, cfg) is None
    assert pick_show(100, cfg)[0] == "small"

def test_medium_is_disco():
    assert pick_show(5000, CFG)[1] == disco_rainbow(3.0)

def test_custom_bands_and_durations():
    cfg = ShowConfig(medium_from=50, big_from=500, medium_seconds=2.0)
    assert pick_show(49, cfg)[0] == "small"
    assert pick_show(50, cfg) == ("medium", disco_rainbow(2.0))
    assert pick_show(500, cfg)[0] == "big"

def test_invalid_config_rejected():
    with pytest.raises(ValueError):
        ShowConfig(medium_from=10000, big_from=1000)
    with pytest.raises(ValueError):
        ShowConfig(small_seconds=0)
