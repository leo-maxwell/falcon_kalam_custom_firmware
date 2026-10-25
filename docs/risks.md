# Unresolved risks and unsupported features

Offline verification passed (`artifacts/verification.txt`,
`artifacts/verification_base.txt`, `artifacts/negative_tests.log`). None of
that proves the firmware works on the keyboard. **HR** marks items that need
a human decision or review before the first flash.

## Risks

| # | Risk | Status / impact | Evidence and mitigation |
|---|---|---|---|
| R1 **HR** | **The bootloader is not backed up yet.** DFU cannot read `0x08000000–0x08001FFF` or the option bytes. | Open / high if lost | Identified as the MIT-licensed STM32duino-bootloader (`docs/bootloader.md`); source pinned as a submodule. Neither firmware nor DFU alt 2 writes below `0x08002000`, and our linker script places nothing there. Backup route A: our firmware's read-only `FALCON_READ` command, right after the first flash. Route B: SWD before flashing (pads, probe, RDP state unknown). **Decide: accept route A (flash before the backup), or do SWD first.** |
| R2 | **A newer QMK on the GD32.** This is intended (POLICY-QMK-LATEST). The vendor ran ChibiOS `be44b3` (2025-03-14 build); ours is QMK 0.34.7's pinned ChibiOS. GD32F103 is not fully STM32F103-compatible. | Accepted by policy / medium | Same board files, clock tree, flash descriptor, pin tables and bootloader flags as the vendor (byte-level checks). The vendor image is the fallback (`recovery.md`). |
| R3 | **HSE frequency is inferred (8 MHz).** | Low / high | Follows from PLL ×9 plus two independent 72 MHz timing constants, and from USB needing 48 MHz at ÷1.5. |
| R4 | **The first boot resets persistent configuration.** | **Accepted by the user** | Nothing is lost anyway: the backup's EEPROM is the result of a Bootmagic erase (decoded write log, `vendor_analysis.txt` §7). `SKIP_VERSION=yes` fixes the VIA magic across our builds; clear EEPROM with Bootmagic if a later build changes the EEPROM layout. |
| R5 **HR** | **Physical layout is not settled: OPEN ISSUE L-1** (`docs/layout.md`). | **Open until the user settles it** | `LAYOUT_all` covers the 63 VIA positions. A read-only probe on the vendor firmware (`tools/falcon_probe.py matrix`) is ready and needs the keyboard out of DFU plus the user pressing every key. |
| R6 | **WS2812 bit-bang timing.** It is cycle-counted NOP timing. | Medium / low | Same driver, defaults, 72 MHz clock, PA1, 4 LEDs, GRB, all effects as the vendor (cross-checked with the second agent, `docs/rgb-crosscheck.md`). A failure shows up as wrong colours or flicker. |
| R7 **HR** | **Bootloader flag handling (source-confirmed upstream bug).** `bool flagSet` makes `0x424D` behave like `0x424C`. | Expected / low | If the device runs upstream code: after **every flash** it stays in DFU until replugged, and `QK_REBOOT` lands in DFU. **Never download to alt 1**: that erases `0x08002000`. The variant actually installed is unknown until it is dumped. |
| R8 | **The macOS Caps Lock/input-source fix is untested.** | Open | The release contains no NKRO code; the vendor had NKRO + Command "NKRO Toggle". Checklist §C4. |
| R9 | **GCC 8.5 toolchain** (the vendor used 10.1). | Low | Versions are logged; the build is reproducible with this toolchain. |
| R10 **HR** | **New custom firmware code: POLICY-FLASHREAD.** `via_command_kb` in the policy keymap reads memory for the host. | Needs code review | Only reads; whitelisted to the bootloader, option bytes and F_SIZE/UID; length ≤ 24; overflow-checked. Any host program can read those ranges, including the chip UID (a device identifier). Remove after the backup if not wanted. |
| R11 | **Wrong image flashed.** `falcon_hs_r2_default.bin` (base, with NKRO) sits next to the release. | Procedural | Only `falcon_hs_r2_falcon.bin` with the approved SHA-256 may be flashed (checklist §0). |

## Unsupported or intentionally omitted (release)

* **NKRO**: off by policy (POLICY-NKRO). It is still on in the base, as in a normal QMK build.
* **Console and Command**: the vendor had both (Command: LShift+RShift+key, including "NKRO Toggle" and "Jump to Bootloader"). They are omitted, as in normal QMK keyboards.
* **Matrix positions `4,0`, `4,13`, `4,14`** are not in any layout until L-1 is settled.
* **The vendor default keymap is not reproduced** (Caps = `LT(1,Caps)`, GESC, Fn+Tab = `QK_BOOT`). The release is HHKB as required. The vendor map is in `vendor_analysis.txt` §9.
* **`QK_BOOT` is not on any default layer.** Use Bootmagic (Esc held while plugging in) or assign it in VIA.
* **Fn+Esc = `KC_PWR`**, as on a real HHKB. On macOS it opens the shut-down dialog.
