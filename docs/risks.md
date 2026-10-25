# Unresolved risks and unsupported features

Offline verification passed (`artifacts/verification.txt`,
`artifacts/negative_tests.log`). None of that proves the firmware works on the
keyboard. The items below stay open until a hardware test.

## Risks

| # | Risk | Likelihood / impact | Evidence and mitigation |
|---|---|---|---|
| R1 | **The bootloader is not backed up.** The readback starts at `0x08002000`, so `0x08000000–0x08001FFF` (Maple bootloader) and the option bytes were never saved. | Low / **high** | Neither firmware nor DFU alt 2 writes below `0x08002000`, and the linker script places nothing there. If the bootloader is lost anyway, the only way back is SWD (PA13/PA14 stay enabled) with an ST-Link and a matching STM32duino bootloader binary, which has not been identified. Recommended before the first flash: a full-chip SWD backup, if the hardware allows it (read-out protection state is unknown). |
| R2 | **A newer QMK/ChibiOS revision on the GD32.** The vendor's HAL revision is proven on this chip; ours (QMK 0.34.7, pinned ChibiOS) is not. GD32F103 is not fully STM32F103-compatible: flash timing, USB peripheral corner cases, and different peripheral reset/start-up timing. | Medium / medium | Same board files, clock tree, flash descriptor, pin tables and bootloader flags as the vendor image (checked byte-level by the verifier). The vendor image restores normal operation if ours fails to enumerate (see recovery.md). |
| R3 | **HSE frequency is inferred (8 MHz), not measured.** | Low / high | It follows from PLL ×9 plus two independent 72 MHz timing constants, and from the vendor's USB working at USBPRE /1.5. If wrong, USB would not enumerate on the vendor firmware either. |
| R4 | **The first boot resets persistent configuration.** VIA's EEPROM magic is derived from the build date, so our firmware rejects the vendor data and reinitialises EEPROM (keymap, RGB, eeconfig). | Certain / low | The vendor write log holds only 4 bytes; the full original state is in `originals/falcon_flash_backup.bin`. `SKIP_VERSION=yes` fixes the magic for our builds, so if a later build changes the EEPROM layout, clear EEPROM with Bootmagic. |
| R5 | **Physical layout positions not confirmed.** The vendor keymap uses `4,0/4,13/4,14`, which are absent from the VIA JSON. Which split variants (backspace, left shift, space) this unit has is unknown. | Medium / low | LAYOUT_all covers exactly the 63 VIA positions. The first hardware test must exercise every key with VIA's key tester (preflash checklist §C). |
| R6 | **WS2812 bit-bang timing.** It is cycle-counted NOP timing. GD32 flash execution speed and a different compiler/HAL can shift it. | Medium / low | Same driver, defaults and 72 MHz clock as the vendor. Failure shows up as wrong colours or flicker, not damage. LED placement, the vendor effect list and any brightness cap are unknown; all rgblight effects except RGB test are enabled at QMK default brightness (4 LEDs, worst case ≈ 240 mA). |
| R7 | **Bootloader entry behaviour is assumed.** That the Maple bootloader honours `BKP_DR10 = 0x424C/0x424D` and offers a DFU window at power-on follows QMK's stm32duino conventions; neither has been observed on this unit. | Medium / medium | Bootmagic (hold Esc while plugging in) calls `bootloader_jump()`, the same path the vendor's `QK_BOOT` (Fn+Tab in its keymap) used. Test bootloader re-entry immediately after the first flash (checklist §C1). |
| R8 | **macOS Caps Lock/input-source fix is untested.** Disabling NKRO is the requested fix; it has not been confirmed on the user's Mac. | — | Checklist §C4. |
| R9 | **GCC 8.5 toolchain.** It is the Homebrew keg QMK's formula installs. It is old, but it builds cleanly. | Low / low | Toolchain versions are recorded in `artifacts/build.log`; the build is reproducible only with this toolchain. |

## Unsupported or intentionally omitted

* **NKRO**: permanently disabled. The build fails if `NKRO_ENABLE` is set.
* **Matrix positions `4,0`, `4,13`, `4,14`** are not in any layout: there is no VIA or physical evidence for them.
* **Console/debug output, Command**: off, as in QMK defaults.
* **The vendor default keymap is not reproduced.** It used Caps = `LT(1, Caps)`, GESC and Fn+Tab = `QK_BOOT`. This firmware ships the native HHKB map; the vendor map is documented in `artifacts/vendor_analysis.txt` §9 and can be recreated in VIA.
* **`QK_BOOT` is not on any default layer.** It is reachable with Bootmagic or by assigning it in VIA.
* **Fn+Esc = `KC_PWR`**, as on a real HHKB. On macOS it opens the shut-down dialog.
* **The keymap is named `hhkb_via`, not `via`**, because QMK's lint reserves the name `via`. This has no functional effect.
