"""Listen to a payment websocket and play an effect on the jacket for each incoming payment."""
import asyncio
import logging
from collections.abc import Awaitable, Callable

import websockets

from .payments import is_incoming_payment

log = logging.getLogger(__name__)
MAX_BACKOFF = 30.0
RECONNECT_DELAY = 1.0
QUEUE_LIMIT = 20  # a burst of payments shouldn't keep the jacket busy for minutes

OnPayment = Callable[[], Awaitable[None]]


async def consume(messages, queue: asyncio.Queue) -> None:
    async for message in messages:
        if not is_incoming_payment(message):
            log.debug("ignored message: %.200s", message)
            continue
        if queue.full():
            log.warning("too many payments queued; skipping one")
            continue
        log.info("payment received")
        queue.put_nowait(message)


async def run_effects(queue: asyncio.Queue, on_payment: OnPayment) -> None:
    while True:
        await queue.get()
        try:
            await on_payment()
        except Exception as exc:  # keep listening even if the jacket is temporarily unreachable
            log.error("could not play effect: %s", exc)
        finally:
            queue.task_done()


async def listen(url: str, on_payment: OnPayment, connect=websockets.connect) -> None:
    queue: asyncio.Queue = asyncio.Queue(maxsize=QUEUE_LIMIT)
    worker = asyncio.create_task(run_effects(queue, on_payment))
    backoff = 1.0
    try:
        while True:
            try:
                async with connect(url, ping_interval=20, ping_timeout=20) as ws:
                    log.info("listening for payments")
                    backoff = 1.0
                    await consume(ws, queue)
                log.warning("websocket closed by server; reconnecting")
                await asyncio.sleep(RECONNECT_DELAY)
            except (OSError, websockets.WebSocketException, asyncio.TimeoutError) as exc:
                log.warning("websocket error: %s; retrying in %.0fs", exc, backoff)
                await asyncio.sleep(backoff)
                backoff = min(backoff * 2, MAX_BACKOFF)
    finally:
        worker.cancel()
