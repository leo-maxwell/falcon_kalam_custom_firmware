# Unresolved risks and unsupported features

Offline verification passed (`artifacts/verification.txt`,
`artifacts/verification_base.txt`, `artifacts/negative_tests.log`). None of
that proves the firmware works on the keyboard. **HR** marks items that need
a human decision or review before the first flash.

## Risks

| # | Risk | Status / impact | Evidence and mitigation |
|---|---|---|---|
| R1 | **Bootloader backup.** | **Closed 2026-10-26** | Dumped via FALCON_READ, read twice, identical; byte-identical to upstream `generic-none_bootloader.bin`; RDP off. Stored in `originals/device_dump_2026-10-26/`. |
| R2 | **A newer QMK on the GD32.** This is intended (POLICY-QMK-LATEST). The vendor ran ChibiOS `be44b3` (2025-03-14 build); ours is QMK 0.34.7's pinned ChibiOS. GD32F103 is not fully STM32F103-compatible. | Accepted by policy / medium | Same board files, clock tree, flash descriptor, pin tables and bootloader flags as the vendor (byte-level checks). The vendor image is the fallback (`recovery.md`). |
| R3 | **HSE frequency is inferred (8 MHz).** | Low / high | Follows from PLL ×9 plus two independent 72 MHz timing constants, and from USB needing 48 MHz at ÷1.5. |
| R4 | **The first boot resets persistent configuration.** | **Accepted by the user** | Nothing is lost anyway: the backup's EEPROM is the result of a Bootmagic erase (decoded write log, `vendor_analysis.txt` §7). `SKIP_VERSION=yes` fixes the VIA magic across our builds; clear EEPROM with Bootmagic if a later build changes the EEPROM layout. |
| R5 | **Physical layout.** | **Closed 2026-10-26** | Settled by the user as HHKB classic; 53/60 positions confirmed by the read-only probe, the other 7 hidden by a probe decoder bug (fixed). Re-checked on hardware in §C5. `docs/layout.md`. |
| R13 | **No power-on DFU window.** | Mitigated 2026-10-26 | Confirmed: a replug never shows DFU. But the release's Bootmagic (Esc held at plug-in) entered DFU on the first try (`artifacts/post_flash_verification_2026-10-26.txt`). Pads are needed only if a future image fails to boot or scan keys, so test the Esc re-entry after every flash. |
| R6 | **WS2812 bit-bang timing.** It is cycle-counted NOP timing. | Medium / low | Same driver, defaults, 72 MHz clock, PA1, 4 LEDs, GRB, all effects as the vendor (cross-checked with the second agent, `docs/rgb-crosscheck.md`). A failure shows up as wrong colours or flicker. |
| R7 | **Bootloader flag handling.** | Mostly closed | Observed: after a download the device starts the app by itself, and Bootmagic/`QK_BOOT` → DFU works. The upstream-source `bool` bug is not present in the installed prebuilt binary. Still: **never download to alt 1** (erases `0x08002000`). |
| R8 | **The macOS Caps Lock/input-source fix is untested.** | Open | The release contains no NKRO code; the vendor had NKRO + Command "NKRO Toggle". Checklist §C4. |
| R9 | **GCC 8.5 toolchain** (the vendor used 10.1). | Low | Versions are logged; the build is reproducible with this toolchain. |
| R10 **HR** | **New custom firmware code: POLICY-FLASHREAD.** `via_command_kb` in the policy keymap reads memory for the host. | Needs code review | Only reads; whitelisted to the bootloader, option bytes and F_SIZE/UID; length ≤ 24; overflow-checked. Any host program can read those ranges, including the chip UID (a device identifier). Remove after the backup if not wanted. |
| R12 **HR** | **Console + Command (POLICY-PARITY), as in stock.** While debug is toggled on (LShift+RShift+D), keypress reports go to the HID console, which any host program can read. LShift+RShift+Backspace clears EEPROM. | Accepted for parity, pending decision | Debug is off by default and not persisted across EEPROM resets. Alternative: move both to a debug-only variant (roadmap I5). |
| R11 | **Wrong image flashed.** `falcon_hs_r2_default.bin` (base, with NKRO) sits next to the release. | Procedural | Only `falcon_hs_r2_falcon.bin` with the approved SHA-256 may be flashed (checklist §0). |

## Unsupported or intentionally omitted (release)

* **NKRO**: supported by the base (as in a normal QMK build); disabled by the release configuration (POLICY-NKRO).
* **Command "N" (NKRO toggle)** has no effect in the release, because the configuration compiles NKRO out.
* **Matrix positions `4,0`, `4,13`, `4,14`** are not in any layout until L-1 is settled.
* **The vendor default keymap is not reproduced** (Caps = `LT(1,Caps)`, GESC, Fn+Tab = `QK_BOOT`). The release is HHKB as required. The vendor map is in `vendor_analysis.txt` §9.
* **`QK_BOOT` is not on any default layer.** Use Bootmagic (Esc held while plugging in) or assign it in VIA.
* **Fn+Esc = `KC_PWR`**, as on a real HHKB. On macOS it opens the shut-down dialog.
