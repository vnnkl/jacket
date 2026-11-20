import asyncio, json
from jacket.listener import consume, run_effects, listen

async def agen(items):
    for i in items:
        yield i

PAY = json.dumps({"payment": {"amount": 1000, "status": "success"}})
OUT = json.dumps({"payment": {"amount": -1000, "status": "success"}})

async def test_consume_queues_only_incoming_payments():
    q = asyncio.Queue()
    await consume(agen([PAY, OUT, "noise", PAY]), q)
    assert q.qsize() == 2

async def test_consume_drops_when_queue_full():
    q = asyncio.Queue(maxsize=1)
    await consume(agen([PAY, PAY, PAY]), q)
    assert q.qsize() == 1

async def test_run_effects_survives_failures():
    q, calls = asyncio.Queue(), []
    async def effect():
        calls.append(1)
        if len(calls) == 1:
            raise RuntimeError("jacket gone")
    task = asyncio.create_task(run_effects(q, effect))
    q.put_nowait(PAY); q.put_nowait(PAY)
    await asyncio.wait_for(q.join(), 1)
    task.cancel()
    assert len(calls) == 2

class FakeWS:
    def __init__(self, msgs): self.msgs = msgs
    async def __aenter__(self): return agen(self.msgs)
    async def __aexit__(self, *a): return False

async def test_listen_plays_effect_per_payment_and_reconnects():
    connects, played = [], asyncio.Event()
    count = []
    def connect(url, **kw):
        connects.append(url)
        if len(connects) == 1:
            raise OSError("network down")
        return FakeWS([PAY, PAY])
    async def effect():
        count.append(1)
        if len(count) >= 2:
            played.set()
    task = asyncio.create_task(listen("wss://x", effect, connect=connect))
    await asyncio.wait_for(played.wait(), 3)
    task.cancel()
    assert len(connects) >= 2 and len(count) >= 2
