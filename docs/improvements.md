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
| I8 | **NKRO re-test**: with each QMK update, check upstream for a fix of the macOS Caps Lock / input-source problem; only then consider lifting POLICY-NKRO | watch | trivial once fixed: two L2 lines, flip the verifier checks | low: 6KRO is enough for typing, and the Classic declares an NKRO report but does not use it | nothing indicates a fix so far; not investigated here (POLICY-UPSTREAM) |
| I13 | **Automated hardware tests**: script the probe, console traces and readback compare | deferred | high: console parsing; key presses and replugs still need a person | **high** for maintenance: repeatable checks after each flash and QMK update | discussed 2026-10-27: the dev light test (Shift+Shift+L, release 15) now covers every indicator state without a replug; the probe (`info`, `dump-keymap`) and `console_keycheck.py` are the other pieces; a driver script would chain them |
| I14 | **CI** (build, verify, docs check on every push) | deferred | medium: the runner must reproduce the gcc@8 hashes | medium: every change re-verified | the repository is published at github.com/leo-maxwell/falcom_kalam_custom_firmware (main pushed 2026-10-27, submodules by reference); CI is the remaining part |

Retired: I7 (submit L1 upstream: a QMK or VIA registry entry would present
this firmware as the Kalam Falcon's own, which it is not; the repository is
its only home), I4 (brightness cap from a measured current: POLICY-RGB sets the cap,
and the LEDs now draw a few mA, R14), I6 (newer toolchain: QMK's own toolchain
is GCC 8, and POLICY-UPSTREAM uses upstream as released), I10 (tap/hold Caps
Lock key: POLICY-HHKB copies the Classic's Control and Fn+Tab Caps Lock
exactly, and POLICY-NKRO already solves the macOS Caps Lock problem), I12
(release number: done), I15 (SWD backup: no probe is available; DFU must stay
reachable instead), I20 (channel mapping: the LED test showed channel A = the
left pair, B = the right pair, already meaningful), I21 (boot-protocol
indicator: the Classic has none, and it is not needed), I22 (DIP switches:
done), I23 (settings display: done).
