"""Command line: `jacket on|off|color|disco|preset|listen`."""
import argparse
import asyncio
import logging
import os
import sys
from urllib.parse import urlparse

from .device import Jacket, JacketError
from .effects import disco_rainbow
from .listener import listen
from .protocol import MAX_LEVEL, color, power, scene

NAMED_HUES = {"red": 0, "orange": 30, "yellow": 60, "green": 120, "cyan": 180,
              "blue": 240, "purple": 280, "pink": 320}
WS_ENV = "JACKET_PAYMENT_WS"
ADDRESS_ENV = "JACKET_ADDRESS"


def parse_color(value: str) -> bytes:
    """'red', a hue in degrees like '200', or 'white'."""
    name = value.strip().lower()
    if name == "white":
        return color(0, 0, MAX_LEVEL)
    if name in NAMED_HUES:
        return color(NAMED_HUES[name])
    try:
        return color(int(name))
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"unknown colour {value!r}: use white, {', '.join(NAMED_HUES)}, or a hue 0-359"
        ) from None


def parse_idle(value: str) -> tuple[bytes, ...]:
    """What the jacket shows between payments: off, a colour, or preset:N."""
    if value.lower() == "off":
        return (power(False),)
    if value.lower().startswith("preset:"):
        try:
            return (power(True), scene(int(value.split(":", 1)[1])))
        except ValueError as exc:
            raise argparse.ArgumentTypeError(f"bad preset {value!r}: {exc}") from None
    return (power(True), parse_color(value))


def ws_url(value: str | None) -> str:
    url = value or os.environ.get(WS_ENV, "")
    if not url:
        raise SystemExit(f"Give the payment websocket URL as an argument or set {WS_ENV}.")
    if urlparse(url).scheme not in ("ws", "wss") or not urlparse(url).netloc:
        raise SystemExit(f"Not a websocket URL (must start with ws:// or wss://): {url}")
    return url


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="jacket", description="Control the Hello Fairy LED jacket.")
    ap.add_argument("--address", default=os.environ.get(ADDRESS_ENV),
                    help=f"Bluetooth address (default: scan for 'Hello Fairy', or ${ADDRESS_ENV})")
    ap.add_argument("-v", "--verbose", action="store_true")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("on")
    sub.add_parser("off")
    c = sub.add_parser("color", help="solid colour: white, red, blue, ... or a hue 0-359")
    c.add_argument("color", type=parse_color)
    p = sub.add_parser("preset", help="built-in effect number")
    p.add_argument("number", type=int)
    d = sub.add_parser("disco", help="disco rainbow once")
    d.add_argument("--seconds", type=float, default=3.0)
    d.add_argument("--idle", type=parse_idle, default="off", help="afterwards: off, colour, or preset:N")
    li = sub.add_parser("listen", help="disco rainbow on every incoming payment")
    li.add_argument("url", nargs="?", help=f"payment websocket (or ${WS_ENV})")
    li.add_argument("--seconds", type=float, default=3.0)
    li.add_argument("--idle", type=parse_idle, default="off", help="between payments: off, colour, or preset:N")
    return ap


async def run(args: argparse.Namespace) -> None:
    jacket = Jacket(args.address)
    try:
        if args.cmd == "on":
            await jacket.send(power(True))
        elif args.cmd == "off":
            await jacket.send(power(False))
        elif args.cmd == "color":
            await jacket.send(power(True), args.color)
        elif args.cmd == "preset":
            await jacket.send(power(True), scene(args.number))
        elif args.cmd == "disco":
            await jacket.play(disco_rainbow(args.seconds))
            await jacket.send(*args.idle)
        elif args.cmd == "listen":
            url = ws_url(args.url)
            await jacket.send(*args.idle)

            async def on_payment() -> None:
                await jacket.play(disco_rainbow(args.seconds))
                await jacket.send(*args.idle)

            await listen(url, on_payment)
    finally:
        await jacket.close()


def main(argv: list[str] | None = None) -> None:
    args = build_parser().parse_args(argv)
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format="%(asctime)s %(message)s", datefmt="%H:%M:%S")
    try:
        asyncio.run(run(args))
    except JacketError as exc:
        sys.exit(f"Jacket: {exc}")
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
