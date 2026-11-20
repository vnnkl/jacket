import pytest
from jacket.protocol import frame, power, color, scene

def test_power_matches_bytes_confirmed_on_device():
    assert power(False) == bytes.fromhex("AA020100AD")
    assert power(True) == bytes.fromhex("AA020101AE")

def test_checksum_is_low_byte_of_sum():
    f = frame(0x03, bytes([0xFF, 0xFF]))
    assert f[-1] == sum(f[:-1]) & 0xFF

def test_color_encodes_hsv_big_endian():
    assert color(300, 1000, 500) == frame(0x03, bytes([1, 0x01, 0x2C, 0x03, 0xE8, 0x01, 0xF4]))

def test_color_wraps_hue():
    assert color(360) == color(0)
    assert color(-60) == color(300)

@pytest.mark.parametrize("s,v", [(-1, 10), (1001, 10), (10, 1001)])
def test_color_rejects_out_of_range(s, v):
    with pytest.raises(ValueError):
        color(0, s, v)

def test_scene():
    assert scene(5, 1000) == frame(0x03, bytes([2, 5, 0x03, 0xE8]))
    with pytest.raises(ValueError):
        scene(256)
