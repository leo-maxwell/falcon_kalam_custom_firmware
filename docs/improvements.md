# Optional improvements

Ideas, not commitments. Move an item to `docs/devnotes.md` when work starts.
IDs are permanent.

| ID | Improvement | Value | Depends on / notes |
|---|---|---|---|
| I10 | **macOS Caps Lock / input-source key**: e.g. tap = switch input source, hold = Caps Lock; or Caps Word | addresses the original problem directly | L2 keymap code |
| I8 | **NKRO back on**: re-test NKRO on macOS with each QMK update; lift POLICY-NKRO once an upstream release fixes the problem (it is not fixed here, POLICY-UPSTREAM) | full NKRO, and the Classic's NKRO report | two-line change in L2 |
| I23 | **Mode and switch display**: blink when Fn+Ctrl+M/W switches mode, and a key combination that shows the current mode and switch settings | both are invisible today (L10) | POLICY-RGB rows |
| I4 | **RGB brightness cap** from a measured LED-module current | USB power and heat margin (POLICY-RGB runs full white continuously, R14) | measurement; `RGBLIGHT_LIMIT_VAL` in L2 |
| I20 | **Physical channel mapping**: once Shift+Shift+D shows where channel A sits, give the channels meaningful positions if needed (e.g. left pair / right pair) | readability | `FALCON_KALAM_RGB_CH_A/B` in `rgb_indicators.h` |
| I21 | **Boot-protocol indicator**: show when the host uses the HID boot protocol (BIOS/firmware setup screens) | diagnosis | `usb_device_state_get_protocol()`; a channel-B colour |
| I12 | **Version traceability**: report the git commit through VIA's firmware-version query or `bcdDevice` without breaking reproducibility | identify what is flashed (Command V shows the pinned QMK date, not the build) | — |
| I13 | **Automated hardware tests**: script the probe, console traces and readback compare | repeatable post-flash checks | console parsing |
| I6 | **Newer toolchain** (GCC ≥ 10, as stock used) | modern compiler | must stay reproducible; re-verify RGB timing |
| I7 | **Submit L1 upstream** to QMK | maintenance by upstream | L1 already passes upstream lint |
| I14 | **Remote repository + CI** (build, verify, doc check) | protection against local loss | needs a hosting decision |
| I15 | **Full-chip SWD backup** | complete image incl. option bytes through a second path | RDP is off; pads must be located |
