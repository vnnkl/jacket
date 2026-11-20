import json
from jacket.payments import is_incoming_payment

def msg(payment):
    return json.dumps({"wallet_balance": 1000, "payment": payment})

def test_incoming_lnbits_payment():
    assert is_incoming_payment(msg({"amount": 21000, "status": "success"}))

def test_outgoing_payment_ignored():
    assert not is_incoming_payment(msg({"amount": -21000, "status": "success"}))

def test_pending_or_failed_ignored():
    assert not is_incoming_payment(msg({"amount": 21000, "status": "pending"}))
    assert not is_incoming_payment(msg({"amount": 21000, "status": "failed"}))

def test_missing_status_counts_when_amount_positive():
    assert is_incoming_payment(msg({"amount": 5000}))

def test_unknown_message_shapes_are_ignored():
    assert not is_incoming_payment("")
    assert not is_incoming_payment("not json")
    assert not is_incoming_payment(json.dumps({"hello": 1}))
    assert not is_incoming_payment(json.dumps([1, 2]))
    assert not is_incoming_payment(msg("oops"))
    assert not is_incoming_payment(msg({"amount": "lots"}))

def test_test_trigger():
    assert is_incoming_payment("jacket-test")
