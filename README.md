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
./flash.sh                         # USB: detects the board, builds, flashes (nothing to type)
./flash.sh --build-only            # firmware.bin to upload on the admin page
./flash.sh --ota [jacket.local]    # build and upload over Wi-Fi (asks for the admin password)
```

**First setup:** join the Wi-Fi `Jacket-Setup` (default password `jacketsetup`) with a phone; the setup
page opens (or go to http://192.168.4.1). Set the admin password, Wi-Fi and payment websocket URL.
The first admin password can only be set through this hotspot.

**Admin page:** http://jacket.local (or the IP from the serial log) on the same Wi-Fi. Status, last 10
payments, test buttons, show settings (bands, colour, lengths, what to show between payments),
connections, admin password, firmware upload, restart and factory reset. Passwords and the websocket
URL are never shown again, only replaced. 5 wrong passwords lock logins for 30 s.

**BOOT button:** hold 3 s to reopen the setup hotspot (e.g. at a venue whose Wi-Fi blocks the page),
hold 10 s to wipe all settings.

The ESP32 joins Wi-Fi (falls back to the hotspot after 30 s), keeps the payment websocket open (TLS,
Let's Encrypt roots pinned), connects to the jacket over Bluetooth, and plays the small, medium or big
show per payment. Everything reconnects on its own; settings survive firmware updates.

Stop the Mac listener first (`tmux kill-session -t jacket`): the jacket accepts one Bluetooth connection.

Tests: `cd esp32 && uvx --from platformio pio test -e native` (shows byte for byte against the Mac,
settings validation). After changing `src/jacket/shows.py`, run `uv run python tools/gen_golden.py`.
Try the admin page without a board: `uv run python tools/mock_admin.py [--fresh]` → http://127.0.0.1:8765
(password `testpass1`).
