import argparse, pytest
from jacket.cli import parse_color, parse_idle, ws_url, build_parser, WS_ENV
from jacket.protocol import color, power, scene

def test_parse_color():
    assert parse_color("red") == color(0)
    assert parse_color("200") == color(200)
    assert parse_color("White") == color(0, 0, 1000)
    with pytest.raises(argparse.ArgumentTypeError):
        parse_color("chartreuse-ish")

def test_parse_idle():
    assert parse_idle("off") == (power(False),)
    assert parse_idle("preset:7") == (power(True), scene(7))
    assert parse_idle("blue") == (power(True), color(240))
    with pytest.raises(argparse.ArgumentTypeError):
        parse_idle("preset:x")

def test_ws_url(monkeypatch):
    assert ws_url("wss://a.b/x") == "wss://a.b/x"
    monkeypatch.setenv(WS_ENV, "wss://env/x")
    assert ws_url(None) == "wss://env/x"
    with pytest.raises(SystemExit):
        ws_url("https://a.b")
    monkeypatch.delenv(WS_ENV)
    with pytest.raises(SystemExit):
        ws_url(None)

def test_listen_args():
    a = build_parser().parse_args(["listen", "wss://x/y", "--idle", "off"])
    assert a.cmd == "listen" and a.idle == (power(False),) and a.seconds == 3.0
