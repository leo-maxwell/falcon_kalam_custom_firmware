# Stock firmware: capabilities and feature list

What the manufacturer firmware does, recovered by reverse engineering
(`tools/analyze_vendor_firmware.py`, `tools/feature_parity.py`), plus read-only
probes of the running image on 2026-10-26 (`artifacts/stock_probe_info.txt`).
Hardware facts it revealed are in `docs/hardware.md`.

## 1. Artifacts and identity

| File (`originals/`, read-only, hashes in `SHA256SUMS`) | Content |
|---|---|
| `manufacturer_firmware.bin` (`90123fb1…`) | 43,940 B application + 16 B DFU suffix (`UFD`, target `1EAF:0003`, CRC valid) |
| `falcon_flash_backup.bin` (`4abf550a…`) | DFU upload of `0x08002000–0x0800FFFF`; first 43,940 B = the application; re-read 2026-10-26, identical |
| `manufacturer_jsonfile.json` (`0c9a5b8e…`) | VIA v3 definition: 5×15, `9DD1:9D89`, keycodes `qmk_lighting`, menu `qmk_rgblight`, 3 layout options |

**Build** (QMK Command/console strings at `0x0800B70E`–`0x0800B7E4`): QMK on
ChibiOS `be44b3` / contrib `develop_2021_q3-455-g77cb0a`, GCC 10.1.0, built
2025-03-14 for the QMK board `STM32_F103_STM32DUINO`. No QMK version string is
embedded; keycode numbering means QMK ≥ 0.19. `OPTIONS: BOOTMAGIC MOUSEKEY
EXTRAKEY CONSOLE COMMAND NKRO LTO`.

## 2. Feature list

| Area | Stock behaviour | Evidence |
|---|---|---|
| USB | `9DD1:9D89`, bcdDevice `0x0001`, strings "Falcon"/"Falcon", no serial, 500 mA, remote wakeup | descriptors |
| HID interfaces | 4: boot keyboard (6KRO, EP 81, 8 B); VIA raw HID `0xFF60` (EP 82/03, 32 B); shared EP 84: mouse (ID 2), system (3), consumer (4), **NKRO bitmap (6)**; console `0xFF31` (EP 85). All 1 ms polling | config descriptor |
| NKRO | compiled in and **on at start-up**: NK_OFF fixes the macOS Caps Lock / input-source problem only until the next replug. The exact mechanism is not proven | observed on the device; `OPTIONS` |
| Matrix | COL2ROW scan, 5 ms symmetric-deferred debounce | `docs/hardware.md` §5 |
| Bootmagic | Esc held at plug-in → clear EEPROM → DFU | code `0x08009B40` |
| Command | LShift+RShift+key. Help strings present: debug toggle, matrix debug, keyboard debug, mouse debug, console mode, layer switch 0–9 (F10 = L0), Jump to Bootloader (+ alternate), print EEPROM, Clear EEPROM, NKRO Toggle, version, status, mousekey console. The keys are QMK defaults (D, X, K, M, C, 0–9/F1–F10, B/Esc, E, Backspace, N, V, S); not individually verified | strings `0x0800B487`–`0x0800B9FE` |
| Console | HID console; QMK debug prints (rgblight, eeconfig, keyboard reports) | strings |
| Caps Lock LED | A10, active-low, follows host Caps Lock | code `0x08003E10`, `0x08009BE4` |
| RGB | rgblight, 4 × WS2812 bit-bang on A1, GRB; **all 43 modes** (static, breathing ×4, rainbow mood ×3, swirl ×6, snake ×6, knight ×3, christmas, gradient ×10, RGB test, alternating, twinkle ×6); **no brightness cap**; defaults after EEPROM reset: mode 9 (rainbow swirl), brightness 127, hue 0, sat 255, speed 0; off-on-suspend unknown | `mode_base_table` `0x0800C2B6`; probe |
| VIA | protocol `0x000C`; 4 dynamic layers; 16 macros, 384-byte buffer; layout options default 0 (= 2u-backspace view) | probe |
| Keycode features | handled: layer keys (LM, TO, MO, DF, OSL, OSM), one-shot on/off/toggle, tri-layer, magic keycodes, VIA/dynamic macros, lighting keycodes, `QK_BOOT`, `QK_CLEAR_EEPROM`, Grave-Escape | keycode-constant scan, §4 |
| EEPROM | wear-leveling, 2 KB at the top of flash, located via F_SIZE | `docs/hardware.md` §2 |
| Mouse keys, media/system keys | compiled in (`MOUSEKEY`, `EXTRAKEY`) | `OPTIONS`, HID reports |
| LTO | yes | `OPTIONS` |
| Not determinable statically | `TAPPING_TERM` (read via a function), mousekey parameters, space-cadet (compiled as a jump table) | §4 limits |

**State in the backup:** the EEPROM write log decodes to "erase, then
EECONFIG magic `0xFFFF`", which is the footprint of Bootmagic. So the backup
holds no user configuration. The live probe after boot showed the compiled
defaults.

## 3. Default keymap (compiled, 2 layers at `0x0800C18A`; layers 2–3 transparent)

Generic 60% map, not HHKB:

| Row | Layer 0 | Layer 1 (Fn) |
|---|---|---|
| 0 | `QK_GESC` 1–0 - = `` ` `` Bspc (`0,14`) | `` ` `` F1–F12 |
| 1 | Tab Q–P [ ] `\` (`1,14`) | `QK_BOOT` (Tab), `UG_TOGG` (Q), `UG_NEXT` (W) |
| 2 | `LT(1, Caps)` A–L ; ' Enter | — |
| 3 | LShift, (`3,1` transparent), Z–/ , RShift, `MO(1)` (`3,14`) | — |
| 4 | LCtl (`4,0`), LGui (`4,1`), LAlt (`4,2`), Space, RAlt (`4,11`), `MO(1)` (`4,12`), RCtl (`4,13`) | — |

On layer 1 the Fn keys themselves (`3,14`, `4,12`) are transparent; all other positions are `KC_NO`.

## 4. Method and limits

* **Exact-address checks.** Each fact used by this project is pinned to an
  address and the expected bytes in `analyze_vendor_firmware.py`, so a wrong
  assumption fails loudly.
* **Keycode features** come from scanning the disassembly for QMK keycode
  constants (`QK_*` from the pinned `quantum/keycodes.h`, version 0.0.9) used
  as `cmp`/`sub`/`movw` immediates. The same scan on our release gives the
  same feature profile. Limits: features compiled into jump tables or reached
  through function calls are invisible, so absence proves nothing.
* **Hardware outputs** were swept three ways: peripheral blocks referenced,
  constant pin-mode calls, and every GPIO output-register write.
* **Cross-check with a second agent's notes** (same binary, `90123fb1…`). Their
  RGB findings were all confirmed (PA1, 4 LEDs, buffer `0x20001785`,
  bit-bang, BASEPRI, rgblight strings). Byte order (GRB) and the full effect
  list were resolved here. Their row/col table addresses (`0x0800A464` /
  `0x0800A10C`) are file offsets mislabelled as addresses; the real addresses
  are `0x0800C464` / `0x0800C10C`. The pin values are right.
