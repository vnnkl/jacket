"""Decide whether a websocket message (LNbits wallet updates) is an incoming payment."""
import json

TEST_TRIGGER = "jacket-test"
_FINAL_FAILURES = {"pending", "failed", "expired", "cancelled"}


def is_incoming_payment(message: str | bytes) -> bool:
    if isinstance(message, bytes):
        message = message.decode("utf-8", "replace")
    if message.strip() == TEST_TRIGGER:
        return True
    try:
        data = json.loads(message)
    except (ValueError, TypeError):
        return False
    payment = data.get("payment") if isinstance(data, dict) else None
    if not isinstance(payment, dict):
        return False
    amount = payment.get("amount")
    if not isinstance(amount, (int, float)) or isinstance(amount, bool) or amount <= 0:
        return False
    return str(payment.get("status", "success")).lower() not in _FINAL_FAILURES
