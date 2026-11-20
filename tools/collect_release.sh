#!/usr/bin/env bash
# Collect one board's flashable files from a PlatformIO build into dist/ (used by CI and --from-source).
# Usage: tools/collect_release.sh <env> [dist-dir]
set -euo pipefail
env="$1"; dist="${2:-dist}"
root="$(cd "$(dirname "$0")/.." && pwd)"
build="$root/esp32/.pio/build/$env"
boot_app0="$(find "${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/packages" -path '*framework-arduinoespressif32/tools/partitions/boot_app0.bin' | head -1)"
[[ -f "$build/firmware.bin" ]] || { echo "No build for $env in $build" >&2; exit 1; }
[[ -n "$boot_app0" ]] || { echo "boot_app0.bin not found in the PlatformIO packages" >&2; exit 1; }
mkdir -p "$dist"
cp "$build/bootloader.bin" "$dist/$env-bootloader.bin"
cp "$build/partitions.bin" "$dist/$env-partitions.bin"
cp "$build/firmware.bin" "$dist/$env-firmware.bin"
cp "$boot_app0" "$dist/$env-boot_app0.bin"
echo "Collected $env into $dist"
