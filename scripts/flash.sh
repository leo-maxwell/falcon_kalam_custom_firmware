#!/usr/bin/env bash
# The only sanctioned way to write firmware to the keyboard (POLICY-FLASH-GATE).
#
#   scripts/flash.sh check  <ship|dev> <sha256>   validate everything, touch nothing
#   scripts/flash.sh flash  <ship|dev> <sha256>   [WRITES DEVICE] flash the approved image
#   scripts/flash.sh verify <ship|dev>            read back (device in DFU) and compare
#
# Guards: only the ship or dev image; the full SHA-256 named in the approval
# must match the file, its .sha256 record and docs/firmware.md; DFU alt 2 is
# hard-coded (alt 1 erases 0x08002000); the device must be in DFU before
# anything is written; a pre-flash readback is always saved.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
eval "$(python3 -I "$ROOT/tools/falcon_kalam_facts.py" --shell)"   # FALCON_KALAM_DFU_ID, _APP_REGION_BYTES, _APP_START_SHORT
VIDPID="$FALCON_KALAM_DFU_ID"
ALT=2 # never 1: a download to alt 1 erases the page at the application start
FLASH_BYTES="$FALCON_KALAM_APP_REGION_BYTES" # the application region, bootloader end to flash end

die() { printf 'flash.sh: %s\n' "$*" >&2; exit 1; }
usage() {
    cat >&2 <<'EOF'
usage: scripts/flash.sh check  <ship|dev> <sha256>   validate everything, touch nothing
       scripts/flash.sh flash  <ship|dev> <sha256>   [WRITES DEVICE] flash the approved image
       scripts/flash.sh verify <ship|dev>            read back (device in DFU) and compare
EOF
    exit 2
}

[[ $# -ge 2 ]] || usage
MODE="$1" VARIANT="$2" WANT="${3:-}"
case "$MODE" in check | flash | verify) ;; *) usage ;; esac
case "$VARIANT" in
ship | dev) ;;
*) die "refusing '$VARIANT': only 'ship' or 'dev' may be flashed (the L1 reference image never)" ;;
esac
IMG="$ROOT/artifacts/falcon_kalam_$VARIANT.bin"
[[ -f "$IMG" ]] || die "$IMG missing (run scripts/build.sh)"
HAVE="$(shasum -a 256 "$IMG" | awk '{print $1}')"

validate() {
    [[ "$WANT" =~ ^[0-9a-f]{64}$ ]] || die "give the full approved SHA-256 (64 hex digits)"
    [[ "$HAVE" == "$WANT" ]] || die "image hash $HAVE does not match the approved $WANT"
    [[ -f "$IMG.sha256" ]] || die "$IMG.sha256 missing (run scripts/build.sh)"
    [[ "$(awk '{print $1}' "$IMG.sha256")" == "$HAVE" ]] || die "$IMG.sha256 does not match the image"
    # The hash must sit in the §1 row of this variant, not just anywhere in the file.
    grep -Eq "^\| \*\*$VARIANT\*\* \|[^|]*\|[^|]*\`$HAVE\`" "$ROOT/docs/firmware.md" || die "docs/firmware.md §1 does not list this hash for $VARIANT"
    tail -c 16 "$IMG" | head -c 11 | tail -c 3 | grep -q UFD || die "image has no DFU suffix"
    echo "ok: $VARIANT image $HAVE"
}

in_dfu() { dfu-util -l 2>/dev/null | grep -q "\[$VIDPID\].*alt=$ALT.*Upload to Flash $FALCON_KALAM_APP_START_SHORT"; }

readback() { # $1 = output file
    rm -f "$1"
    dfu-util -d "$VIDPID" -a "$ALT" -U "$1" -Z "$FLASH_BYTES" >/dev/null 2>&1 || true
    [[ "$(stat -f%z "$1" 2>/dev/null || echo 0)" == "$FLASH_BYTES" ]] || die "readback failed ($1)"
}

STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
mkdir -p "$ROOT/build"
case "$MODE" in
check)
    validate
    in_dfu && echo "ok: device in DFU" || echo "note: device not in DFU (hold Esc while plugging in)"
    ;;
flash)
    validate
    in_dfu || die "device not in DFU: unplug, hold Esc, plug in (LEDs red or dark), then retry"
    readback "$ROOT/build/readback_pre_$STAMP.bin"
    echo "ok: pre-flash readback build/readback_pre_$STAMP.bin"
    LOG="$ROOT/artifacts/flash_${STAMP}_$VARIANT.log"
    echo "flashing $VARIANT $HAVE via alt $ALT"
    # -R is required: the bootloader leaves dfuMANIFEST-WAIT-RESET only on a USB reset (dfu.c dfuUpdateByReset).
    dfu-util -d "$VIDPID" -a "$ALT" -D "$IMG" -R 2>&1 | tee "$LOG" | grep -E 'Download done|state\(|Error' || true
    grep -q 'Download done' "$LOG" || die "download did not complete (see $LOG); the device is still in DFU, retry or restore"
    echo "ok: written. Next: confirm 9DD1:9D89 enumerates, then the Esc->DFU test and 'scripts/flash.sh verify $VARIANT'"
    ;;
verify)
    in_dfu || die "device not in DFU: unplug, hold Esc, plug in, then retry"
    OUT="$ROOT/build/readback_verify_$STAMP.bin"
    readback "$OUT"
    N=$(($(stat -f%z "$IMG") - 16))
    cmp -s -n "$N" "$OUT" <(head -c "$N" "$IMG") || die "flash content differs from $VARIANT image ($OUT)"
    echo "ok: flash holds the $VARIANT image ($HAVE, $N bytes)"
    ;;
esac
