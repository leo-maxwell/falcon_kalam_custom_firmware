# Project policies

Each policy has an ID, a rule, where it lives, and what enforces it. A policy
changes only by editing this file together with its implementation and its
check.

## Firmware policies (implemented only in the L2 userspace layer)

| ID | Rule | Implemented in | Enforced by | To change |
|---|---|---|---|---|
| POLICY-NKRO | NKRO is a **configuration** decision, not a base decision: the base supports it (`keyboard.json` `"nkro": true`, verified in the base image), and the release configuration disables it at build time, because stock NKRO breaks the macOS Caps Lock / input-source toggle and stock's runtime NK_OFF did not survive a replug | `falcon/rules.mk` `NKRO_ENABLE = no`; `falcon/config.h` `#error` | `verify_firmware.py` release (no NKRO report, flag or code); `negative_tests.sh` | after an upstream fix is confirmed on macOS: delete both lines, flip the release checks (roadmap I8) |
| POLICY-VIA | Release is VIA-configurable | `falcon/rules.mk` | verifier: `-DVIA_ENABLE`, raw HID `0xFF60` | — |
| POLICY-PARITY | Release provides every stock feature: console, Command, LTO on top of the base's bootmagic/mousekey/extrakey/rgblight | `falcon/rules.mk` | verifier flags + console interface; `artifacts/feature_parity.txt` | `docs/parity.md` records any deliberate deviation |
| POLICY-FLASHREAD | Release can read (never write) the bootloader, option bytes and UID over raw HID, for backup | `falcon/keymap.c` `via_command_kb` | verifier symbol check; probe whitelist | remove after the bootloader backup if not wanted (roadmap I2) |
| POLICY-HHKB | HHKB is the default everywhere: keymaps, first layout, VIA layout option 0 | `keyboard.json` layout order, both keymaps, `via/falcon_hs_r2.json` | verifier: first layout and VIA default view = `LAYOUT_60_hhkb` | — |

## Project policies

| ID | Rule | Enforced by |
|---|---|---|
| POLICY-LAYERING | L1 base = hardware facts plus normal QMK features; every choice goes in L2. L0 QMK is never edited. | review; `build.sh` fails on a dirty QMK tree or if the release keymap is not from `userspace/` |
| POLICY-QMK-LATEST | `qmk_firmware` sits exactly on the newest upstream release tag. Being newer than the vendor is intended. | `qmk_policy.sh --offline` (every build, fatal); `--online` before each release; `--update` to move |
| POLICY-ORIGINALS | `originals/` is immutable evidence: read-only files, hashes in `SHA256SUMS` | `build.sh` hash check; files are mode 444 |
| POLICY-REPRODUCIBLE | Offline build with a pinned toolchain and sources; identical bytes on rebuild | `build.sh --twice` |
| POLICY-EVIDENCE | Hardware parameters come only from verified evidence. Anything unproven is marked unverified, never invented. | `analyze_vendor_firmware.py` (exact-address checks); `docs/hardware.md` status column |
| POLICY-READONLY-DEVICE | Without checklist approval, only read the device: enumeration, DFU **upload**, whitelisted VIA queries | probe whitelist; `docs/preflash-checklist.md` |
| POLICY-FLASH-GATE | Flash only `falcon_hs_r2_falcon.bin` with the approved hash, via DFU **alt 2** (alt 1 erases `0x08002000`), after the checklist approval | `docs/preflash-checklist.md` §0 |
| POLICY-OPEN-ISSUES | An open issue (currently L-1 layout) stays open until the user explicitly settles it in its document | `docs/layout.md` settlement table |

## QMK update procedure (POLICY-QMK-LATEST)

1. `scripts/qmk_policy.sh --online` fails → `scripts/qmk_policy.sh --update` (fetches, checks out the newest tag, syncs submodules, stages the gitlink).
2. `scripts/build.sh --twice` and `scripts/negative_tests.sh`.
3. Diff `artifacts/verification.txt` and `artifacts/feature_parity.txt` against the previous commit. Any change in vendor-equivalence or parity rows needs review.
4. Commit. The new image needs a fresh pre-flash approval.

State: pinned `180f394b` = tag 0.34.7 = upstream `master` (checked 2026-10-26).
