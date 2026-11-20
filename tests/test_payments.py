import json
from jacket.payments import incoming_sats

def msg(payment):
    return json.dumps({"wallet_balance": 1000, "payment": payment})

def test_incoming_lnbits_payment_in_sats():
    assert incoming_sats(msg({"amount": 21000, "status": "success"})) == 21
    assert incoming_sats(msg({"amount": 39431000, "fee": 0})) == 39431

def test_sub_sat_payment_rounds_up_to_one():
    assert incoming_sats(msg({"amount": 400})) == 1

def test_outgoing_payment_ignored():
    assert incoming_sats(msg({"amount": -21000, "status": "success"})) is None

def test_pending_or_failed_ignored():
    assert incoming_sats(msg({"amount": 21000, "status": "pending"})) is None
    assert incoming_sats(msg({"amount": 21000, "status": "failed"})) is None

def test_unknown_message_shapes_are_ignored():
    for m in ["", "not json", json.dumps({"hello": 1}), json.dumps([1, 2]), msg("oops"),
              msg({"amount": "lots"}), msg({"amount": True})]:
        assert incoming_sats(m) is None

def test_test_triggers():
    assert incoming_sats("jacket-test") == 5000
    assert incoming_sats("jacket-test:small") == 100
    assert incoming_sats("jacket-test:medium") == 5000
    assert incoming_sats("jacket-test:big") == 21000
    assert incoming_sats("jacket-test:777") == 777
    assert incoming_sats("jacket-test:nope") is None
