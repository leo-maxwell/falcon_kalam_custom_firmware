# Falcon HS R2 — hardware configuration and reverse-engineering notes

Every fact below is re-checked by `tools/analyze_vendor_firmware.py` against the
immutable originals (output: `artifacts/vendor_analysis.txt`). Addresses are
absolute flash addresses in the vendor application (link base `0x08002000`); a
listing is produced by `tools/disassemble_vendor.sh`.

Status legend: **Verified** = read directly from vendor code/data and consistent
with independent evidence. **Inferred** = derived from verified values by
calculation. **Unverified** = no firmware evidence; needs hardware testing.

## 1. Evidence base

| Artifact | SHA-256 | Notes |
|---|---|---|
| `originals/manufacturer_firmware.bin` | `90123fb1…2cb60b` | 43,956 B = 43,940 B application + 16 B DFU suffix (`UFD`, target `1EAF:0003`, CRC valid) |
| `originals/falcon_flash_backup.bin` | `4abf550a…45d9f2` | 57,344 B DFU upload of `0x08002000–0x0800FFFF`; first 43,940 B equal the application |
| `originals/manufacturer_jsonfile.json` | `0c9a5b8e…f0b754` | VIA v3 definition, 5×15, `9DD1:9D89`, `qmk_rgblight` menu |

The bootloader region `0x08000000–0x08001FFF` was **not** read back and is not
preserved (see `docs/risks.md`).

**What the vendor firmware is.** The image is a QMK build on ChibiOS for the
QMK board `STM32_F103_STM32DUINO`:
ChibiOS `ioline_t` pin encoding, SVC/SysTick vectors of the ChibiOS RT ARMv7-M
port, GPIO reset values identical to that board's `board.h`, the board's
`BKP->DR10` bootloader flags, the ChibiOS STM32F103xB flash descriptor, QMK
wear-leveling, QMK's HID report layout (report IDs 2/3/4/6), a VIA raw-HID
interface, and QMK ≥ 0.19 keycode numbering (`QK_GESC=0x7C16`, `QK_BOOT=0x7C00`,
`UG_TOGG=0x7820`). The VIA JSON uses the v3 format (`menus`), so the vendor
firmware speaks VIA protocol ≥ 12.

QMK's Command/console strings identify the build (`vendor_analysis.txt` §8b):
`BUILD: (Mar 14 2025)`, `CHIBIOS: "be44b3"`,
`CONTRIB: "develop_2021_q3-455-g77cb0a"`, `GCC: 10.1.0`,
`OPTIONS: BOOTMAGIC MOUSEKEY EXTRAKEY CONSOLE COMMAND NKRO LTO`. No QMK version
string is embedded.

That matters for GD32 compatibility: the vendor image demonstrates that this
ChibiOS STM32F1 HAL configuration (RCC, GPIO/AFIO, USB FS device, flash
programming, BKP/PWR, SysTick, DWT) works on *this* GD32F103C8T6 — but only for
the HAL revision the vendor used.

## 2. MCU, memory map, bootloader

| Item | Value | Status | Evidence |
|---|---|---|---|
| MCU | GigaDevice GD32F103C8T6, Cortex-M3 | Given | — |
| Flash | 64 KB, 1 KB pages | Verified | readback ends at `0x0800FFFF`; WL storage found in last 2 pages, placed via F_SIZE |
| SRAM | 20 KB | Given (datasheet), not measured | vendor initial SP `0x20000400` is consistent; size not proven by firmware |
| Bootloader | STM32duino-bootloader "boot20" (MIT, LeafLabs-derived), `1EAF:0003`, bcdDevice `0x0201`, alt 2, app at `0x08002000` | Verified (USB identity, 2026-10-26); variant unknown | `docs/bootloader.md`, `artifacts/device_inspection_2026-10-26.txt` |
| Bootloader flags | `BKP_DR10 = 0x424C` at boot (`0x080094B0`); `0x424D` before `mcu_reset` (`0x08007876`) | Verified | identical to QMK `STM32_F103_STM32DUINO` `board.c` / `bootloaders/stm32duino.c` |
| Flash descriptor | 128 × 1 KB sectors, base `0x08000000` (`0x0800C148`) | Verified | ChibiOS `STM32F103xB` registry — larger than the part; storage placement is clamped by flash size |

Flash map used by this project (enforced by
`keyboards/falcon_hs_r2/ld/STM32F103x8_stm32duino_falcon_hs_r2.ld`):

```
0x08000000 ┌──────────────────────────┐
           │ Maple/STM32duino bootldr │  8 KB   never written by this project
0x08002000 ├──────────────────────────┤
           │ application (flash0)     │ 54 KB   link fails if exceeded
0x0800F800 ├──────────────────────────┤
           │ wear-leveling EEPROM     │  2 KB   1 KB base image + 1 KB write log
0x08010000 └──────────────────────────┘
```

## 3. Clocks and USB

`stm32_clock_init` at `0x080090AC`:

| Register write | Meaning | Status |
|---|---|---|
| `RCC_CR \|= HSEON`, wait HSERDY | external crystal in use | Verified |
| `RCC_CFGR \|= 0x001D0000` | PLLSRC = HSE, PLLXTPRE = /1, PLLMUL = ×9 | Verified |
| `RCC_CFGR = 0x001D6400` | AHB /1, APB1 /2, APB2 /2, ADC /4, USBPRE = /1.5 | Verified |
| `FLASH_ACR = 0x12` | 2 wait states + prefetch | Verified |
| SW = PLL | SYSCLK from PLL | Verified |

SYSCLK = 72 MHz is shown independently twice: the matrix input delay is 18 NOPs
(`GPIO_INPUT_PIN_DELAY = CPU_CLOCK / 1 MHz / 4`, `0x08006146`) and the
post-row wait compares DWT `CYCCNT` against 2,160 cycles = 30 µs
(`MATRIX_IO_DELAY`, `0x080061B6`). With PLL ×9 this **infers HSE = 8 MHz**, and
USB = 72 MHz / 1.5 = 48 MHz. These are exactly the values in QMK's
`STM32_F103_STM32DUINO/configs/mcuconf.h`, which this build uses unchanged; the
verifier finds the same `0x001D6400` literal and PLL instruction in our image.

USB device descriptor (`0x0800BEE5`): USB 2.0, class 0, EP0 64 bytes, VID
`0x9DD1`, PID `0x9D89`, bcdDevice `0x0001`, manufacturer and product string
"Falcon", no serial. Our firmware reproduces the same 18-byte device descriptor.

## 4. JTAG / SWD

`boardInit` sets `AFIO_MAPR.SWJ_CFG = 010` (`0x080094AA`): JTAG-DP disabled,
**SW-DP kept enabled**. This releases PA15/PB3/PB4 — PB3 and PB4 are matrix rows,
so this remap is required — while PA13 (SWDIO) and PA14 (SWCLK) stay available
for SWD recovery. Our build uses the same `board.c`. **Verified.**

## 5. Matrix

| Item | Value | Status | Evidence |
|---|---|---|---|
| Rows (5) | `B3 B4 B5 B6 A2` | Verified | `row_pins[]` at `0x0800C464` |
| Cols (15) | `A9 A3 A4 A5 A6 A7 B0 B1 B10 B11 B12 B13 B14 B15 A8` | Verified | `col_pins[]` at `0x0800C10C` |
| Diode direction | `COL2ROW` | Verified | scan loop `0x08006100` (below) |
| Debounce | 5 ms, symmetric deferred (QMK default) | Inferred | `memcmp` of 10-byte raw matrix + "elapsed > 4 ms" at `0x08006208` |

Scan loop at `0x08006100`: for each entry of `row_pins` it calls
`palSetLineMode(row, 6)` (PAL_MODE_OUTPUT_PUSHPULL) and writes the pin mask to
`GPIOx->BRR` (drives the row **low**); it then reads `GPIOx->IDR` for each of the
15 `col_pins` and records a key when the bit reads **0**; afterwards
`unselect_row` (`0x080056A4`) returns the row to mode 3 (PAL_MODE_INPUT_PULLUP).
Rows are strobed low and columns are pulled-up inputs, which is QMK's `COL2ROW`.
The tentative pinout and direction supplied with the task are therefore
confirmed. Our compiled image contains both pin tables byte-for-byte identical to
the vendor's.

Physical key positions come from the VIA JSON (63 positions). The vendor's
compiled default keymap (2 layers at `0x0800C18A`) is a generic 60% map and also
assigns `4,0` (LCtrl), `4,13` (RCtrl) and `4,14` — positions the VIA JSON draws
as blockers. Whether switches exist at those positions is **unverified**; this
project does not use them.

| Variant (VIA option) | Positions |
|---|---|
| Split backspace: off / on | `0,14` 2u / `0,13` + `0,14` |
| Split left shift: off / on | `3,0` 2.25u / `3,0` 1.25u + `3,1` |
| Split spacebar: off / on | `4,7` 7u / `4,5` 3u + `4,7` 1u + `4,9` 3u |

## 6. Persistent storage (EEPROM emulation)

| Item | Value | Status |
|---|---|---|
| Driver | QMK wear-leveling on ChibiOS EFL (embedded flash) | Verified (`backing_store_init` at `0x080095xx`) |
| Placement | end of flash, from F_SIZE (`0x1FFFF7E0`, masked `(x & 0xFFFF) << 10`) | Verified |
| Region | `0x0800F800–0x0800FFFF` (2 KB backing, 1 KB logical) | Verified (readback: `0x0800F800–0x0800FBFF` erased, write log data at `0x0800FC08`) |
| F_SIZE on this unit | reads 64 KB | Inferred (storage landed at the 64 KB boundary) |
| Write width | 16-bit half-words (STM32F1 FPEC-compatible) | Verified indirectly: the vendor firmware wrote the log on this chip |

Our build pins `WEAR_LEVELING_EFL_FLASH_SIZE = 64 KB` (`config.h`), so placement
no longer depends on how a GD32 reports F_SIZE; the verifier confirms the image
does not read F_SIZE at all. Backing/logical sizes stay 2048/1024 as in the
vendor image, and the linker script keeps code out of those pages.

The write log decodes (flash stores `~value`) to two `OPTIMIZED_64` entries,
logical addresses 0 and 1 = `0xFF`, right after a full erase. That is exactly
QMK's `nvm_eeconfig_disable()`: format, then EECONFIG magic = `0xFFFF`. In other
words, Bootmagic (or Clear-EEPROM) ran just before the backup, and **the backup
contains no user configuration**.

## 7. RGB

| Item | Value | Status | Evidence |
|---|---|---|---|
| Driver | QMK WS2812 bit-bang | Verified | `0x08004AE8`: BSRR/BRR toggles with NOP-loop timing |
| Data pin | PA1 | Verified | `ws2812_init` sets GPIOA mask `0x0002` to output (`0x080056C8`); writer uses mask `0x0002` |
| LED count | 4 | Verified | `rgblight_set` → `ws2812_setleds(led, 4)` (`0x08004776`) |
| Byte order | GRB | Verified | `rgblight_setrgb` stores r→[1], g→[0], b→[2] (`0x080049F6`) |
| Reset time | 280 µs (QMK default) | Verified | 1,344 × 15-cycle loop at 72 MHz |
| Physical placement (underglow vs. indicators) | — | Unverified | — |
| Enabled effects | all 43 rgblight modes | Verified | `mode_base_table` at `0x0800C2B6` |
| Brightness cap | — | Unverified | not reconstructed |
| Cross-check with the second agent's notes | agree, except their table addresses | — | `docs/rgb-crosscheck.md` |

## 8. HID interfaces

| Interface | Vendor | This firmware |
|---|---|---|
| Boot keyboard (6KRO) | yes | yes |
| Shared EP: mouse (ID 2), system (ID 3), consumer (ID 4) | yes | yes |
| Shared EP: **NKRO bitmap (ID 6)** | **yes** | base: **yes**; release: **no**, disabled by configuration (POLICY-NKRO) |
| VIA raw HID (`0xFF60`/`0x61`, 32 B) | yes | yes (protocol `0x000D`) |

## 9. Unresolved assumptions (summary)

See `docs/risks.md` for impact and mitigation. Briefly: HSE frequency only
inferred; **physical layout (OPEN ISSUE L-1, `docs/layout.md`)**; LED part and
placement; bootloader binary variant (not dumped yet) and option-byte/RDP state;
GD32 behaviour under the newer QMK/ChibiOS (intended by policy).
