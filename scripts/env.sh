#!/usr/bin/env bash
# Shared environment for scripts/build.sh and scripts/negative_tests.sh: the
# pinned toolchain and the QMK settings that make the build reproducible
# (POLICY-REPRODUCIBLE). Source it. Defines ROOT, QMK and KB; exports PATH,
# LC_ALL, TZ, QMK_HOME, QMK_USERSPACE, SKIP_VERSION and SOURCE_DATE_EPOCH.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QMK="$ROOT/qmk_firmware"
KB=falcon_kalam

# Pinned toolchain (Homebrew, keg-only) -- prepend so nothing else on PATH wins.
GCC_BIN="${FALCON_KALAM_GCC_BIN:-/opt/homebrew/opt/arm-none-eabi-gcc@8/bin}"
BINUTILS_BIN="${FALCON_KALAM_BINUTILS_BIN:-/opt/homebrew/opt/arm-none-eabi-binutils/bin}"
export PATH="$GCC_BIN:$BINUTILS_BIN:$PATH"
export LC_ALL=C TZ=UTC QMK_HOME="$QMK"
export QMK_USERSPACE="$ROOT/userspace"   # policy layer (QMK External Userspace)
export SKIP_VERSION=yes                  # fixed QMK_VERSION/QMK_BUILDDATE -> reproducible image
# __DATE__/__TIME__ (embedded by QMK Command's version banner) follow the pinned
# QMK commit time, so the image does not depend on the day of the build.
SOURCE_DATE_EPOCH="$(git -C "$QMK" log -1 --format=%ct HEAD)"
export SOURCE_DATE_EPOCH
