#!/usr/bin/env bash
# Disassemble the vendor application (DFU suffix stripped) at its link address
# 0x08002000. Output: build/vendor_app.dis (git-ignored). Addresses quoted in
# docs/hardware.md and tools/analyze_vendor_firmware.py refer to this listing.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OBJDUMP="${FALCON_KALAM_BINUTILS_BIN:-/opt/homebrew/opt/arm-none-eabi-binutils/bin}/arm-none-eabi-objdump"
BIN="$ROOT/originals/manufacturer_firmware.bin"
mkdir -p "$ROOT/build"
# The application is the file minus its 16-byte DFU suffix (43,940 B; analyze_vendor_firmware.py asserts it).
head -c "$(( $(stat -f%z "$BIN") - 16 ))" "$BIN" > "$ROOT/build/vendor_app.bin"
"$OBJDUMP" -D -b binary -marm -Mforce-thumb --adjust-vma=0x08002000 \
    "$ROOT/build/vendor_app.bin" > "$ROOT/build/vendor_app.dis"
echo "$ROOT/build/vendor_app.dis ($(wc -l < "$ROOT/build/vendor_app.dis") lines)"
