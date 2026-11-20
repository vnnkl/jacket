# jacket

An LED jacket (Hello Fairy Bluetooth controller) that plays a light show for every Lightning payment.

## Quick start: flash an ESP32

You need a Linux (incl. NixOS) or Mac computer, an ESP32, ESP32-S3 or ESP32-C3 board (not S2), and a
USB data cable. Flashing uses plain **esptool** with the firmware from the latest GitHub release;
no PlatformIO or compiler needed.

```bash
git clone https://github.com/vnnkl/jacket
cd jacket

# NixOS / Nix: get esptool, curl and picocom
nix-shell
# other Linux / macOS: pipx install esptool   (or: brew install esptool)

# plug in the board, then:
./flash.sh            # detects ESP32 / S3 / C3, downloads the matching firmware, checks it, flashes
./flash.sh --monitor  # serial log (optional)
```

On Linux your user needs serial access: NixOS `users.users.<you>.extraGroups = [ "dialout" ];`,
other distros `sudo usermod -aG dialout $USER` (log out and in again).

Then:
1. On your phone, join the Wi-Fi **Jacket-Setup** (password `jacketsetup`). The setup page opens
   by itself (otherwise go to http://192.168.4.1).
2. Set an admin password, your Wi-Fi (2.4 GHz) and the payment websocket URL, then save.
3. Rejoin your normal Wi-Fi and open **http://jacket.local** for the admin page and test buttons.

Close the Hello Fairy app on all phones: the jacket accepts only one Bluetooth connection.
If the board isn't found, try another USB cable or hold BOOT while plugging it in.

Flashing by hand (what `flash.sh` does; files from the release, `<b>` = esp32, esp32s3 or esp32c3,
bootloader offset `0x1000` on esp32 and `0x0` on S3/C3):

```bash
esptool --chip <b> --port /dev/ttyUSB0 write_flash \
  0x1000 <b>-bootloader.bin 0x8000 <b>-partitions.bin 0xe000 <b>-boot_app0.bin 0x10000 <b>-firmware.bin
```

Only these areas are written, so saved settings survive. Updates later: upload `<b>-firmware.bin`
on the admin page, or `./flash.sh --ota jacket.local <b>`.

## Mac command line (optional)

Control the jacket (controller `RSL100`, firmware 1.1.0) over Bluetooth from a Mac, and play the
payment shows from the Mac instead of an ESP32.

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
./flash.sh                         # USB: release firmware via esptool
./flash.sh --ota [host] [board]    # upload release firmware over Wi-Fi (asks for the admin password)
./flash.sh --from-source           # developers: build with PlatformIO, flash with esptool
```

Releases: pushing a tag `v*` makes GitHub Actions run the tests, build all three boards and publish
the files with `SHA256SUMS` (`.github/workflows/release.yml`).

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

## Presentation

A short non-technical deck about the project: [docs/slides/jacket.pdf](docs/slides/jacket.pdf)
(source `docs/slides/jacket.md`, made with [Marp](https://marp.app); text-only outline in
`docs/presentation.md`). Rebuild:

```bash
cd docs/slides
npx @marp-team/marp-cli --no-stdin --html --allow-local-files jacket.md --pdf -o jacket.pdf
npx @marp-team/marp-cli --no-stdin --html jacket.md -o jacket.html   # opens in any browser, press F for full screen
```
