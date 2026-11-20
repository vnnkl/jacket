"""Persistent BLE connection to the jacket with automatic reconnect."""
import asyncio
import logging
from collections.abc import Iterable

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

from .protocol import NOTIFY_CHAR, WRITE_CHAR

log = logging.getLogger(__name__)
NAME_PREFIX = "Hello Fairy"
SCAN_SECONDS = 15.0
CONNECT_TIMEOUT = 20.0


class JacketError(RuntimeError):
    pass


async def find_address(address: str | None) -> str:
    if address:
        return address
    dev = await BleakScanner.find_device_by_filter(
        lambda d, adv: (d.name or adv.local_name or "").startswith(NAME_PREFIX), timeout=SCAN_SECONDS
    )
    if dev is None:
        raise JacketError(
            "No 'Hello Fairy' jacket found. Is it switched on, in range, and is the Hello Fairy app closed?"
        )
    return dev.address


class Jacket:
    def __init__(self, address: str | None = None):
        self._address = address
        self._client: BleakClient | None = None
        self._lock = asyncio.Lock()

    async def _connected(self) -> BleakClient:
        if self._client and self._client.is_connected:
            return self._client
        address = await find_address(self._address)
        client = BleakClient(address, timeout=CONNECT_TIMEOUT)
        try:
            await client.connect()
            await client.start_notify(NOTIFY_CHAR, lambda _, data: log.debug("jacket replied %s", data.hex(" ")))
        except (BleakError, asyncio.TimeoutError, OSError) as exc:
            raise JacketError(f"Could not connect to the jacket at {address}: {exc}") from exc
        self._address, self._client = address, client
        log.info("connected to jacket %s", address)
        return client

    async def play(self, steps: Iterable[tuple[bytes, float]]) -> None:
        """Send packets with their delays; reconnects once if the link dropped."""
        steps = tuple(steps)
        async with self._lock:
            for attempt in (1, 2):
                try:
                    client = await self._connected()
                    for packet, delay in steps:
                        await client.write_gatt_char(WRITE_CHAR, packet, response=False)
                        if delay:
                            await asyncio.sleep(delay)
                    return
                except (BleakError, asyncio.TimeoutError, OSError) as exc:
                    log.warning("jacket write failed (attempt %d): %s", attempt, exc)
                    await self._drop()
            raise JacketError("Lost the Bluetooth connection to the jacket and could not reconnect.")

    async def send(self, *packets: bytes) -> None:
        await self.play((p, 0.0) for p in packets)

    async def _drop(self) -> None:
        client, self._client = self._client, None
        if client:
            try:
                await client.disconnect()
            except (BleakError, OSError):
                pass

    async def close(self) -> None:
        async with self._lock:
            await self._drop()
