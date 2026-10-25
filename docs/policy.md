# Project policies

## POLICY-QMK-LATEST — always the latest QMK release

`qmk_firmware/` must point exactly at the **newest upstream QMK release tag**
(`X.Y.Z` on `master`). Being on a newer QMK than the vendor is intended.

| Check | When | Command |
|---|---|---|
| Pinned commit is a release tag and the newest tag in the local clone | every build (offline, fatal) | `scripts/qmk_policy.sh --offline` (called by `build.sh`) |
| Pinned tag equals upstream's newest | before each release, and periodically | `scripts/qmk_policy.sh --online` (read-only network) |
| Move to the newest release | when `--online` fails | `scripts/qmk_policy.sh --update`, then `scripts/build.sh --twice`, `scripts/negative_tests.sh`, review the diff of `artifacts/verification.txt`, commit |

State at the time of writing: pinned `180f394b`, which is tag **0.34.7** and
upstream `master` HEAD (checked online 2026-10-26).

An update changes the binary, so the pre-flash checklist applies again in
full. The vendor-equivalence checks in `verify_firmware.py` (pin tables, clock
constants, flash descriptor, bootloader flags) are what catch upstream changes
that matter for this hardware.

## POLICY-NKRO, POLICY-VIA, POLICY-FLASHREAD

These are firmware-configuration policies. They live only in the userspace
layer; see `docs/architecture.md`.

## POLICY-READONLY-DEVICE

Without explicit approval, tools in this repository may only read from the
keyboard: USB enumeration, DFU **upload** (device → host), and the VIA query
commands allowed by `tools/falcon_probe.py`'s whitelist. Flashing, erasing,
EEPROM reset and bootloader jumps require the checklist approval.
