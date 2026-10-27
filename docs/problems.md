# Problems, risks and bugs

IDs are permanent: R = risk, B = bug, L = known limitation. **HR** = needs the
owner's decision. Closed items move to the ledger as one line.

## Open

| ID | Problem | Status / impact | Mitigation / next step |
|---|---|---|---|
| R13 | **No power-on DFU window**: a broken image could leave DFU reachable only through the pads; with no SWD probe, losing DFU bricks the unit | **partly fixed** (POLICY-RECOVERY; verified on the device: Shift+Shift+F lands in DFU): any CPU fault resets into DFU. Not safely fixable in firmware: hangs without a fault (a watchdog survives the reset and the bootloader never feeds it, so it would cut DFU sessions short), `chSysHalt()` (its hook `CH_CFG_SYSTEM_HALT_HOOK` is upstream's; only a keyboard-level `chconf.h` could override it, and a choice may not go into L1), images that run but don't scan keys | flash only verified images through `scripts/flash.sh`; Esc→DFU test after every flash; dev fault test Shift+Shift+F |
| R2 | **Newer QMK/ChibiOS on the GD32** (intended, POLICY-QMK-LATEST) | the first flash proved USB, matrix scanning, EEPROM init and the DFU path | open: long-term wear-leveling writes. Settings persisted across replugs in the dev round (2026-10-27); watch over weeks of use, and repeat the check after each QMK update |

## Known limitations

| ID | Limitation |
|---|---|
| L1 | In **ship** the VIA key tester and `falcon_kalam_probe.py matrix` see an all-zero matrix (QMK hides it unless built with `VIA_INSECURE`). Use dev, or the console (Shift+Shift+X) |
| L2 | Fn+Esc = Power in Mac mode, sent as Keyboard Power `0x66` like the real HHKB Classic. This Mac ignores it (dev release 4, 2026-10-27): the key does nothing. Kept on purpose: the Classic sends the same code (POLICY-HHKB) |
| L3 | `QK_BOOT` is on no default layer: use Esc at plug-in, Shift+Shift+B, or assign it in VIA |
| L4 | Command N ("NKRO toggle") does nothing in dev (no NKRO); in ship it types N |
| L5 | A DFU download to alt 1 erases `0x08002000`. `scripts/flash.sh` hard-codes alt 2; never call `dfu-util -D` by hand |
| L6 | The first boot of an image with a new settings schema resets the configuration, VIA keymaps and macros included (schema stamp, `policy.md` §5). VIA's own reset follows only QMK updates (pinned build date) |
| L7 | After a CPU fault the keyboard drops into DFU and stops typing until replugged. The LEDs keep their last colour (the fault handler must not touch them) |
| L8 | As on the HHKB Professional Classic (POLICY-HHKB, kept on purpose): in Win and HHK mode the Delete key sends Delete (Backspace = Fn+Delete, or SW3 from the next plug-in), and HHK mode has no GUI/Command key (◇ = Muhenkan/Henkan). HHK mode is the factory setting, so on a Mac there is no Command key until Mac mode is selected: Fn+Ctrl+M (until the next plug-in) or SW2 (Fn+Ctrl+Shift+2, then replug) |
| L9 | All six VIA layers belong to the HHKB modes (stock had two spare transparent layers); custom keys go into a mode's layers |
| L11 | Deviations from the HHKB Classic, all accepted (`policy.md` §2.3). Noticeable in use: no SW6, so a key press always wakes a sleeping host; 6KRO only until upstream fixes NKRO on macOS (POLICY-NKRO, I8); SW1/SW2 both on acts as HHK mode |
| L12 | At plug-in LED 1 can light in a dim random colour for a moment: the bootloader runs first and leaves the LED data line undriven (`hardware.md` §6). The firmware drives it and sends a dark frame at `keyboard_pre_init`, its earliest point (release 12); the bootloader's own time stays, as the bootloader is upstream. Accepted: no further firmware work. Hardware fix: a pull-down resistor on A1 |

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
| R9 | 2026-10-26 | GCC 8.5 is QMK's own macOS toolchain (the Homebrew `qmk` formula depends on `arm-none-eabi-gcc@8`); it stays, as POLICY-UPSTREAM uses upstream's toolchain |
| R10 | 2026-10-26 | FLASHREAD exposes the chip UID, so it is dev-only, enforced by POLICY-VARIANTS (guard, verifier, negative test) |
| R11 | 2026-10-26 | `scripts/flash.sh` accepts only `ship` or `dev`, with the full approved SHA-256 matching the file, its `.sha256` record and `firmware.md`; negative tests prove the refusals |
| R12 | 2026-10-26 | console + Command stay in ship for stock parity; POLICY-RGB shows red on channel A while keystrokes go to the console |
| R14 | 2026-10-26 | full-white current within budget by worst-case analysis: 4 × 60 mA (WS2812 maximum at full white) + MCU < 50 mA ≈ 290 mA, against the declared 500 mA (`bMaxPower`). Since release 3 the cap is a quarter of full scale (POLICY-RGB), and with QMK's CIE 1931 curve the diodes run at most at 12/255 duty, one colour each: the LED share is now a few mA. No measurement is planned (I4 retired) |
| R16 | 2026-10-27 | deviations from the HHKB Classic settled: each is listed with its reason in `policy.md` §2.3 and accepted (L11). Virtual switch flips take effect at the next plug-in and the factory setting is HHK mode, as on the Classic |
| L10 | 2026-10-27 | the virtual DIP switches and the mode had no display; Fn+Ctrl+Shift+0 and the mode keys now show them (I23) |
| B1 | 2026-10-27 | Caps Lock LED: the A10 indicator is compiled in since the variant images; on the device it follows the host's Caps Lock (dev release 4) |
| B5 | 2026-10-27 | settings-reset blink lost to USB suspend during enumeration: it now starts once the host has configured the keyboard; seen as red ×3 on the device (dev release 6). Release 7 shows it on a dark background, so the normal state no longer appears between the flashes; release 8 kept a dark layer from post-init, but `rgblight_init` had already drawn the base, so green still flashed; release 9 stores rgblight off on a reset (`eeconfig_init_user` runs before `rgblight_init`) and switches it on when the blinks end; LED 1 still showed green, because it garbles the first frame after the data pin is set up (`hardware.md` §6), so release 10 sends the boot frame twice |
| B7 | 2026-10-27 | settings-display gap showed the normal state: an all-off layer covers it; dark on the device (dev release 6) |
| B8 | 2026-10-27 | dev fault test: its console line never arrived, because the 50 ms wait blocked the main loop that sends the console; release 7 faults from the main loop 200 ms later, and the line arrives |
| B9 | 2026-10-27 | plug-in flicker (green, off, green): QMK draws the base at start-up, and the host suspends the bus during enumeration. Release 11 keeps the LEDs dark until the host has configured the keyboard (at most 2 s), then lights them once |
| B10 | 2026-10-27 | dev: Shift+Shift+6–9 and F6–F9 selected default layers 6–9, which have no keymap, so every key read `KC_NO` until a replug (QMK's `MAGIC_KEY_SWITCH_LAYER_WITH_{N,F}KEYS` defaults); off in dev as in ship (release 13) |
| B11 | 2026-10-27 | a raw-HID rgblight "effect 0" command switched the base light off for the session, although the policy says VIA commands cannot change it: `enforce_green_base` now re-enables rgblight once the boot is over (release 13) |
| B12 | 2026-10-27 | FLASHREAD's window ended at `0x1FFFF7F0`, so the dump held 64 of the 96 UID bits; the window now ends at `0x1FFFF7F4` and the probe reads 20 B (release 13; `originals/device_dump_2026-10-26/` keeps the 16-byte dump) |
| B6 | 2026-10-27 | Eject sent QMK's `0xCC`; now the Classic's `0xB8` (release 6). macOS accepts it: Control+Eject opens the shut-down dialog (release 7); plain Eject shows nothing on this Mac (no disc drive) |
| R15 | 2026-10-27 | "dark" no longer ambiguous: Bootmagic and Command B leave all four LEDs red in DFU on the device (Command: release 3; Bootmagic: release 6, after LED 1 had lost the first frame, now held low and sent twice) |
| B2 | 2026-10-26 | `falcon_kalam_probe.py matrix` read VIA ≥ 12 replies one byte early; fixed; `decode-log` re-maps the affected log |
| B3 | 2026-10-26 | builds were reproducible only within one day: QMK Command embeds `__DATE__`. Fixed: `SOURCE_DATE_EPOCH` = pinned QMK commit time; the verifier checks the embedded date. Images `9b61141e…` (flashed) and `dd580803…` carry their build-day date; nothing else differs |
| B4 | 2026-10-26 | a macOS `.DS_Store` in `keyboards/falcon_kalam/` was staged into QMK and failed `qmk lint`; staging now uses `rsync --delete-excluded --exclude .DS_Store` |
