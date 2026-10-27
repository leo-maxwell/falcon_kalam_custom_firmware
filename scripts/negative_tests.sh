#!/usr/bin/env bash
# Prove the four safety nets actually fire (run after scripts/build.sh), in this order:
#   1. NKRO cannot be re-enabled in the ship keymap (shared users/falcon_kalam config.h #error).
#   2. Every debug-only feature (FLASHREAD, VIA_INSECURE, each debug trace, the dev marker)
#      is refused by the ship variant (POLICY-VARIANTS guards in ship/keymap.c).
#   3. An image that would reach the EEPROM pages fails to link
#      (flash0 ends at 0x0800F000 in the keyboard-local linker script).
#   4. scripts/flash.sh refuses the L1 image and any hash that is not the approved one.
# Test 3 uses a throw-away keymap inside the staged copy, restored afterwards;
# the others build the ship keymap with extra make variables.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/env.sh"   # ROOT, QMK, KB, toolchain, QMK environment
STAGED="$QMK/keyboards/$KB"
LOG="$ROOT/artifacts/negative_tests.log"

[[ -d "$STAGED" ]] || { echo "run scripts/build.sh first" >&2; exit 1; }
trap 'rsync -a --delete --delete-excluded --exclude .DS_Store "$ROOT/keyboards/$KB/" "$STAGED/"' EXIT
exec > >(tee "$LOG") 2>&1

fail=0
expect_failure() {
    local name="$1" pattern="$2"; shift 2
    local out
    if out="$(cd "$QMK" && make "$KB:$name" "$@" 2>&1 | sed $'s/\x1b\\[[0-9;]*[mK]//g'; exit "${PIPESTATUS[0]}")"; then
        echo "FAIL  $name: build unexpectedly succeeded"; fail=1
    elif grep -q -- "$pattern" <<<"$out"; then
        echo "PASS  $name: rejected ($(grep -m1 -- "$pattern" <<<"$out" | sed 's/^[[:space:]]*//' | cut -c1-140))"
    else
        echo "FAIL  $name: failed, but not with '$pattern'"; echo "$out" | tail -20; fail=1
    fi
}

# 1. POLICY-NKRO: NKRO forced on from the command line for the ship keymap.
expect_failure ship "POLICY-NKRO: NKRO must stay disabled" NKRO_ENABLE=yes

# 2. POLICY-VARIANTS: each debug-only feature is refused by the ship variant.
expect_failure ship "POLICY-VARIANTS: FLASHREAD is dev-only" FALCON_KALAM_FLASHREAD=yes
expect_failure ship "POLICY-VARIANTS: VIA_INSECURE is dev-only" EXTRAFLAGS=-DVIA_INSECURE
expect_failure ship "POLICY-VARIANTS: debug traces" EXTRAFLAGS=-DACTION_DEBUG
expect_failure ship "POLICY-VARIANTS: debug traces" EXTRAFLAGS=-DWEAR_LEVELING_DEBUG_OUTPUT
expect_failure ship "POLICY-VARIANTS: debug traces" KEYCODE_STRING_ENABLE=yes
expect_failure ship "POLICY-VARIANTS: debug traces" DEBUG_MATRIX_SCAN_RATE_ENABLE=yes
expect_failure ship "POLICY-VARIANTS: FALCON_KALAM_DEV" EXTRAFLAGS=-DFALCON_KALAM_DEV

# 3. 20 KB of extra read-only data pushes the ~38 KB base image past 0x0800F000.
mkdir -p "$STAGED/keymaps/neg_oversize"
cp "$STAGED/keymaps/default/keymap.c" "$STAGED/keymaps/neg_oversize/keymap.c"
cat >> "$STAGED/keymaps/neg_oversize/keymap.c" <<'EOF'
const uint8_t falcon_kalam_ballast[20 * 1024] = {1};
// Referenced through a volatile index so --gc-sections cannot drop it.
void keyboard_post_init_user(void) {
    static volatile uint32_t idx;
    volatile uint8_t sink = falcon_kalam_ballast[idx];
    (void)sink;
}
EOF
expect_failure neg_oversize "region .flash0. overflowed\|will not fit in region"

(cd "$QMK" && make "$KB:neg_oversize:clean" >/dev/null 2>&1) || true

# 4. POLICY-FLASH-GATE: scripts/flash.sh refuses anything but an approved ship/dev image
#    (check mode; each case is refused before any USB access).
HASH="$(awk '{print $1}' "$ROOT/artifacts/falcon_kalam_ship.bin.sha256")"
expect_script_failure() {
    local label="$1" pattern="$2"; shift 2
    local out
    if out="$("$ROOT/scripts/flash.sh" "$@" 2>&1)"; then
        echo "FAIL  flash.sh $label: accepted"; fail=1
    elif grep -q -- "$pattern" <<<"$out"; then
        echo "PASS  flash.sh $label: rejected ($out)"
    else
        echo "FAIL  flash.sh $label: wrong reason: $out"; fail=1
    fi
}
expect_script_failure "L1 reference image" "only 'ship' or 'dev'" check default "$HASH"
expect_script_failure "wrong hash" "does not match the approved" check ship "$(printf '0%.0s' {1..64})"
expect_script_failure "short hash" "full approved SHA-256" check ship abc
[[ $fail -eq 0 ]] && echo "RESULT: PASS" || echo "RESULT: FAIL"
exit $fail
