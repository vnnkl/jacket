#!/usr/bin/env bash
# Build and flash the jacket payment-light firmware. Settings are entered later on the admin page.
#   ./flash.sh                     USB: detect the board, build, flash, open the serial log
#   ./flash.sh --build-only [env]  build firmware.bin for uploading on the admin page (default env: esp32)
#   ./flash.sh --ota [host] [env]  build and upload over Wi-Fi (default host: jacket.local)
# Optional: ESP_PORT=/dev/cu.xxx, JACKET_ADMIN_PASSWORD for --ota without a prompt.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
FW="$ROOT/esp32"
BIN_OUT="$ROOT/firmware.bin"
PIO=(uvx --from platformio pio)
ESPTOOL=(uvx --from esptool esptool)

die() { echo "Error: $*" >&2; exit 1; }
command -v uv >/dev/null || die "uv is not installed. Install it with: brew install uv"

find_port() {
  if [[ -n "${ESP_PORT:-}" ]]; then echo "$ESP_PORT"; return; fi
  local ports=()
  for p in /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.wchusbserial* /dev/cu.usbmodem* /dev/ttyUSB* /dev/ttyACM*; do
    [[ -e "$p" ]] && ports+=("$p")
  done
  [[ ${#ports[@]} -eq 0 ]] && die "No ESP32 found. Plug it in with a data USB cable (some cables are charge-only), or set ESP_PORT."
  if [[ ${#ports[@]} -gt 1 ]]; then
    echo "Several serial ports found:" >&2
    select p in "${ports[@]}"; do [[ -n "$p" ]] && { echo "$p"; return; }; done
  fi
  echo "${ports[0]}"
}

detect_env() {
  local out chip
  out="$("${ESPTOOL[@]}" --port "$1" chip-id 2>&1)" || die "Could not talk to the board on $1. Hold BOOT while plugging in and retry.
$out"
  chip="$(grep -Eio 'ESP32(-[A-Z0-9]+)?' <<<"$out" | head -1 | tr '[:lower:]' '[:upper:]')"
  case "$chip" in
    ESP32-S3) echo esp32s3 ;;
    ESP32-C3) echo esp32c3 ;;
    ESP32)    echo esp32 ;;
    ESP32-S2) die "ESP32-S2 has no Bluetooth; it cannot talk to the jacket. Use an ESP32, ESP32-S3 or ESP32-C3." ;;
    *)        die "Unsupported or unknown chip '$chip'. Supported: ESP32, ESP32-S3, ESP32-C3." ;;
  esac
}

build() {
  "${PIO[@]}" run -d "$FW" -e "$1" >&2
  cp "$FW/.pio/build/$1/firmware.bin" "$BIN_OUT"
  echo "Built $BIN_OUT" >&2
}

build_only() {
  local env="${1:-esp32}"
  build "$env"
  echo "Upload it on the admin page under Firmware."
}

ota() {
  local host="${1:-jacket.local}" env="${2:-esp32}" pass="${JACKET_ADMIN_PASSWORD:-}"
  build "$env"
  [[ -z "$pass" ]] && { read -rsp "Admin password for $host: " pass; echo; }
  echo "Uploading to http://$host/update ..."
  local out code
  out="$(curl -sS --max-time 180 -w '\n%{http_code}' -u "admin:$pass" -F "firmware=@$BIN_OUT" "http://$host/update")" \
    || die "Could not reach $host. Is the jacket controller on and on the same Wi-Fi?"
  code="${out##*$'\n'}"
  [[ "$code" == 200 ]] || die "Upload failed (HTTP $code): ${out%$'\n'*}"
  echo "Uploaded. The controller restarts with the new firmware in a few seconds."
}

usb() {
  local port env
  port="$(find_port)"
  echo "Board port: $port"
  env="$(detect_env "$port")"
  echo "Board type: $env"
  echo "Building and flashing (first run downloads the ESP32 toolchain, ~10 min)..."
  "${PIO[@]}" run -d "$FW" -e "$env" -t upload --upload-port "$port"
  echo
  echo "Flashed. Settings you saved before are kept. On a fresh board:"
  echo "  1. Join the Wi-Fi 'Jacket-Setup' (password: jacketsetup) with your phone."
  echo "  2. The setup page opens (or go to http://192.168.4.1). Set the admin password, Wi-Fi and websocket URL."
  echo "Stop the Mac listener first (tmux kill-session -t jacket): the jacket accepts one Bluetooth connection."
  read -rp "Open the serial log now? [Y/n] " yn
  [[ "${yn:-Y}" =~ ^[Nn] ]] || "${PIO[@]}" device monitor -d "$FW" -e "$env" -p "$port"
}

main() {
  case "${1:-}" in
    --build-only) shift; build_only "$@" ;;
    --ota) shift; ota "$@" ;;
    -h|--help) sed -n '2,6p' "$0" ;;
    "") usb ;;
    *) die "Unknown option $1 (see --help)" ;;
  esac
}

main "$@"
