from jacket.effects import disco_rainbow
from jacket.protocol import power

def test_disco_lasts_requested_duration():
    steps = disco_rainbow(seconds=3.0)
    assert abs(sum(delay for _, delay in steps) - 3.0) < 1e-6

def test_disco_cycles_through_many_hues():
    steps = disco_rainbow(seconds=3.0)
    colours = {pkt for pkt, _ in steps if pkt[1] == 0x03}
    assert len(colours) >= 6

def test_disco_strobes_off_and_on():
    pkts = [p for p, _ in disco_rainbow(seconds=3.0)]
    assert power(False) in pkts and power(True) in pkts

def test_disco_ends_powered_on():
    pkts = [p for p, _ in disco_rainbow(seconds=3.0)]
    assert pkts[-1] == power(True) or pkts[-1][1] == 0x03
    assert [p for p in pkts if p[1] == 0x02][-1] == power(True)

def test_disco_is_pure():
    assert disco_rainbow(3.0) == disco_rainbow(3.0)
