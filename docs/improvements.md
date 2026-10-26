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
| I20 | **Physical channel mapping**: give the RGB channels and the settings-display LEDs meaningful positions (e.g. left pair / right pair) | open: needs the dev-round LED test | trivial: `FALCON_KALAM_RGB_CH_A/B` in `rgb_indicators.h` | low: readability of the indicators | after the dev-round LED test |
| I8 | **NKRO re-test**: with each QMK update, check upstream for a fix of the macOS Caps Lock / input-source problem; only then consider lifting POLICY-NKRO | watch | trivial once fixed: two L2 lines, flip the verifier checks | low: 6KRO is enough for typing, and the Classic declares an NKRO report but does not use it | nothing indicates a fix so far; not investigated here (POLICY-UPSTREAM) |
| I13 | **Automated hardware tests**: script the probe, console traces and readback compare | deferred | high: console parsing; key presses still need a person | **high** for maintenance: repeatable checks after each flash and QMK update | — |
| I14 | **Remote repository + CI** (build, verify, docs check on every push) | deferred | low for a private repository; medium for CI, which must reproduce the gcc@8 hashes on a hosted runner | medium: the only copy is local; every change re-verified | needs a hosting decision |
| I7 | **Submit L1 upstream** to QMK | deferred | medium: upstream review, naming of a clone, and the enlarged EEPROM is a project choice upstream may not accept | low: one unit; upstream would maintain L1 only | L1 already passes upstream lint |

Retired: I6 (newer toolchain: QMK's own toolchain is GCC 8, and POLICY-UPSTREAM
uses upstream as released), I10 (tap/hold Caps Lock key: POLICY-HHKB copies the
Classic's Control and Fn+Tab Caps Lock exactly, and POLICY-NKRO already solves
the macOS Caps Lock problem), I21 (boot-protocol indicator: the Classic has
none, and it is not needed), I12 (release number: done), I22 (DIP switches: done), I23 (settings display: done), I15 (SWD backup: no
probe is available; DFU must stay reachable instead).
