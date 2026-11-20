# jacket

Control the Hello Fairy LED jacket (controller `RSL100`, firmware 1.1.0) over Bluetooth from a Mac,
and make it play a disco rainbow every time a payment arrives.

Close the Hello Fairy app first: the controller accepts only one Bluetooth connection.

```bash
uv run jacket on | off
uv run jacket color red          # white, red, orange, yellow, green, cyan, blue, purple, pink, or hue 0-359
uv run jacket preset 7           # built-in effects 0-20
uv run jacket disco              # 3 s disco rainbow
uv run jacket listen wss://…/api/v1/ws/<id> [--seconds 3] [--idle white|off|<colour>|preset:N]
```

`JACKET_ADDRESS` skips the Bluetooth scan; `JACKET_PAYMENT_WS` sets the websocket URL.
`listen` reacts to LNbits wallet updates with a positive `payment.amount` (and to the text `jacket-test`),
queues bursts, and reconnects to both the websocket and the jacket on its own.
