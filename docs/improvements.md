# Optional improvements

Ideas, not commitments. Move an item to `docs/devnotes.md` when work starts;
an item starts only after its implementation details are confirmed. IDs are
permanent; retired IDs are not reused. Ordered by gain for effort; deferred
items last.

**Effort**: trivial (a few lines) · low (a day, offline) · medium (several
days, or needs the device) · high (needs extra hardware or tooling).
**Gain** for daily use or for maintaining the firmware: low · medium · high.

| ID | Improvement | Status | Effort | Gain | Notes |
|---|---|---|---|---|---|
| I23 | **Mode and switch display**: a blink when Fn+Ctrl+M/W switches mode, and a key combination that shows the current mode and the saved switches (pending a replug) | open; details to be confirmed | low: rgblight layers and a combination in `hhkb_modes.c` | **medium**: mode and switches are invisible today (L10), and a flipped switch waits for a replug | an addition the Classic lacks, on the RGB module (already a deviation, POLICY-RGB); depends on the LED-order test (I20) |
| I12 | **Version traceability**: a release number in VIA's firmware-version query (`VIA_FIRMWARE_VERSION`, already read by `falcon_kalam_probe.py info`) | open | low: one define, bumped per release; a git hash would be circular, since images are committed | low–medium: shows what is flashed without entering DFU (today: `flash.sh verify`); Command V shows only the pinned QMK date | stays reproducible |
| I20 | **Physical channel mapping**: give the RGB channels meaningful positions (e.g. left pair / right pair) | open | trivial: `FALCON_KALAM_RGB_CH_A/B` in `rgb_indicators.h` | low: readability of the indicators | after the dev-round LED test |
| I8 | **NKRO re-test**: with each QMK update, check upstream for a fix of the macOS Caps Lock / input-source problem; only then consider lifting POLICY-NKRO | watch | trivial once fixed: two L2 lines, flip the verifier checks | low: 6KRO is enough for typing, and the Classic declares an NKRO report but does not use it | nothing indicates a fix so far; not investigated here (POLICY-UPSTREAM) |
| I13 | **Automated hardware tests**: script the probe, console traces and readback compare | deferred | high: console parsing; key presses still need a person | **high** for maintenance: repeatable checks after each flash and QMK update | — |
| I14 | **Remote repository + CI** (build, verify, docs check on every push) | deferred | low for a private repository; medium for CI, which must reproduce the gcc@8 hashes on a hosted runner | medium: the only copy is local; every change re-verified | needs a hosting decision |
| I4 | **RGB brightness cap** from a measured LED-module current | deferred: no current meter available | low in code; needs a USB current meter | low: the worst case (≈ 290 mA) is within the 500 mA budget (R14) | `RGBLIGHT_LIMIT_VAL` in L2 |
| I7 | **Submit L1 upstream** to QMK | deferred | medium: upstream review, naming of a clone, and the enlarged EEPROM is a project choice upstream may not accept | low: one unit; upstream would maintain L1 only | L1 already passes upstream lint |
| I15 | **Full-chip SWD backup and recovery**: read and write the whole flash (bootloader included) through the chip's debug port | open | high: the SWD pads are not located; needs a GD32-capable probe | low: everything is already backed up through DFU and FLASHREAD; SWD adds the only path that works when the bootloader itself or a hanging image blocks DFU | RDP is off; `hardware.md` §2 |

Retired: I6 (newer toolchain: QMK's own toolchain is GCC 8, and POLICY-UPSTREAM
uses upstream as released), I10 (tap/hold Caps Lock key: POLICY-HHKB copies the
Classic's Control and Fn+Tab Caps Lock exactly, and POLICY-NKRO already solves
the macOS Caps Lock problem), I21 (boot-protocol indicator: the Classic has
none, and it is not needed), I22 (DIP switches: done).
