# Optional improvements

Ideas, not commitments. Move an item to `docs/devnotes.md` when work starts.
IDs are permanent; retired IDs are not reused. Ordered by gain for effort.

**Effort**: trivial (a few lines) · low (a day, offline) · medium (several
days, or needs the device) · high (needs extra hardware or tooling).
**Gain** for daily use or for maintaining the firmware: low · medium · high.

| ID | Improvement | Effort | Gain | Notes |
|---|---|---|---|---|
| I23 | **Mode and switch display**: a blink when Fn+Ctrl+M/W switches mode, and a key combination that shows the current mode and the saved switches (pending a replug) | low: one or two rgblight layers and a combination in `hhkb_modes.c` | **medium**: mode and switches are invisible today (L10), and a flipped switch waits for a replug | an addition the Classic lacks; acceptable because the RGB module is already a deviation (POLICY-RGB rows) |
| I14 | **Remote repository + CI** (build, verify, docs check on every push) | low for a private repository; medium for CI, which must reproduce the gcc@8 hashes on a hosted runner | **medium**: the only copy is local today; every change re-verified | needs a hosting decision |
| I12 | **Version traceability**: a release number in VIA's firmware-version query (`VIA_FIRMWARE_VERSION`, already read by `falcon_kalam_probe.py info`) | low: one define, bumped per release; a git hash would be circular, since images are committed | low–medium: shows what is flashed without entering DFU (today: `flash.sh verify`); Command V shows only the pinned QMK date | stays reproducible |
| I20 | **Physical channel mapping**: give the RGB channels meaningful positions (e.g. left pair / right pair) | trivial: `FALCON_KALAM_RGB_CH_A/B` in `rgb_indicators.h` | low: readability of the indicators | only after the dev-round LED test |
| I8 | **NKRO back on**: re-test NKRO on macOS with each QMK update; lift POLICY-NKRO once an upstream release fixes the problem | trivial once upstream fixes it: two L2 lines, flip the verifier checks | low: 6KRO is enough for typing, and the Classic declares an NKRO report but does not use it | waits on upstream (POLICY-UPSTREAM) |
| I13 | **Automated hardware tests**: script the probe, console traces and readback compare | high: console parsing; key presses still need a person | medium for maintenance: repeatable checks after each flash and QMK update | — |
| I4 | **RGB brightness cap** from a measured LED-module current | low in code; needs a USB current meter | low: the worst case (≈ 290 mA) is within the 500 mA budget (R14) | `RGBLIGHT_LIMIT_VAL` in L2 |
| I15 | **Full-chip SWD backup** | high: the SWD pads are not located; needs a GD32-capable probe | low: bootloader, option bytes, UID and application are already backed up; adds a second recovery path only | RDP is off |
| I21 | **Boot-protocol indicator**: show when the host uses the HID boot protocol | low: `usb_device_state_get_protocol()` and a channel-B colour | low: matters only in BIOS/firmware setup screens, rare with a Mac | — |
| I7 | **Submit L1 upstream** to QMK | medium: upstream review, naming of a clone, and the enlarged EEPROM is a project choice upstream may not accept | low: one unit; upstream would maintain L1 only | L1 already passes upstream lint |

Retired: I6 (newer toolchain: QMK's own toolchain is GCC 8, and POLICY-UPSTREAM
uses upstream as released), I10 (tap/hold Caps Lock key: POLICY-HHKB copies the
Classic's Control and Fn+Tab Caps Lock exactly, and POLICY-NKRO already solves
the macOS Caps Lock problem), I22 (DIP switches: done).
