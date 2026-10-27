#!/usr/bin/env bash
# POLICY-QMK-LATEST (docs/policy.md): qmk_firmware must sit exactly on the
# newest upstream QMK release tag (X.Y.Z on master).
#
#   scripts/qmk_policy.sh --offline   pinned commit is a release tag and the newest
#                                     one in the local clone (no network; build.sh)
#   scripts/qmk_policy.sh --online    also compare against upstream (read-only network; the default)
#   scripts/qmk_policy.sh --update    fetch, check out the newest release tag, sync
#                                     submodules and stage the new gitlink (no commit)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QMK="$ROOT/qmk_firmware"
UPSTREAM=https://github.com/qmk/qmk_firmware.git
MODE="${1:---online}"

newest() { grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -t. -k1,1n -k2,2n -k3,3n | tail -1; }

head_tag="$(git -C "$QMK" tag --points-at HEAD | newest || true)"
local_newest="$(git -C "$QMK" tag | newest)"
echo "qmk HEAD:            $(git -C "$QMK" rev-parse --short HEAD) (release tag: ${head_tag:-none})"
echo "newest local tag:    $local_newest"

case "$MODE" in
--offline)
    [[ -n "$head_tag" ]] || { echo "POLICY: HEAD is not on a release tag"; exit 1; }
    [[ "$head_tag" == "$local_newest" ]] || { echo "POLICY: $head_tag is older than local tag $local_newest"; exit 1; }
    echo "policy (offline):    OK -- run 'scripts/qmk_policy.sh --online' to compare with upstream"
    ;;
--online)
    remote_newest="$(git ls-remote --tags --refs "$UPSTREAM" | awk '{print $2}' | sed 's#refs/tags/##' | newest)"
    echo "newest upstream tag: $remote_newest"
    [[ -n "$head_tag" ]] || { echo "POLICY: HEAD is not on a release tag -- run 'scripts/qmk_policy.sh --update'"; exit 1; }
    [[ "$head_tag" == "$remote_newest" ]] || { echo "POLICY: outdated -- run 'scripts/qmk_policy.sh --update'"; exit 1; }
    echo "policy (online):     OK"
    ;;
--update)
    git -C "$QMK" fetch --tags origin
    target="$(git -C "$QMK" tag | newest)"
    echo "checking out $target"
    git -C "$QMK" checkout --quiet "refs/tags/$target"
    git -C "$QMK" submodule sync --recursive
    git -C "$QMK" submodule update --init --recursive
    git -C "$ROOT" add qmk_firmware
    echo "staged gitlink -> $target. Next: scripts/build.sh --twice, review, commit."
    ;;
*)
    echo "usage: $0 [--offline|--online|--update]" >&2; exit 2 ;;
esac
