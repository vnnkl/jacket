# jacket

Control the Hello Fairy LED jacket (controller `RSL100`, firmware 1.1.0) over Bluetooth from a Mac,
and make it play a disco rainbow every time a payment arrives.

Close the Hello Fairy app first: the controller accepts only one Bluetooth connection.

```bash
uv run jacket on | off
uv run jacket color red          # white, red, orange, yellow, green, cyan, blue, purple, pink, or hue 0-359
uv run jacket preset 7           # built-in effects 0-20
uv run jacket disco              # 3 s disco rainbow
uv run jacket show small|medium|big|<sats>
uv run jacket listen wss://…/api/v1/ws/<id>
```

Shows by payment size (all adjustable with `--min-sats --medium-from --big-from --small-color
--small-seconds --medium-seconds --big-seconds --idle`):

| Payment | Show |
|---|---|
| below `--min-sats` (0) | nothing |
| below 1,000 sats | 2 s pulse in one colour (yellow) |
| 1,000–9,999 sats | 3 s disco rainbow |
| 10,000+ sats | 10 s: accelerating rainbow sweep, accelerating disco strobe, white flashes and fade |

The jacket stays off between shows (`--idle`). Test through the websocket with the text
`jacket-test:small`, `jacket-test:medium`, `jacket-test:big` or `jacket-test:<sats>`.

`JACKET_ADDRESS` skips the Bluetooth scan; `JACKET_PAYMENT_WS` sets the websocket URL.
`listen` reacts to LNbits wallet updates with a positive `payment.amount` (msat),
queues bursts, and reconnects to both the websocket and the jacket on its own.

## ESP32 (runs without the Mac)

Supported boards: ESP32, ESP32-S3, ESP32-C3 (the ESP32-S2 has no Bluetooth). Wi-Fi must be 2.4 GHz.

```bash
./flash.sh    # plug the board in first; asks for Wi-Fi name, password and the websocket URL
```

The script detects the board, writes `esp32/include/secrets.h` (git-ignored), builds with PlatformIO
(via `uvx`, nothing to install but `uv`) and uploads. On boot the ESP32 joins Wi-Fi, keeps the payment
websocket open (TLS, Let's Encrypt roots pinned), connects to the jacket over Bluetooth, keeps it off,
and plays a 3 s disco rainbow for every incoming payment. Everything reconnects on its own.

Stop the Mac listener first (`tmux kill-session -t jacket`): the jacket accepts one Bluetooth connection.
Logic tests: `cd esp32 && uvx --from platformio pio test -e native`.
