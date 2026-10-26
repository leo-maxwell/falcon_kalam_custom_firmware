# Optional improvements

Ideas, not commitments. Move an item to `docs/devnotes.md` when work starts.
IDs are permanent.

| ID | Improvement | Value | Depends on / notes |
|---|---|---|---|
| I9 | **Self-recovery into DFU**: independent watchdog plus a boot-failure counter in a backup register; after repeated failed boots the firmware writes `0x424C` and resets, so the bootloader stays in DFU | removes the pad risk (R13) | must not trigger on normal resets; L2 only |
| I10 | **macOS Caps Lock / input-source key**: e.g. tap = switch input source, hold = Caps Lock; or Caps Word | addresses the original problem directly | L2 keymap code |
| I11 | **RGB status indication**: Caps Lock, Fn layer active, "about to enter DFU" | uses the only free light | rgblight layers; L2 |
| I8 | **NKRO without the macOS problem**: find the root cause, re-test on each QMK update, lift POLICY-NKRO | full NKRO | two-line change in L2 once fixed |
| I12 | **Version traceability**: report the git commit through VIA's firmware-version query or `bcdDevice` without breaking reproducibility | identify what is flashed | — |
| I13 | **Automated hardware tests**: script the probe, console traces and readback compare | repeatable post-flash checks | console parsing |
| I5 | **Debug image**: keep console + Command only in a separate debug build | closes R12 | user decision |
| I16 | **Remove FLASHREAD** now that the bootloader is backed up | closes R10 | user decision |
| I4 | **RGB brightness cap** from a measured LED-module current | USB power margin | measurement; `RGBLIGHT_LIMIT_VAL` in L2 |
| I6 | **Newer toolchain** (GCC ≥ 10, as stock used) | modern compiler | must stay reproducible; re-verify RGB timing |
| I7 | **Submit L1 upstream** to QMK | maintenance by upstream | L1 already passes upstream lint |
| I14 | **Remote repository + CI** (build, verify, doc check) | protection against local loss | needs a hosting decision |
| I15 | **Full-chip SWD backup** | complete image incl. option bytes via a second path | RDP is off; pads must be located |
| I17 | **Matrix test build** with `VIA_INSECURE` so the VIA key tester works | convenience | test-only image; never the daily image |
