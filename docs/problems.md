# Problems, risks and bugs

IDs are permanent: R = risk, B = bug, L = known limitation. **HR** = needs the
owner's decision. Closed items move to the ledger as one line.

## Open

| ID | Problem | Status / impact | Mitigation / next step |
|---|---|---|---|
| B14 | **Dev light test freezes the settings display** (step 8): while the test runs, `falcon_kalam_rgb_task` returns before `show_task()`, so only the "now" half shows, its layers linger under steps 9–12, and the "after replug" half plays after "done". The same freeze hits Fn+Ctrl+Shift+0 and the mode keys during the test; a restarted test can leave a reset or switch blink running over the new step, and the polled layers are not cleared at the start (dev shows debug blue beside steps 1–2) | open, dev only (found in the final review, 2026-10-27); ship unaffected | fix: call `show_task()` in the test branch, stop running blinks and clear the polled layers at `falcon_kalam_rgb_test_start`; needs a release and a dev check |

**HR** marks an item that needs the owner's decision.

## Known limitations

| ID | Limitation |
|---|---|
| L1 | In **ship** the VIA key tester and `falcon_kalam_probe.py matrix` see an all-zero matrix (QMK hides it unless built with `VIA_INSECURE`). Use dev, or the console (Shift+Shift+X) |
| L2 | Fn+Esc = Power in Mac mode, sent as Keyboard Power `0x66` like the real HHKB Classic. This Mac ignores it (dev release 4, 2026-10-27): the key does nothing. Kept on purpose: the Classic sends the same code (POLICY-HHKB) |
| L3 | `QK_BOOT` is on no default layer: use Esc at plug-in, Shift+Shift+B, or assign it in VIA |
| L4 | Command N ("NKRO toggle") types N in both images: no NKRO command is compiled. Caps (keyboard lock) and Z (sleep LED) likewise |
| L5 | A DFU download to alt 1 erases `0x08002000`. `scripts/flash.sh` hard-codes alt 2; never call `dfu-util -D` by hand |
| L6 | The first boot of an image with a new settings schema resets the configuration, VIA keymaps and macros included (schema stamp, `policy.md` §5). VIA's own reset follows only QMK updates (pinned build date) |
| L7 | After a CPU fault the keyboard drops into DFU and stops typing until replugged. The LEDs keep their last colour (the fault handler must not touch them) |
| L8 | As on the HHKB Professional Classic (POLICY-HHKB, kept on purpose): in Win and HHK mode the Delete key sends Delete (Backspace = Fn+Delete, or SW3 from the next plug-in), and HHK mode has no GUI/Command key (◇ = Muhenkan/Henkan). HHK mode is the factory setting, so on a Mac there is no Command key until Mac mode is selected: Fn+Ctrl+M (until the next plug-in) or SW2 (Fn+Ctrl+Shift+2, then replug) |
| L9 | All six VIA layers belong to the HHKB modes (stock had two spare transparent layers); custom keys go into a mode's layers |
| L11 | Deviations from the HHKB Classic, all accepted (`policy.md` §2.3). Noticeable in use: the Caps Lock LED lights 200 ms after the host sets Caps Lock (POLICY-CAPSLED); no SW6, so a key press always wakes a sleeping host; 6KRO only until upstream fixes NKRO on macOS (POLICY-NKRO, I8); SW1/SW2 both on acts as HHK mode |
| L14 | A hang **without** a CPU fault (an endless loop, `chSysHalt()`, an image that runs but does not scan keys) is not recoverable from the firmware: there is no power-on DFU window, a watchdog would keep running through the reset and abort DFU sessions, and the `chSysHalt()` hook is upstream's. A CPU fault does land in DFU (POLICY-RECOVERY). Mitigation: only images that passed the build's verification are flashed, through `scripts/flash.sh`, and every flash ends with the Esc→DFU test |

## Ledger (closed)

| ID | Closed | How |
|---|---|---|
| B1 | 2026-10-27 | Caps Lock LED: the A10 indicator is compiled in since the variant images; on the device it follows the host's Caps Lock (dev release 4) |
| B5 | 2026-10-27 | settings-reset blink lost to USB suspend during enumeration: it now starts once the host has configured the keyboard; seen as red ×3 on the device (dev release 6). Release 7 shows it on a dark background, so the normal state no longer appears between the flashes; release 8 kept a dark layer from post-init, but `rgblight_init` had already drawn the base, so green still flashed; release 9 stores rgblight off on a reset (`eeconfig_init_user` runs before `rgblight_init`) and switches it on when the blinks end; LED 1 still showed green, because it garbles the first frame after the data pin is set up (`hardware.md` §6), so release 10 sends the boot frame twice |
| B6 | 2026-10-27 | Eject sent QMK's `0xCC`; now the Classic's `0xB8` (release 6). macOS accepts it: Control+Eject opens the shut-down dialog (release 7); plain Eject shows nothing on this Mac (no disc drive) |
| B7 | 2026-10-27 | settings-display gap showed the normal state: an all-off layer covers it; dark on the device (dev release 6) |
| B8 | 2026-10-27 | dev fault test: its console line never arrived, because the 50 ms wait blocked the main loop that sends the console; release 7 faults from the main loop 200 ms later, and the line arrives |
| B9 | 2026-10-27 | plug-in flicker (green, off, green): QMK draws the base at start-up, and the host suspends the bus during enumeration. Release 11 keeps the LEDs dark until the host has configured the keyboard (at most 2 s), then lights them once |
| B10 | 2026-10-27 | dev: Shift+Shift+6–9 and F6–F9 selected default layers 6–9, which have no keymap, so every key read `KC_NO` until a replug (QMK's `MAGIC_KEY_SWITCH_LAYER_WITH_{N,F}KEYS` defaults); off in dev as in ship (release 13) |
| B11 | 2026-10-27 | a raw-HID rgblight "effect 0" command switched the base light off for the session, although the policy says VIA commands cannot change it: `enforce_green_base` now re-enables rgblight once the boot is over (release 13) |
| B12 | 2026-10-27 | FLASHREAD's window ended at `0x1FFFF7F0`, so the dump held 64 of the 96 UID bits; the window now ends at `0x1FFFF7F4` and the probe reads 20 B (release 13; `originals/device_dump_2026-10-26/` keeps the 16-byte dump) |
| B13 | 2026-10-27 | LED 1 flashed green at plug-in (yellow after the Bootmagic red): the first frame began after a high idle on A1 (QMK's board default is a pull-up, and the driver's init only switches the mode), so LED 1 read a leading 1 bit. Release 14 drives the line low and holds it for two reset periods before the first frame; verified on the device: no flash at plug-in, Bootmagic red without yellow |
| R2 | 2026-10-27 | QMK 0.34.7 / ChibiOS on the GD32: USB, matrix, EEPROM, DFU and the fault path verified over fourteen releases; settings persisted across every replug and flash. Wear: a settings change costs one or two 16-bit cells of the 1,024-cell write log, and the pages are erased only when the log is full, so even at the conservative 10,000 erase cycles the keyboard outlives millions of settings writes; VIA keymap edits are the same size. Re-checked by the verifier after each QMK update (operations.md §5) |
| R13 | 2026-10-27 | the fault path is fixed (POLICY-RECOVERY, verified with the dev fault test); what remains, a hang without a fault, cannot be fixed in firmware and is recorded as the limitation L14, mitigated by the flash gate and the Esc→DFU test after every flash |
| R15 | 2026-10-27 | "dark" no longer ambiguous: Bootmagic and Command B leave all four LEDs red in DFU on the device (Command: release 3; Bootmagic: release 6, after LED 1 had lost the first frame, now held low and sent twice) |
| R16 | 2026-10-27 | deviations from the HHKB Classic settled: each is listed with its reason in `policy.md` §2.3 and accepted (L11). Virtual switch flips take effect at the next plug-in and the factory setting is HHK mode, as on the Classic |
| L10 | 2026-10-27 | the virtual DIP switches and the mode had no display; Fn+Ctrl+Shift+0 and the mode keys now show them (I23) |
| L12 | 2026-10-27 | the plug-in flash was B13, not the bootloader's floating window; closed with it |
| B2 | 2026-10-26 | `falcon_kalam_probe.py matrix` read VIA ≥ 12 replies one byte early; fixed; `decode-log` re-maps the affected log |
| B3 | 2026-10-26 | builds were reproducible only within one day: QMK Command embeds `__DATE__`. Fixed: `SOURCE_DATE_EPOCH` = pinned QMK commit time; the verifier checks the embedded date. Images `9b61141e…` (flashed) and `dd580803…` carry their build-day date; nothing else differs |
| B4 | 2026-10-26 | a macOS `.DS_Store` in `keyboards/falcon_kalam/` was staged into QMK and failed `qmk lint`; staging now uses `rsync --delete-excluded --exclude .DS_Store` |
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
| R14 | 2026-10-26 | full-white current within budget by worst-case analysis: 4 × 60 mA (WS2812 maximum at full white) + MCU < 50 mA ≈ 290 mA, against the declared 500 mA (`bMaxPower`). The cap (POLICY-RGB; 69 of 255 since release 17) and QMK's CIE 1931 curve put each diode at 14/255 duty at most, one colour each: the LED share is now a few mA. No measurement is planned (I4 retired) |
