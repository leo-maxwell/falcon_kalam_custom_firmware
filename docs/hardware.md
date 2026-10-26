# Hardware facts and conventions

Facts about the physical falcon_kalam unit, as recovered from the stock image
and the device. Every code-derived fact is re-checked on each build by
`tools/analyze_vendor_firmware.py` (report: `artifacts/vendor_analysis.txt`).
Addresses are flash addresses in the stock application (link base `0x08002000`).

Status: **Verified** = read from code/data or the device, consistent with
independent evidence · **Inferred** = calculated from verified values ·
**Unverified** = not established.

## 0. Provenance

* Reported by the gifter (2026-10-26, not independently verified): a **clone** of
  MatrixLAB's Falcon, made by the 迦蓝 team ("kalam" in this project's dev names, POLICY-NAMING). Stock strings say "Falcon";
  the VIA JSON says "Falcon_HS".
* It is **not** compatible with the genuine MatrixLAB Falcon in QMK
  (`qmk_firmware/keyboards/matrix/falcon`): that one is an AVR ATmega32U4 with
  the Atmel DFU bootloader and USB `4D58:474E`. This unit is a GD32F103 (ARM)
  with the STM32duino bootloader and USB `9DD1:9D89`. Never flash `matrix/falcon`
  firmware onto it.

## 1. Conventions

* **Matrix position** `r,c` = row index (0–4, pins `B3 B4 B5 B6 A2`), column index
  (0–14, pins `A9 A3 A4 A5 A6 A7 B0 B1 B10 B11 B12 B13 B14 B15 A8`).
* **Pins** are written QMK style: `A10` = PA10.
* Key names follow the HHKB legend; ◇ = GUI/Command (except in HHK mode, §7).

## 2. MCU and memory

| Item | Value | Status / evidence |
|---|---|---|
| MCU | GigaDevice GD32F103C8T6, Cortex-M3, 72 MHz | given; clock Verified (§4) |
| Flash | 64 KB, 1 KB pages | Verified: F_SIZE register reads 64 (dump 2026-10-26) |
| SRAM | 20 KB | datasheet; not measured |
| Read-out protection | **off** (`RDP = 0xA5`), no write protection, USER `0xFF` | Verified: option-byte dump |
| Debug | SWD on PA13/PA14 enabled, JTAG off (`AFIO_MAPR.SWJ_CFG=010`, which frees PB3/PB4 for rows) | Verified (`0x080094AA`); pads not located |

```
0x08000000 ┌──────────────────────────┐
           │ STM32duino bootloader    │  8 KB  (6,960 B used)   never written by this project
0x08002000 ├──────────────────────────┤
           │ application              │ 52 KB  the linker refuses to exceed it
0x0800F000 ├──────────────────────────┤
           │ wear-leveling EEPROM     │  4 KB  2 KB base image + 2 KB write log
0x08010000 └──────────────────────────┘
```

EEPROM: QMK wear-leveling on ChibiOS embedded flash. Writes are 16-bit and the
flash stores `~value` (erased = 1). **Stock** uses the top 2 KB
(`0x0800F800`, 1 KB of settings; Verified, readback), located from F_SIZE.
**Ours** uses the top 4 KB (`0x0800F000`, 2 KB of settings), with the flash
size pinned to 64 KB. The larger area is a project choice (POLICY-HHKB needs
six keymap layers); `0x0800F000–0x0800F7FF` is erased in the stock readback,
so nothing of stock's is overwritten.

## 3. Bootloader

| Item | Value |
|---|---|
| Identity | Roger Clark's **STM32duino-bootloader**, byte-identical to upstream `generic-none_bootloader.bin` (no LED, no button) at the pinned commit `df689808` (`bootloader/STM32duino-bootloader/`). **MIT licence** (LeafLabs). Dump: `originals/device_dump_2026-10-26/bootloader_0x08000000.bin` |
| USB | `1EAF:0003`, bcdDevice `0x0201`, "LeafLabs" / "Maple 003" / serial "LLM 003"; DFU 1.1, 1024-byte transfers, 100 mA |
| DFU alts | **2** = flash at `0x08002000` (ours) · 1 = flash at `0x08005000`: **a download to alt 1 erases `0x08002000`, never use it** · 0 = RAM, unsupported |
| Upload limit | uploads end at F_SIZE × 1 KB (`0x08010000`). The bootloader cannot read itself, the option bytes or the UID over DFU |
| Power-on DFU window | **none**: a watched replug shows the app directly (`artifacts/usb_watch_replug.txt`) |
| Flags (`BKP_DR10`) | `0x424C` = stay in DFU; the app writes it at every boot, so any reset into the bootloader stays there. `0x424D` = just uploaded, start the app. Observed: after a download the app starts by itself |

**Entering DFU** (LEDs dark = DFU):

| Method | Works on |
|---|---|
| Hold **Esc** (`0,0`) while plugging in (Bootmagic; also clears EEPROM) | stock (verified in code at `0x08009B40`) and ours (verified on the device; ours also paints the LEDs red) |
| LShift+RShift+B (Command) | stock and ours (ship and dev) |
| Fn+Tab (`QK_BOOT`) | stock default keymap only |
| VIA host command | nowhere: QMK removed it in 2021 |
| A CPU fault (POLICY-RECOVERY) | ours (ship and dev); dev can trigger one on purpose with Shift+Shift+F |
| Shorting the reset/boot pads, or SWD | always; last resort |

## 4. Clock and USB

`RCC_CFGR = 0x001D6400`, HSE on, PLL ×9, `FLASH_ACR = 0x12` → SYSCLK 72 MHz,
APB1/APB2 ÷2, ADC ÷4, USB = 72 ÷ 1.5 = 48 MHz (Verified, `0x080090AC`).
**HSE = 8 MHz** is Inferred: two independent 72 MHz timing constants (an
18-NOP input delay and a 2,160-cycle = 30 µs DWT wait) combined with PLL ×9.
Working USB confirms it. USB D+ re-enumeration: PA12 is driven low, then
released (`board.h`).

## 5. Matrix and physical layout

* **`COL2ROW`**: rows are driven low one at a time; columns are pulled-up,
  active-low inputs (Verified, scan loop `0x08006100`). Debounce: 5 ms,
  symmetric deferred (Inferred).
* **Physical layout: HHKB classic, 60 keys.** Settled by the user on
  2026-10-26. The read-only probe confirmed 53 of 60 positions
  (`artifacts/layout_probe_decoded.txt`); the other 7 were invisible to a
  since-fixed probe decoder.

```
row 0  Esc 0,0 │ 1 0,1 │ 2 0,2 … 9 0,9 │ 0 0,10 │ - 0,11 │ = 0,12 │ \ 0,13 │ ` 0,14
row 1  Tab 1,0 │ Q 1,1 … P 1,10 │ [ 1,11 │ ] 1,12 │ Bspc 1,14
row 2  Ctrl 2,0 │ A 2,1 … L 2,9 │ ; 2,10 │ ' 2,11 │ Enter 2,14
row 3  Shift 3,0 │ Z 3,2 … M 3,8 │ , 3,9 │ . 3,10 │ / 3,11 │ Shift 3,12 │ Fn 3,14
row 4        Alt 4,1 │ ◇ 4,2 │ Space 4,7 │ ◇ 4,11 │ Alt 4,12
```

The VIA JSON names PCB variants that are not fitted on this unit: a 2u
backspace on `0,14`; split left shift `3,0`+`3,1`; split space
`4,5`/`4,7`/`4,9`. The stock keymap also maps `4,0`, `4,13` and `4,14`; these
positions never fired on the probe.

## 6. Lights

There are exactly two:

| Light | Pin / drive | Details |
|---|---|---|
| **Caps Lock LED** | `A10`, push-pull, **active-low** | on whenever the host's Caps Lock is on (`led_set` at `0x08003E10`); off (high) at init |
| **RGB module** | `A1`, bit-banged WS2812, **GRB**, 4 LEDs, 280 µs reset | a separate, **detachable module** on a connector; dark in DFU after Bootmagic (never initialised). Index order and placement (underglow vs. indicator), LED part, current draw: Unverified. Use: `docs/policy.md` POLICY-RGB |

How "nothing else" was established: in the stock image, the only constant pin
setups and GPIO writes are A1, A10, A12 (USB) and the `board.h` defaults. No
timer, SPI, I2C, ADC, UART, DMA, or GPIO port C–E is referenced.

## 7. HHKB Professional Classic reference

The convention our keymap copies (POLICY-HHKB). It describes PFU's keyboard,
not this unit; this unit only shares its physical layout (§5). Sources: PFU's
*HHKB Professional Classic User's Guide* (P3PC-6661-01EN), and tmk's dumps
of a real Classic (USB `04FE:0020`, STM32L072): its USB descriptors and its
EEPROM keymap (gists linked from geekhack topic 106213).

**Setting modes.** DIP SW1/SW2: OFF/OFF = **HHK mode** (factory default),
ON/OFF = **Win mode**, OFF/ON = **Mac mode**. Fn+Control+W switches to Win
mode (to HHK mode when the DIPs select HHK), Fn+Control+M to Mac mode; the
keys are pressed in that order.

| Base layer | HHK | Win | Mac |
|---|---|---|---|
| Delete key | Delete `0x4C` (SW3 off, factory) / Backspace (SW3 on) | same | Backspace (SW3 ignored) |
| ◇ keys | left Muhenkan `0x8B`, right Henkan `0x8A` | Windows key | Command |
| Alt keys | Alt | Alt | Option |

| Fn + | Function | HID code | HHK | Win | Mac |
|---|---|---|---|---|---|
| 1 … = | F1–F12 | keyboard page | ✓ | ✓ | ✓ |
| \ · \` | Insert · Delete | `0x49` · `0x4C` | ✓ | ✓ | ✓ |
| I · O · P | PrtSc · ScrLk · Pause | `0x46` · `0x47` · `0x48` | ✓ | ✓ | ✓ |
| K · L · , · . | Home · PgUp · End · PgDn | `0x4A` `0x4B` `0x4D` `0x4E` | ✓ | ✓ | ✓ |
| [ · / · ; · ' | ↑ ↓ ← → | `0x52` `0x51` `0x50` `0x4F` | ✓ | ✓ | ✓ |
| Delete | Backspace / Clear (keypad) | `0x2A` / `0x53` | BS | BS | Clear |
| Tab | Caps Lock | `0x39` | — | ✓ (some drivers need Shift) | ✓ |
| Return | Enter (keypad) | `0x58` | — | — | ✓ |
| H · J · N · M | keypad * / + − | `0x55` `0x54` `0x57` `0x56` | — | ✓ | ✓ |
| A · S · D | Vol− · Vol+ · Mute | consumer `0xEA` `0xE9` `0xE2` | — | — | ✓ |
| F | Eject | consumer `0xB8` | — | — | ✓ |
| Esc | Power (cannot switch a computer on) | keyboard `0x66` | — | — | ✓ |
| Right ◇ | Stop | keyboard `0x78` | ✓ | — | — |

An unsupported combination sends the plain key (the HHK-mode Fn dump holds
`0x2B` Tab, `0x29` Esc, `0x04` A, … there).

| Item | Status |
|---|---|
| HHK-mode base and Fn bytes (Stop `0x78`, Delete/Backspace, ◇ = `0x8B`/`0x8A`, plain keys where unsupported) | Verified: EEPROM keymap dump |
| Consumer codes | Verified: the Mac-mode descriptor lists exactly Play/Pause, Stop, Previous, Next, Mute, Vol−, Vol+, Eject; the Win-mode one omits Eject |
| Power = Keyboard Power `0x66` | Inferred: the Classic has only keyboard, consumer and vendor reports (no system-control collection), so `0x66` is the only HID code it can send |
| Clear = `0x53` (Keypad Num Lock and Clear) | Inferred: the manual says "Clear (keypad)"; community notes describe Fn+Delete as Num Lock in Mac mode |
| Win and Mac base/Fn bytes | from the manual only (not dumped) |
| Whether a key-selected mode survives a power cycle | Unverified |
| Other DIP switches | SW4 = left ◇ becomes Fn; SW5 = swap Alt and ◇; SW6 = power saving (wake-up by key) |

The Classic's USB interfaces: boot keyboard (6KRO); NKRO bitmap plus consumer
reports; a vendor interface for its keymap tool.
