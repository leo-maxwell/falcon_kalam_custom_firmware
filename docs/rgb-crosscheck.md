# RGB: independent cross-check of the second agent's notes

Input: the second agent's "static firmware reverse-engineering notes"
(2026-10-26), pasted by the user. Their input file `Falcon_HS.bin` has SHA-256
`90123fb1…2cb60b`, the same as `originals/manufacturer_firmware.bin`. Every item
below was re-checked here directly against the binary
(`tools/analyze_vendor_firmware.py` §7–8b), not taken from their text.

| Their claim | Verdict | Our evidence |
|---|---|---|
| rgblight, not RGB Matrix; strings `rgblight_init`, `rgblight_config EEPROM`, … | **Confirmed** | strings at `0x0800B116`–`0x0800BCC0`; VIA menu `qmk_rgblight`. (My first string scan was truncated and missed them; corrected.) |
| 4 LEDs; buffer `0x20001785`; `0x08004776` sets `r1=4` and branches to `0x08004B60` | **Confirmed** | identical addresses and values |
| `0x08004758` "initialises" four 3-byte buffers | **Confirmed, renamed** | it is `rgblight_set()`'s "disabled → write zeros" path, not init |
| Output loop `0x08004B60`, BASEPRI raised, bit writer `0x08004AE8` | **Confirmed** | identical |
| PA1 via GPIOA `BSRR`/`BRR` mask 2 at `0x08004AEE/AF0/AF8/B0E` | **Confirmed** | identical; also `ws2812_init` sets PA1 to output push-pull (`0x080056C8`) |
| Bit-banged WS2812/SK6812-style one-wire | **Confirmed** | QMK bitbang driver; reset latch 280 µs = QMK default |
| "Not enough evidence to confirm RGB/GRB order" | **Resolved: GRB** | `rgblight_setrgb` (`0x080049E4`) stores r→[1], g→[0], b→[2], so `rgb_led_t = {g,r,b}` = QMK `WS2812_BYTE_ORDER_GRB`, sent in memory order. The physical LED part is still unidentified; GRB is what the vendor firmware drives. |
| "Not enough evidence to recover every effect" | **Resolved: all effects** | `mode_base_table` at `0x0800C2B6` lists all 43 rgblight modes (incl. RGB test, alternating, twinkle). The base keyboard now enables all of them. |
| Timing under a new clock config not guaranteed | **Agree, narrowed** | our clock tree is identical (72 MHz; same `RCC_CFGR` literal found in our image), and the driver and defaults are the same. Residual risk: compiler/HAL code-generation differences (R6). |
| Row/col tables at `0x0800A464` / `0x0800A10C` | **Incorrect addresses** | those are file offsets plus `0x08000000`. The literals the scan routine loads (`0x08006228`, `0x08006230`) are `0x0800C464` / `0x0800C10C` (file offsets `0xA464`/`0xA10C`). The pin values themselves are correct. |
| Budget 57,344 B (56 KiB) | **Correct, before reservation** | minus the 2 KiB wear-leveling area = 55,296 B, enforced by our linker script |
| NK_OFF fixes Caps Lock until replug | **Consistent** | the vendor has NKRO + Command ("NKRO Toggle") compiled in. Reverting on replug matches NKRO being forced on at start-up; the exact mechanism is not proven. Our release has no NKRO code at all. |
| "Preserve original dynamic-keymap mapping and four layers" | **Not possible from the backup** | the backup's EEPROM is the result of `eeconfig_disable()` (Bootmagic); no user mapping survives. The vendor's *compiled* keymap (2 layers) is documented; the release uses HHKB as the user requires. `tools/falcon_probe.py dump-keymap` can capture the live layers before flashing. |

Still open on hardware: the LED part, its physical position (underglow vs.
indicator), and a colour test for R/G/B order (checklist §C6).
