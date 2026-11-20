#!/usr/bin/env bash
# Flash the jacket payment-light firmware with esptool. Settings are entered later on the admin page
# and survive reflashing. No PlatformIO needed: firmware comes from the GitHub release.
#
#   ./flash.sh                      detect the board on USB, download the release, flash it
#   ./flash.sh --monitor            show the board's serial log
#   ./flash.sh --ota [host] [board] upload the release firmware over Wi-Fi (default: jacket.local esp32)
#   ./flash.sh --from-source        developers: build with PlatformIO, then flash with esptool
#
# Options (env vars): ESP_PORT=/dev/ttyUSB0  JACKET_VERSION=v0.3.1 (default: latest)
#                     JACKET_ADMIN_PASSWORD=... (for --ota without a prompt)
# NixOS: run `nix-shell` in this folder first (provides esptool, curl, picocom).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
REPO="vnnkl/jacket"
VERSION="${JACKET_VERSION:-latest}"
CACHE="$ROOT/.firmware"
BAUD=460800

die() { echo "Error: $*" >&2; exit 1; }
need() { command -v "$1" >/dev/null || die "$1 is missing. $2"; }

esptool_cmd() {
  if command -v esptool >/dev/null; then echo esptool
  elif command -v esptool.py >/dev/null; then echo esptool.py
  elif command -v uvx >/dev/null; then echo "uvx --from esptool esptool"
  else die "esptool not found. NixOS: run 'nix-shell' here (or nix-shell -p esptool). Others: pipx install esptool"
  fi
}

find_port() {
  if [[ -n "${ESP_PORT:-}" ]]; then echo "$ESP_PORT"; return; fi
  local ports=() p
  for p in /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial* /dev/cu.usbmodem*; do
    [[ -e "$p" ]] && ports+=("$p")
  done
  [[ ${#ports[@]} -gt 0 ]] || die "No ESP32 found. Plug it in with a data USB cable (some are charge-only), or set ESP_PORT."
  if [[ ${#ports[@]} -gt 1 ]]; then
    echo "Several serial ports found:" >&2
    select p in "${ports[@]}"; do [[ -n "$p" ]] && { echo "$p"; return; }; done
  fi
  echo "${ports[0]}"
}

check_port_access() {
  [[ -r "$1" && -w "$1" ]] && return
  die "No permission to use $1. Add yourself to the 'dialout' group and log in again.
  NixOS: users.users.<you>.extraGroups = [ \"dialout\" ];   Others: sudo usermod -aG dialout \$USER"
}

# Maps esptool's chip description to a firmware build. Checks the variants first: plain ESP32
# reports as e.g. "ESP32-D0WD-V3".
board_for_chip() {
  case "$(tr '[:lower:]' '[:upper:]' <<<"$1")" in
    *ESP32-S3*) echo esp32s3 ;;
    *ESP32-C3*) echo esp32c3 ;;
    *ESP32-S2*) die "ESP32-S2 has no Bluetooth, so it cannot talk to the jacket. Use an ESP32, ESP32-S3 or ESP32-C3." ;;
    *ESP32-C2*|*ESP32-C6*|*ESP32-H2*|*ESP32-P4*) die "This chip isn't supported yet. Use an ESP32, ESP32-S3 or ESP32-C3." ;;
    *ESP32*) echo esp32 ;;
    *) die "Could not identify the chip. esptool said: $1" ;;
  esac
}

detect_board() {
  local out chip
  out="$($ESPTOOL --port "$1" chip_id 2>&1)" ||
    die "Could not talk to the board on $1. Hold BOOT while plugging it in and retry.
$out"
  chip="$(grep -Ei 'Chip (is|type)' <<<"$out" | head -1)"
  board_for_chip "${chip:-$out}"
}

sha256() { if command -v sha256sum >/dev/null; then sha256sum "$@"; else shasum -a 256 "$@"; fi; }

release_url() {
  if [[ "$VERSION" == latest ]]; then echo "https://github.com/$REPO/releases/latest/download/$1"
  else echo "https://github.com/$REPO/releases/download/$VERSION/$1"; fi
}

download_firmware() {  # $1 = board; files land in $CACHE/<board>-*.bin, checksum-verified
  local board="$1" f
  need curl "Install curl (NixOS: nix-shell)."
  mkdir -p "$CACHE"
  echo "Downloading firmware ($VERSION) for $board..."
  curl -fsSL "$(release_url SHA256SUMS)" -o "$CACHE/SHA256SUMS" || die "Could not download the release from github.com/$REPO."
  for f in bootloader partitions boot_app0 firmware; do
    curl -fsSL "$(release_url "$board-$f.bin")" -o "$CACHE/$board-$f.bin" || die "Release has no $board-$f.bin."
  done
  (cd "$CACHE" && grep " $board-" SHA256SUMS | sha256 -c --quiet -) || die "Checksum mismatch; the download is corrupt. Try again."
}

bootloader_offset() { [[ "$1" == esp32 ]] && echo 0x1000 || echo 0x0; }

# Writes only bootloader, partition table, OTA selector and app: the settings area (NVS) is untouched.
write_firmware() {  # $1 = port, $2 = board, $3 = directory with <board>-*.bin
  local port="$1" board="$2" dir="$3"
  $ESPTOOL --chip "$board" --port "$port" --baud "$BAUD" write_flash \
    "$(bootloader_offset "$board")" "$dir/$board-bootloader.bin" \
    0x8000 "$dir/$board-partitions.bin" \
    0xe000 "$dir/$board-boot_app0.bin" \
    0x10000 "$dir/$board-firmware.bin"
}

monitor() {
  local port; port="${1:-$(find_port)}"
  echo "Serial log on $port (115200 baud). Exit: Ctrl-A Ctrl-X (picocom) / Ctrl-] (miniterm)."
  if command -v picocom >/dev/null; then picocom -b 115200 "$port"
  elif python3 -c 'import serial' 2>/dev/null; then python3 -m serial.tools.miniterm "$port" 115200
  else die "Install picocom to see the log (NixOS: nix-shell)."; fi
}

after_flash() {
  cat <<'EOF'

Flashed. Saved settings are kept. On a fresh board:
  1. On your phone, join the Wi-Fi 'Jacket-Setup' (password: jacketsetup).
  2. The setup page opens (or go to http://192.168.4.1): set admin password, Wi-Fi, websocket URL.
  3. Then use http://jacket.local on your normal Wi-Fi.
Close the Hello Fairy app (and stop any Mac listener): the jacket accepts one Bluetooth connection.
EOF
  read -rp "Open the serial log now? [Y/n] " yn
  [[ "${yn:-Y}" =~ ^[Nn] ]] || monitor "$1"
}

flash_release() {
  local port board
  port="$(find_port)"; check_port_access "$port"
  echo "Board port: $port"
  board="$(detect_board "$port")"; echo "Board type: $board"
  download_firmware "$board"
  write_firmware "$port" "$board" "$CACHE"
  after_flash "$port"
}

flash_from_source() {
  local port board
  port="$(find_port)"; check_port_access "$port"
  board="$(detect_board "$port")"; echo "Board type: $board"
  need uvx "PlatformIO is run through uv: install uv first."
  uvx --from platformio pio run -d "$ROOT/esp32" -e "$board"
  "$ROOT/tools/collect_release.sh" "$board" "$ROOT/dist"
  write_firmware "$port" "$board" "$ROOT/dist"
  after_flash "$port"
}

ota() {
  local host="${1:-jacket.local}" board="${2:-esp32}" pass="${JACKET_ADMIN_PASSWORD:-}" out code
  download_firmware "$board"
  [[ -n "$pass" ]] || { read -rsp "Admin password for $host: " pass; echo; }
  echo "Uploading to http://$host/update ..."
  out="$(curl -sS --max-time 180 -w $'\n%{http_code}' -u "admin:$pass" -F "firmware=@$CACHE/$board-firmware.bin" "http://$host/update")" ||
    die "Could not reach $host. Is the controller on and on the same Wi-Fi?"
  code="${out##*$'\n'}"
  [[ "$code" == 200 ]] || die "Upload failed (HTTP $code): ${out%$'\n'*}"
  echo "Uploaded. The controller restarts with the new firmware in a few seconds."
}

main() {
  case "${1:-}" in
    "") ESPTOOL="$(esptool_cmd)"; flash_release ;;
    --monitor) monitor "${2:-}" ;;
    --ota) shift; ota "$@" ;;
    --from-source) ESPTOOL="$(esptool_cmd)"; flash_from_source ;;
    -h|--help) sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//' ;;
    *) die "Unknown option $1 (see --help)" ;;
  esac
}

main "$@"
