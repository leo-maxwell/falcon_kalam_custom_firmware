# Progress: release firmware vs stock firmware

Goal: **phase 1** = every stock feature, implemented the same way or better on
current QMK. **Phase 2** = improvements. Status as of release image
`falcon_hs_r2_falcon.bin` (SHA-256 in `artifacts/falcon_hs_r2_falcon.bin.sha256`).

Status key: **=** same as stock · **+** same function, done better ·
**×** excluded by policy · **?** needs the live probe or hardware to confirm ·
**new** not in stock.

Evidence: `artifacts/feature_parity.txt` (generated from both binaries on
every build), `artifacts/vendor_analysis.txt`, `artifacts/verification.txt`.
Nothing here is hardware-tested yet.

## Phase 1: feature parity

### Firmware platform

| Feature | Stock | Release | Status |
|---|---|---|---|
| Firmware base | QMK (version not embedded), ChibiOS `be44b3`, built 2025-03-14, GCC 10.1 | QMK **0.34.7** (latest), its pinned ChibiOS, GCC 8.5 | **+** newer QMK (policy); toolchain older, see roadmap |
| Build | not reproducible / unknown | offline, pinned, byte-reproducible, verified | **+** |
| LTO | yes | yes | **=** |
| Image size | 43,940 B | 44,092 B of 55,296 B | **=** (+152 B) |
| Flash layout | app `0x08002000`, EEPROM at the end of flash via F_SIZE | same addresses; flash size pinned; linker refuses overlap with EEPROM | **+** |
| Clock / USB clock | HSE 8 MHz ×9 = 72 MHz, USB ÷1.5 | identical (same `RCC_CFGR` literal) | **=** |
| JTAG off / SWD on, BKP bootloader flags | yes | identical board code | **=** |

### USB / host interface (from descriptors)

| Feature | Stock | Release | Status |
|---|---|---|---|
| VID:PID, bcdDevice, strings | `9DD1:9D89`, `0x0001`, "Falcon"/"Falcon" | same IDs; product string "Falcon HS R2" | **=** |
| Interfaces / endpoints / polling | 4 HID, EP 81/82/03/84/85, 1 ms | identical | **=** |
| Power, remote wakeup | 500 mA, yes | identical | **=** |
| Boot keyboard 6KRO | yes | yes | **=** |
| NKRO (report ID 6) | yes; breaks macOS Caps Lock / input source | **no NKRO code** (build fails if enabled) | **×** POLICY-NKRO (this is the fix) |
| Mouse keys, system, consumer reports | yes | yes | **=** |
| HID console (`qmk console`) | yes | yes | **=** |
| VIA raw HID | yes, protocol ≥ 12 (exact: **?**) | protocol `0x000D` | **+** (newer) |

### Keyboard functions

| Feature | Stock | Release | Status |
|---|---|---|---|
| Matrix pins, COL2ROW, scan delays | as recovered | identical pin tables, same QMK scan code | **=** |
| Debounce | 5 ms symmetric deferred | QMK default (same) | **=** |
| Bootmagic (Esc on plug-in → DFU + clear EEPROM) | yes | yes | **=** |
| Command (LShift+RShift+key: B bootloader, Backspace clear EEPROM, …) | yes | yes, QMK defaults | **=** (N "NKRO toggle" is a no-op) |
| EEPROM emulation | wear-leveling 2 KB / 1 KB logical | same, location deterministic | **+** |
| VIA dynamic keymap layers | **?** (probe `info`) | 4 | **?** |
| VIA macros | **?** (probe `info`) | 16, buffer = rest of the 1 KB EEPROM | **?** |
| VIA layout options | 3 options; default view = 2u backspace | same 3; default view = **HHKB** | **+** |
| RGB underglow | rgblight, 4 × WS2812 on PA1, GRB, bit-bang, all 43 effects | identical | **=** |
| RGB off on USB suspend | unknown | yes (`RGBLIGHT_SLEEP`) | **?**/+ |
| RGB defaults (mode, HSV, brightness cap) | **?** (probe `info` reads the current state) | QMK defaults, no cap | **?** |
| Grave-escape, space-cadet, magic keycodes | compiled in | compiled in (QMK defaults) | **=** |

### Default keymap

The stock default is a generic 60% map. The release uses **native HHKB**, as
required. Every stock capability is still reachable:

| Stock binding | Release |
|---|---|
| Esc = `QK_GESC` | Esc; `QK_GESC` assignable in VIA |
| Caps position = `LT(1, Caps)` | Control (HHKB); Caps = Fn+Tab |
| Fn+1…= = F1–F12, Fn+Esc = `` ` `` | Fn+1…= = F1–F12 (HHKB), `` ` `` on the top row |
| Fn+Tab = `QK_BOOT` | Bootmagic (Esc), Command LShift+RShift+B, or VIA |
| Fn+Q / Fn+W = RGB toggle / next mode | Fn+Z/X/C/V/B = toggle / mode / hue / sat / value |
| Bottom row LCtl·LGui·LAlt·Space·RAlt·Fn·RCtl (incl. `4,0`/`4,13`) | HHKB: Alt·GUI·Space·GUI·Alt; `4,0`/`4,13`/`4,14` pending L-1 |
| (none) | full HHKB Fn layer: arrows, Home/End/PgUp/PgDn, media, Ins/Del, PrtSc/ScrLk/Pause, keypad ops |

### Phase 1 summary

* **Done:** every stock firmware feature except NKRO, which is excluded on purpose.
* **Waiting on the live probe** (read-only, with the stock firmware running): exact stock VIA protocol, layer and macro counts, and RGB defaults. Run `tools/falcon_probe.py info` once, then update the **?** rows.
* **Waiting on hardware:** everything. Nothing has been flashed.

## Phase 2: improvements (roadmap)

| # | Improvement | Depends on | Notes |
|---|---|---|---|
| I1 | Settle the physical layout (L-1) and fix `LAYOUT_all` to match reality | probe / user | `docs/layout.md` |
| I2 | Back up and identify the bootloader, then drop POLICY-FLASHREAD if not wanted | first flash | `docs/bootloader.md` |
| I3 | `QK_REBOOT` that really restarts the app: override `mcu_reset()` to skip the `0x424D` flag, which the upstream bootloader mishandles | I2 (confirm the installed bootloader has the bug) | policy layer only |
| I4 | RGB power/brightness cap from a measured LED current | hardware | `RGBLIGHT_LIMIT_VAL` |
| I5 | Decide console/Command for daily use: keep for parity, or move to a debug variant (console can expose keypresses while debug is on) | user decision | R12 |
| I6 | Newer toolchain (stock used GCC 10.1; ours is Homebrew GCC 8.5) | policy decision | must keep reproducibility |
| I7 | Submit the base keyboard to upstream QMK | I1 | the base already passes upstream lint |
| I8 | Re-test NKRO on new QMK releases; lift POLICY-NKRO if macOS behaves | each QMK update | two-line change in the policy layer |
