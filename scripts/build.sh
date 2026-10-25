#!/usr/bin/env bash
# Reproducible, offline build of the Falcon HS R2 firmware.
#
#   scripts/build.sh            build the via keymap (the release artifact) and
#                               the default keymap (compile check only)
#   scripts/build.sh --twice    additionally rebuild from clean and require a
#                               byte-identical binary
#
# Never flashes anything. Outputs go to artifacts/.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QMK="$ROOT/qmk_firmware"
KB=falcon_hs_r2
RELEASE_KM=hhkb_via
ART="$ROOT/artifacts"
LOG="$ART/build.log"
TWICE=no
[[ "${1:-}" == "--twice" ]] && TWICE=yes

# Pinned toolchain (Homebrew, keg-only) -- prepend so nothing else on PATH wins.
GCC_BIN="${FALCON_GCC_BIN:-/opt/homebrew/opt/arm-none-eabi-gcc@8/bin}"
BINUTILS_BIN="${FALCON_BINUTILS_BIN:-/opt/homebrew/opt/arm-none-eabi-binutils/bin}"
export PATH="$GCC_BIN:$BINUTILS_BIN:$PATH"
export LC_ALL=C TZ=UTC QMK_HOME="$QMK"
export SKIP_VERSION=yes   # fixed QMK_VERSION/QMK_BUILDDATE -> reproducible image

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

step "Original artifacts unchanged"
(cd "$ROOT/originals" && shasum -a 256 -c SHA256SUMS) || die "originals/ does not match SHA256SUMS"

step "Re-derive hardware facts from the manufacturer firmware"
python3 -I "$ROOT/tools/analyze_vendor_firmware.py" --report "$ART/vendor_analysis.txt" | tail -1

step "QMK source pinned and clean"
# Gitlink recorded in the index (equals HEAD's after a commit; also works pre-commit).
PINNED="$(git -C "$ROOT" ls-files -s qmk_firmware | awk '{print $2}')"
ACTUAL="$(git -C "$QMK" rev-parse HEAD)"
echo "pinned:       $PINNED"
echo "checked out:  $ACTUAL ($(git -C "$QMK" describe --tags 2>/dev/null || echo untagged))"
[[ "$PINNED" == "$ACTUAL" ]] || die "qmk_firmware is not at the pinned commit"
git -C "$QMK" submodule status --recursive | sed 's/^/  /'
if git -C "$QMK" submodule status --recursive | grep -q '^[-+U]'; then
    die "qmk_firmware submodules are missing or not at their recorded commits"
fi

# Stage the board definition into the QMK tree. The copy is ignored via the
# submodule's info/exclude, so the pinned tree itself stays unmodified.
step "Stage keyboards/$KB into qmk_firmware"
EXCLUDE="$(git -C "$QMK" rev-parse --git-path info/exclude)"
[[ "$EXCLUDE" = /* ]] || EXCLUDE="$QMK/$EXCLUDE"
grep -qxF "/keyboards/$KB/" "$EXCLUDE" 2>/dev/null || echo "/keyboards/$KB/" >> "$EXCLUDE"
rsync -a --delete "$ROOT/keyboards/$KB/" "$QMK/keyboards/$KB/"
DIRTY="$(git -C "$QMK" status --porcelain --ignore-submodules=none)"
[[ -z "$DIRTY" ]] || { echo "$DIRTY"; die "qmk_firmware working tree is dirty"; }
echo "staged (tree clean)"

step "qmk lint"
(cd "$QMK" && qmk lint -kb "$KB" -km default && qmk lint -kb "$KB" -km "$RELEASE_KM")

build_one() {
    local km="$1"
    (cd "$QMK" && make "$KB:$km:clean" >/dev/null && make "$KB:$km" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)")
}

step "Build $KB:default (compile check)"
build_one default

step "Build $KB:$RELEASE_KM (release)"
build_one "$RELEASE_KM"

OUT="$QMK/.build"
NAME="${KB}_${RELEASE_KM}"
cp "$OUT/$NAME.bin" "$OUT/$NAME.elf" "$OUT/$NAME.map" "$ART/"
cp "$OUT/obj_$NAME/cflags.txt" "$ART/$NAME.cflags.txt"
cp "$OUT/obj_$NAME/src/info_config.h" "$ART/$NAME.info_config.h"
(cd "$ART" && shasum -a 256 "$NAME.bin" > "$NAME.bin.sha256" && cat "$NAME.bin.sha256")

if [[ "$TWICE" == yes ]]; then
    step "Reproducibility: rebuild from clean and compare"
    FIRST="$(shasum -a 256 "$ART/$NAME.bin" | awk '{print $1}')"
    build_one "$RELEASE_KM" >/dev/null
    SECOND="$(shasum -a 256 "$OUT/$NAME.bin" | awk '{print $1}')"
    echo "first:  $FIRST"
    echo "second: $SECOND"
    [[ "$FIRST" == "$SECOND" ]] || die "build is not reproducible"
    echo "reproducible: yes"
fi

step "Verify firmware image"
python3 -I "$ROOT/scripts/verify_firmware.py" \
    --bin "$ART/$NAME.bin" --elf "$ART/$NAME.elf" --map "$ART/$NAME.map" \
    --keyboard-json "$ROOT/keyboards/$KB/keyboard.json" \
    --report "$ART/verification.txt"

step "Done -- artifact NOT flashed"
echo "$ART/$NAME.bin"
