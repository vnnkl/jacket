"""Turn a websocket message (LNbits wallet update) into the incoming amount in sats, or None."""
import json
import math

TEST_TRIGGER = "jacket-test"
TEST_AMOUNTS = {"": 5000, "small": 100, "medium": 5000, "big": 21000}  # sats per test band
_IGNORED_STATUSES = {"pending", "failed", "expired", "cancelled"}
MSAT_PER_SAT = 1000


def _test_amount(message: str) -> int | None:
    """'jacket-test', 'jacket-test:big' or 'jacket-test:777' -> sats."""
    head, _, arg = message.partition(":")
    if head != TEST_TRIGGER:
        return None
    if arg in TEST_AMOUNTS:
        return TEST_AMOUNTS[arg]
    return int(arg) if arg.isdigit() and int(arg) > 0 else None


def incoming_sats(message: str | bytes) -> int | None:
    if isinstance(message, bytes):
        message = message.decode("utf-8", "replace")
    message = message.strip()
    if message.startswith(TEST_TRIGGER):
        return _test_amount(message)
    try:
        data = json.loads(message)
    except (ValueError, TypeError):
        return None
    payment = data.get("payment") if isinstance(data, dict) else None
    if not isinstance(payment, dict):
        return None
    msat = payment.get("amount")
    if not isinstance(msat, (int, float)) or isinstance(msat, bool) or msat <= 0:
        return None
    if str(payment.get("status", "success")).lower() in _IGNORED_STATUSES:
        return None
    return max(1, math.ceil(msat / MSAT_PER_SAT))
