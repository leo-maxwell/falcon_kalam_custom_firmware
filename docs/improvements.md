# Optional improvements

Ideas, not commitments. Move an item to `docs/devnotes.md` when work starts.
IDs are permanent.

| ID | Improvement | Value | Depends on / notes |
|---|---|---|---|
| I9 | **Self-recovery into DFU**: independent watchdog plus a boot-failure counter in a backup register; after repeated failed boots the firmware writes `0x424C` and resets, so the bootloader stays in DFU | removes the pad risk (R13) | must not trigger on normal resets; L2 only |
| I10 | **macOS Caps Lock / input-source key**: e.g. tap = switch input source, hold = Caps Lock; or Caps Word | addresses the original problem directly | L2 keymap code |
| I8 | **NKRO without the macOS problem**: find the root cause, re-test on each QMK update, lift POLICY-NKRO | full NKRO | two-line change in L2 once fixed |
| I12 | **Version traceability**: report the git commit through VIA's firmware-version query or `bcdDevice` without breaking reproducibility | identify what is flashed | — |
| I13 | **Automated hardware tests**: script the probe, console traces and readback compare | repeatable post-flash checks | console parsing |
| I4 | **RGB brightness cap** from a measured LED-module current (relevant now: POLICY-RGB runs full white continuously, R14) | USB power and heat margin | measurement; `RGBLIGHT_LIMIT_VAL` in L2 |
| I19 | **Red also for Bootmagic DFU entry**: initialise the WS2812 and paint red in a Bootmagic hook before `bootloader_jump()` | removes most of the "dark" ambiguity (R15) | override `bootmagic_reset_eeprom` or `bootmagic_scan` in L2 |
| I20 | **Physical channel mapping**: once Shift+Shift+D shows where channel A sits, give the channels meaningful positions if needed (e.g. left pair / right pair) | readability | edit `FALCON_KALAM_RGB_CH_A/B` in `rgb_indicators.h` |
| I21 | **Boot-protocol indicator**: show when the host uses the HID boot protocol (BIOS/firmware setup screens) | diagnosis | `usb_device_state_get_protocol()`; a channel-B colour |
| I6 | **Newer toolchain** (GCC ≥ 10, as stock used) | modern compiler | must stay reproducible; re-verify RGB timing |
| I7 | **Submit L1 upstream** to QMK | maintenance by upstream | L1 already passes upstream lint |
| I14 | **Remote repository + CI** (build, verify, doc check) | protection against local loss | needs a hosting decision |
| I15 | **Full-chip SWD backup** | complete image incl. option bytes via a second path | RDP is off; pads must be located |
