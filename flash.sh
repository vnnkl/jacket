#!/usr/bin/env bash
# Flash the jacket payment-light firmware onto an ESP32.
# Asks for Wi-Fi name/password and the payment websocket URL, detects the board, builds, uploads.
# Non-interactive: set WIFI_SSID, WIFI_PASSWORD, JACKET_PAYMENT_WS (and optionally JACKET_BLE_ADDRESS, ESP_PORT).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
FW="$ROOT/esp32"
SECRETS="$FW/include/secrets.h"
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

c_string() {  # escape for a C string literal
  local s="${1//\\/\\\\}"; s="${s//\"/\\\"}"; printf '"%s"' "$s"
}

ask_secrets() {
  local ssid="${WIFI_SSID:-}" pass="${WIFI_PASSWORD:-}" url="${JACKET_PAYMENT_WS:-}" addr="${JACKET_BLE_ADDRESS:-}"
  [[ -z "$ssid" ]] && read -rp "Wi-Fi name (2.4 GHz network): " ssid
  [[ -z "$pass" ]] && { read -rsp "Wi-Fi password: " pass; echo; }
  [[ -z "$url" ]] && read -rp "Payment websocket URL (wss://...): " url
  [[ -n "$ssid" ]] || die "Wi-Fi name is required."
  [[ "$url" =~ ^wss?://[^/]+ ]] || die "Websocket URL must start with ws:// or wss://"
  [[ -z "$addr" || "$addr" =~ ^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$ ]] || die "JACKET_BLE_ADDRESS must look like aa:bb:cc:dd:ee:ff"
  (
    umask 077
    {
      echo "// Written by flash.sh - contains your Wi-Fi password. Git-ignored; do not commit."
      echo "#pragma once"
      echo "#define WIFI_SSID $(c_string "$ssid")"
      echo "#define WIFI_PASSWORD $(c_string "$pass")"
      echo "#define PAYMENT_WS_URL $(c_string "$url")"
      echo "#define JACKET_ADDRESS $(c_string "$addr")"
    } > "$SECRETS"
  )
}

main() {
  local port env
  port="$(find_port)"
  echo "Board port: $port"
  env="$(detect_env "$port")"
  echo "Board type: $env"
  ask_secrets
  echo "Building and flashing (first run downloads the ESP32 toolchain, ~5 min)..."
  "${PIO[@]}" run -d "$FW" -e "$env" -t upload --upload-port "$port"
  echo
  echo "Flashed. Close the Hello Fairy app and stop the Mac listener (tmux kill-session -t jacket):"
  echo "the jacket accepts only one Bluetooth connection."
  read -rp "Open the serial monitor now? [Y/n] " yn
  [[ "${yn:-Y}" =~ ^[Nn] ]] || "${PIO[@]}" device monitor -d "$FW" -e "$env" -p "$port"
}

main "$@"
