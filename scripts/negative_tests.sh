#!/usr/bin/env bash
# Prove the two safety nets actually fire (run after scripts/build.sh):
#   1. NKRO cannot be re-enabled in the release keymap (shared users/falcon config.h #error).
#   2. An image that would reach the EEPROM pages fails to link
#      (flash0 ends at 0x0800F800 in the keyboard-local linker script).
#   3. Debug-only features (FLASHREAD, VIA_INSECURE, debug traces) are refused by the ship variant.
# Uses throw-away keymaps inside the staged copy; restores it afterwards.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QMK="$ROOT/qmk_firmware"
KB=falcon_hs_r2
STAGED="$QMK/keyboards/$KB"
LOG="$ROOT/artifacts/negative_tests.log"
export PATH="${FALCON_GCC_BIN:-/opt/homebrew/opt/arm-none-eabi-gcc@8/bin}:${FALCON_BINUTILS_BIN:-/opt/homebrew/opt/arm-none-eabi-binutils/bin}:$PATH"
export LC_ALL=C TZ=UTC QMK_HOME="$QMK" SKIP_VERSION=yes QMK_USERSPACE="$ROOT/userspace"

[[ -d "$STAGED" ]] || { echo "run scripts/build.sh first" >&2; exit 1; }
trap 'rsync -a --delete "$ROOT/keyboards/$KB/" "$STAGED/"' EXIT
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

# 1. POLICY-NKRO: NKRO forced on from the command line for the release keymap.
expect_failure falcon "POLICY-NKRO: NKRO must stay disabled" NKRO_ENABLE=yes

# 3. POLICY-VARIANTS: debug-only features are refused by the ship variant.
expect_failure falcon "POLICY-VARIANTS: FLASHREAD is dev-only" FALCON_FLASHREAD=yes
expect_failure falcon "POLICY-VARIANTS: VIA_INSECURE is dev-only" EXTRAFLAGS=-DVIA_INSECURE
expect_failure falcon "POLICY-VARIANTS: debug traces" EXTRAFLAGS=-DACTION_DEBUG

# 2. 20 KB of extra read-only data pushes the ~38 KB base image past 0x0800F800.
mkdir -p "$STAGED/keymaps/neg_oversize"
cp "$STAGED/keymaps/default/keymap.c" "$STAGED/keymaps/neg_oversize/keymap.c"
cat >> "$STAGED/keymaps/neg_oversize/keymap.c" <<'EOF'
const uint8_t falcon_ballast[20 * 1024] = {1};
// Referenced through a volatile index so --gc-sections cannot drop it.
void keyboard_post_init_user(void) {
    static volatile uint32_t idx;
    volatile uint8_t sink = falcon_ballast[idx];
    (void)sink;
}
EOF
expect_failure neg_oversize "region .flash0. overflowed\|will not fit in region"

(cd "$QMK" && make "$KB:neg_oversize:clean" >/dev/null 2>&1) || true
[[ $fail -eq 0 ]] && echo "RESULT: PASS" || echo "RESULT: FAIL"
exit $fail
