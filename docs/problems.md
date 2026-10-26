# Problems, risks and bugs

IDs are permanent: R = risk, B = bug, L = known limitation. **HR** = needs the
owner's decision. Closed items move to the ledger as one line.

## Open

| ID | Problem | Status / impact | Mitigation / next step |
|---|---|---|---|
| B1 | **Caps Lock LED never lights** on image `9b61141e…` (the A10 indicator was not configured) | **fixed** in the ship and dev images (indicator compiled in, verified offline); not flashed | flash and confirm the LED (devnotes) |
| R15 | **"Dark" is ambiguous** under POLICY-RGB | **fixed** in the images: Bootmagic paints the LEDs red too, so dark = host asleep. Red in DFU relies on the WS2812s latching the last frame (unverified) | confirm red in DFU (devnotes) |
| R13 | **No power-on DFU window**: a broken image could leave DFU reachable only through the pads or SWD | **partly fixed** (POLICY-RECOVERY): any CPU fault resets into DFU. Not safely fixable in firmware: hangs without a fault (a watchdog survives the reset and the bootloader never feeds it, so it would cut DFU sessions short), `chSysHalt()` (its hook is in L1's `chconf.h`), images that run but don't scan keys | flash only verified images through `scripts/flash.sh`; Esc→DFU test after every flash; dev fault test Shift+Shift+F (devnotes) |
| R2 | **Newer QMK/ChibiOS on the GD32** (intended, POLICY-QMK-LATEST) | the first flash proved USB, matrix scanning, EEPROM init and the DFU path | open: long-term wear-leveling writes. Dev-round persistence test with the wear-leveling trace; repeat after each QMK update |

## Known limitations

| ID | Limitation |
|---|---|
| L1 | In **ship** the VIA key tester and `falcon_kalam_probe.py matrix` see an all-zero matrix (QMK hides it unless built with `VIA_INSECURE`). Use dev, or the console (Shift+Shift+X) |
| L2 | Fn+Esc = Power in Mac mode, sent as Keyboard Power `0x66` like the real HHKB. What macOS does with it is unverified (devnotes) |
| L3 | `QK_BOOT` is on no default layer: use Esc at plug-in, Shift+Shift+B, or assign it in VIA |
| L4 | Command N ("NKRO toggle") does nothing in dev (no NKRO); in ship it types N |
| L5 | A DFU download to alt 1 erases `0x08002000`. `scripts/flash.sh` hard-codes alt 2; never call `dfu-util -D` by hand |
| L6 | The first boot of an image with a new settings schema resets the configuration, VIA keymaps and macros included (schema stamp, `policy.md` §5). VIA's own reset follows only QMK updates (pinned build date) |
| L7 | After a CPU fault the keyboard drops into DFU and stops typing until replugged. The LEDs keep their last colour (the fault handler must not touch them) |
| L8 | As on a real HHKB: in Win and HHK mode the Delete key sends Delete (Backspace = Fn+Delete, or SW3), and HHK mode has no GUI/Command key (◇ = Muhenkan/Henkan). HHK mode is the factory setting; Fn+Ctrl+M (until the next plug-in) or SW2 (from the next plug-in) selects Mac mode |
| L9 | All six VIA layers belong to the HHKB modes (stock had two spare transparent layers); custom keys go into a mode's layers |
| L10 | The virtual DIP switches have no display: a flip shows only as its blink (1 = off, 2 = on), and the setting through the keys' behaviour after the next plug-in (I23) |
| L11 | Deviations from the HHKB Classic, all accepted (`policy.md` §2.3). Noticeable in use: no SW6, so a key press always wakes a sleeping host; 6KRO only until upstream fixes NKRO on macOS (POLICY-NKRO, I8); SW1/SW2 both on acts as HHK mode |

## Ledger (closed)

| ID | Closed | How |
|---|---|---|
| R1 | 2026-10-26 | bootloader dumped twice through FLASHREAD, identical; = upstream `generic-none`; RDP off; stored in `originals/device_dump_2026-10-26/` |
| R3 | 2026-10-26 | HSE 8 MHz: inferred from code, confirmed indirectly by working USB at ÷1.5 |
| R4 | 2026-10-26 | the first-boot EEPROM reset is an accepted cost of the conversion; the backup held no user configuration |
| R5 | 2026-10-26 | the physical layout is HHKB classic; the probe saw 53 of 60 positions |
| R6 | 2026-10-26 | WS2812 timing and colour order statically identical to stock: same bit writer (T1H/T0L 5×6-NOP loops, 12-NOP T1L/T0H, 1344-pass latch, BASEPRI masking) and the same GRB buffer layout; enforced by the verifier on every build. Visual confirmation comes with the dev round |
| R7 | 2026-10-26 | the upstream bootloader source has a `bool` flag bug that would keep the device in DFU after a download; the installed prebuilt binary does not (the app started by itself) |
| R8 | 2026-10-26 | without NKRO, Caps Lock and input-source switching work on macOS, also after a replug (image `9b61141e…`); no later image has NKRO code |
| R9 | 2026-10-26 | GCC 8.5 is QMK's own macOS toolchain (the Homebrew `qmk` formula depends on `arm-none-eabi-gcc@8`); a newer compiler stays optional (I6) |
| R10 | 2026-10-26 | FLASHREAD exposes the chip UID, so it is dev-only, enforced by POLICY-VARIANTS (guard, verifier, negative test) |
| R11 | 2026-10-26 | `scripts/flash.sh` accepts only `ship` or `dev`, with the full approved SHA-256 matching the file, its `.sha256` record and `firmware.md`; negative tests prove the refusals |
| R12 | 2026-10-26 | console + Command stay in ship for stock parity; POLICY-RGB shows magenta on channel A while keystrokes go to the console |
| R14 | 2026-10-26 | full-white current within budget by worst-case analysis: 4 × 60 mA (WS2812 maximum at full white) + MCU < 50 mA ≈ 290 mA, against the declared 500 mA (`bMaxPower`). A measurement stays optional (I4) |
| R16 | 2026-10-27 | deviations from the HHKB Classic settled: each is listed with its reason in `policy.md` §2.3 and accepted (L11). Virtual switch flips take effect at the next plug-in and the factory setting is HHK mode, as on the Classic |
| B2 | 2026-10-26 | `falcon_kalam_probe.py matrix` read VIA ≥ 12 replies one byte early; fixed; `decode-log` re-maps the affected log |
| B3 | 2026-10-26 | builds were reproducible only within one day: QMK Command embeds `__DATE__`. Fixed: `SOURCE_DATE_EPOCH` = pinned QMK commit time; the verifier checks the embedded date. Images `9b61141e…` (flashed) and `dd580803…` carry their build-day date; nothing else differs |
| B4 | 2026-10-26 | a macOS `.DS_Store` in `keyboards/falcon_kalam/` was staged into QMK and failed `qmk lint`; staging now uses `rsync --delete-excluded --exclude .DS_Store` |
