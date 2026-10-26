#!/usr/bin/env bash
# Reproducible, offline build of the falcon_kalam firmware.
#
# Layers and variants (docs/policy.md):
#   base  keyboards/falcon_kalam                normal QMK keyboard, keymap "default" (reference)
#   ship  userspace/.../keymaps/ship          daily image (POLICY-VARIANTS)
#   dev   userspace/.../keymaps/dev      every debug feature; both share userspace/users/falcon_kalam
#
#   scripts/build.sh            build + verify all three, write the stock/ship parity table
#   scripts/build.sh --twice    additionally rebuild ship and dev from clean and
#                               require byte-identical binaries
#
# Never flashes anything. Outputs go to artifacts/.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QMK="$ROOT/qmk_firmware"
KB=falcon_kalam
BASE_KM=default
RELEASE_KM=ship          # ship variant (POLICY-VARIANTS)
DEV_KM=dev               # dev variant: every debug feature, never the daily image
ART="$ROOT/artifacts"
LOG="$ART/build.log"
TWICE=no
[[ "${1:-}" == "--twice" ]] && TWICE=yes

# Pinned toolchain (Homebrew, keg-only) -- prepend so nothing else on PATH wins.
GCC_BIN="${FALCON_KALAM_GCC_BIN:-/opt/homebrew/opt/arm-none-eabi-gcc@8/bin}"
BINUTILS_BIN="${FALCON_KALAM_BINUTILS_BIN:-/opt/homebrew/opt/arm-none-eabi-binutils/bin}"
export PATH="$GCC_BIN:$BINUTILS_BIN:$PATH"
export LC_ALL=C TZ=UTC QMK_HOME="$QMK"
export QMK_USERSPACE="$ROOT/userspace"   # policy layer (QMK External Userspace)
export SKIP_VERSION=yes                  # fixed QMK_VERSION/QMK_BUILDDATE -> reproducible image
# __DATE__/__TIME__ (embedded by QMK Command's version banner) follow the pinned
# QMK commit time, so the image does not depend on the day of the build.
export SOURCE_DATE_EPOCH="$(git -C "$QMK" log -1 --format=%ct HEAD)"

mkdir -p "$ART"
# Strip ANSI colour codes so the committed log is plain text.
exec > >(perl -pe 'BEGIN { $| = 1 } s/\e\[[0-9;]*[mK]//g' | tee "$LOG") 2>&1

step() { printf '\n==> %s\n' "$*"; }
die()  { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

step "Environment"
echo "date (UTC):   $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "host:         $(uname -srm)"
command -v arm-none-eabi-gcc >/dev/null || die "arm-none-eabi-gcc not found in $GCC_BIN"
echo "gcc:          $(arm-none-eabi-gcc --version | head -1)"
echo "objcopy:      $(arm-none-eabi-objcopy --version | head -1)"
echo "qmk cli:      $(qmk --version 2>/dev/null | tail -1)"
echo "python:       $(python3 --version)"
echo "make:         $(make --version | head -1)"
echo "userspace:    $QMK_USERSPACE"

step "Original artifacts unchanged"
(cd "$ROOT/originals" && shasum -a 256 -c SHA256SUMS) || die "originals/ does not match SHA256SUMS"

step "Re-derive hardware facts from the manufacturer firmware"
python3 -I "$ROOT/tools/analyze_vendor_firmware.py" --report "$ART/vendor_analysis.txt" | tail -1

step "QMK source pinned, clean, and on the latest local release (docs/policy.md)"
# Gitlink recorded in the index (equals HEAD's after a commit; also works pre-commit).
PINNED="$(git -C "$ROOT" ls-files -s qmk_firmware | awk '{print $2}')"
ACTUAL="$(git -C "$QMK" rev-parse HEAD)"
echo "pinned:       $PINNED"
echo "checked out:  $ACTUAL"
[[ "$PINNED" == "$ACTUAL" ]] || die "qmk_firmware is not at the pinned commit"
"$ROOT/scripts/qmk_policy.sh" --offline || die "QMK version policy violated (docs/policy.md)"
git -C "$QMK" submodule status --recursive | sed 's/^/  /'
if git -C "$QMK" submodule status --recursive | grep -q '^[-+U]'; then
    die "qmk_firmware submodules are missing or not at their recorded commits"
fi

# Stage the base keyboard into the QMK tree. The copy is ignored via the
# submodule's info/exclude, so the pinned tree itself stays unmodified.
step "Stage base keyboards/$KB into qmk_firmware"
EXCLUDE="$(git -C "$QMK" rev-parse --git-path info/exclude)"
[[ "$EXCLUDE" = /* ]] || EXCLUDE="$QMK/$EXCLUDE"
grep -qxF "/keyboards/$KB/" "$EXCLUDE" 2>/dev/null || echo "/keyboards/$KB/" >> "$EXCLUDE"
rsync -a --delete --delete-excluded --exclude .DS_Store "$ROOT/keyboards/$KB/" "$QMK/keyboards/$KB/"
DIRTY="$(git -C "$QMK" status --porcelain --ignore-submodules=none)"
[[ -z "$DIRTY" ]] || { echo "$DIRTY"; die "qmk_firmware working tree is dirty"; }
echo "staged (tree clean)"

step "qmk lint (base keyboard, upstream rules)"
(cd "$QMK" && QMK_USERSPACE= qmk lint -kb "$KB" -km "$BASE_KM")

build_one() {
    local km="$1"
    (cd "$QMK" && make "$KB:$km:clean" >/dev/null && make "$KB:$km" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)")
}

collect() {
    local km="$1" name="${KB}_$1" out="$QMK/.build"
    cp "$out/$name.bin" "$out/$name.elf" "$out/$name.map" "$ART/"
    cp "$out/obj_$name/cflags.txt" "$ART/$name.cflags.txt"
    cp "$out/obj_$name/src/info_config.h" "$ART/$name.info_config.h"
    (cd "$ART" && shasum -a 256 "$name.bin" > "$name.bin.sha256" && cat "$name.bin.sha256")
}

step "Build base $KB:$BASE_KM (normal firmware, reference only)"
build_one "$BASE_KM"
collect "$BASE_KM"
grep -q "keymaps/$BASE_KM/keymap.c" "$ART/${KB}_$BASE_KM.cflags.txt" || die "base keymap not taken from the base keyboard"

for KM in "$RELEASE_KM" "$DEV_KM"; do
    step "Build $KB:$KM (base + userspace configuration)"
    build_one "$KM"
    collect "$KM"
    grep -q "$QMK_USERSPACE/keyboards/$KB/keymaps/$KM/keymap.c" "$ART/${KB}_$KM.cflags.txt" \
        || die "$KM keymap was not taken from the userspace configuration"
done

NAME="${KB}_${RELEASE_KM}"
DEV_NAME="${KB}_${DEV_KM}"
if [[ "$TWICE" == yes ]]; then
    for KM in "$RELEASE_KM" "$DEV_KM"; do
        step "Reproducibility: rebuild $KM from clean and compare"
        FIRST="$(shasum -a 256 "$ART/${KB}_$KM.bin" | awk '{print $1}')"
        build_one "$KM" >/dev/null
        SECOND="$(shasum -a 256 "$QMK/.build/${KB}_$KM.bin" | awk '{print $1}')"
        echo "first:  $FIRST"
        echo "second: $SECOND"
        [[ "$FIRST" == "$SECOND" ]] || die "$KM build is not reproducible"
        echo "reproducible: yes"
    done
fi

step "Verify base image (normal firmware: NKRO present, no VIA)"
python3 -I "$ROOT/scripts/verify_firmware.py" --profile base \
    --bin "$ART/${KB}_$BASE_KM.bin" --elf "$ART/${KB}_$BASE_KM.elf" \
    --keyboard-json "$ROOT/keyboards/$KB/keyboard.json" \
    --report "$ART/verification_base.txt" | tail -1

step "Verify dev image (POLICY-VARIANTS: every debug feature present)"
python3 -I "$ROOT/scripts/verify_firmware.py" --profile dev \
    --bin "$ART/$DEV_NAME.bin" --elf "$ART/$DEV_NAME.elf" \
    --keyboard-json "$ROOT/keyboards/$KB/keyboard.json" \
    --via-json "$ROOT/via/falcon_kalam.json" \
    --report "$ART/verification_dev.txt" | tail -1

step "Verify ship image (POLICY-VARIANTS: no debug-only features; NKRO absent, VIA present)"
python3 -I "$ROOT/scripts/verify_firmware.py" --profile release \
    --bin "$ART/$NAME.bin" --elf "$ART/$NAME.elf" \
    --keyboard-json "$ROOT/keyboards/$KB/keyboard.json" \
    --via-json "$ROOT/via/falcon_kalam.json" \
    --report "$ART/verification.txt"

step "Stock vs release feature parity (from the binaries)"
python3 -I "$ROOT/tools/feature_parity.py" "$ROOT/originals/manufacturer_firmware.bin" "$ART/$NAME.bin" \
    --report "$ART/feature_parity.txt"

step "Documentation consistency (POLICY-DOCS)"
python3 -I "$ROOT/scripts/check_docs.py" || echo "WARNING: docs out of date -- update docs/firmware.md before flashing (docs/operations.md §2)"

step "Done -- artifacts NOT flashed"
echo "ship: $ART/$NAME.bin"
echo "dev:  $ART/$DEV_NAME.bin"
