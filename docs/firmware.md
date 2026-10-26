# Our firmware: features

## 1. Current images (POLICY-VARIANTS)

Built from one tree: QMK 0.34.7 (`180f394b`) + `keyboards/falcon_kalam` (L1)
+ `userspace/` (L2: shared `users/falcon_kalam/` plus one keymap per
variant). Both images are reproducible on any day (`SOURCE_DATE_EPOCH` =
the pinned QMK commit time). **Release 3** (`VIA_FIRMWARE_VERSION`, shown by
`falcon_kalam_probe.py info`; bumped whenever the images change).

| Variant | Image | SHA-256 | Size (of 53,248 B) | Use |
|---|---|---|---|---|
| **ship** | `artifacts/falcon_kalam_ship.bin` | `1be146c6aff3967db0cb47313ed304be23117c6fa7c31c63dde1f270a25a9f2b` | 37,812 B | the daily image |
| **dev** | `artifacts/falcon_kalam_dev.bin` | `a89f6e7adab2f5868ee0823e9e028df2addca3b92853c7b49ecde49e77f1ebe3` | 43,292 B | testing and diagnosis only; never the daily image |
| (base) | `artifacts/falcon_kalam_default.bin` | — | — | L1 reference with NKRO, no VIA; **never flash** |

Stock image: 43,940 B. What is on the device and what is being tested:
`docs/devnotes.md`.

## 2. Features

vs stock: **=** same · **+** same function, done better · **cfg** supported
by L1, disabled by L2 · **new** not in stock.
Hardware: **✓** = verified on the device on 2026-10-26, on an image with the
same feature code · **pending** = see devnotes.

| Feature | Ship | Dev | vs stock | Hardware |
|---|---|---|---|---|
| Platform | QMK 0.34.7, ChibiOS (pinned), GCC 8.5, LTO | same | + newer QMK | ✓ |
| USB identity | `9DD1:9D89`, bcdDevice `0x0001`, "Falcon" / "Falcon" (POLICY-NAMING), 500 mA, remote wakeup | same | = | ✓ |
| HID interfaces | keyboard 6KRO, VIA, shared mouse/system/consumer, console; 1 ms | same | = (no NKRO report) | ✓ |
| NKRO | off (POLICY-NKRO) | off | cfg (the macOS fix) | ✓ Caps Lock and input-source switching work on macOS, also after a replug |
| Matrix, layout | COL2ROW, stock pins, 5 ms debounce; HHKB classic (`LAYOUT_60_hhkb`) | same | = / + | pending: full key test |
| Keymap (POLICY-HHKB) | a copy of the HHKB Professional Classic: its six layers (HHK, Mac, Win; base + Fn each) with its HID codes, its mode keys Fn+Ctrl+M/W, and its DIP switches SW1–SW5 as virtual switches (Fn+Ctrl+Shift+1…5); HHK mode from the factory, as the Classic (§3) | same | new (stock: generic 60%) | pending |
| Settings display (additions) | a mode key flashes the mode colour (HHK purple, Mac pink, Win gold); Fn+Ctrl+Shift+0 shows the mode colour on the first LED and SW3–SW5 on the other three (green = on), first as in effect, then as after the next plug-in if different (§3) | same | new | pending |
| Release number | `VIA_FIRMWARE_VERSION`, readable over VIA | same | new | pending |
| VIA | protocol `0x000D`, 6 layers (the three modes, in the Classic's order), 16 macros in a 1,107 B buffer (stock: 4 layers, 383 B), opens as HHKB | same + **`VIA_INSECURE`** (VIA key tester and `falcon_kalam_probe.py matrix` work) | + | ✓ (probe) |
| Bootmagic | Esc at plug-in → clear EEPROM → DFU, LEDs painted red | same | = (+ red) | ✓ (red: pending) |
| Fault recovery (POLICY-RECOVERY) | any CPU fault resets into DFU instead of hanging | same + Shift+Shift+F fault test | new | pending |
| Command (LShift+RShift+) | **selective**: H help, D debug, X matrix print, K key reports, V version, S status, B bootloader; every other combination types normally | **full** QMK set (adds Esc/bootloader, Backspace/clear EEPROM, Caps/lock, E, M, C, N, Z, `` ` ``, 0–9, F1–F10) + F fault test | = (ship: safer subset) | pending |
| Console | HID console (`qmk console`) | same + traces: key actions/tapping (`ACTION_DEBUG`), EEPROM backing store (`WEAR_LEVELING_DEBUG_OUTPUT`), matrix scan rate, readable keycode names | = / dev + | connects ✓; output pending |
| Bootloader read (FLASHREAD) | — | read-only dump of bootloader, option bytes, F_SIZE/UID (`falcon_kalam_probe.py dump-bootloader`) | new | ✓ |
| Caps Lock LED | A10, active-low | same | = | pending |
| RGB (POLICY-RGB) | **status indicator**: white at the brightness cap (63, a quarter of full scale) = normal; every colour is capped, the bootloader red included; red = bootloader, orange blinks = settings reset, magenta = keystroke printout, yellow = debug, blue = powered but not configured by a host, green = layer locked, dark blinks = virtual DIP switch flipped, mode colours = mode flash and settings display (`policy.md` §2.1); effects compiled out (L1 keeps them); no light toggle key; off while the host sleeps | same + cyan blink while FLASHREAD serves a request | deliberate change (stock: decorative, rainbow swirl) | pending |
| EEPROM | wear-leveling 4 KB at `0x0800F000` (2 KB of settings; stock 2 KB / 1 KB), flash size pinned; a schema stamp resets stale settings once after a layout change | same | + | pending (persistence) |
| Flash safety | the linker refuses to grow into the EEPROM; the bootloader is never written | same | new | ✓ readback |
| Keycode features | layer keys, one-shot, tri-layer, magic, macros, lighting, grave-escape, space-cadet, send-string, mouse/media keys | same | = | — |

## 3. Keymap (ship and dev)

A copy of the HHKB Professional Classic (POLICY-HHKB; reference:
`hardware.md` §7). Layers in the Classic's order: 0/1 HHK, 2/3 Mac, 4/5 Win.

```
Base (all modes):  Esc 1 2 3 4 5 6 7 8 9 0 - = \ `
                   Tab  Q W E R T Y U I O P [ ] Delete
                   Ctrl  A S D F G H J K L ; ' Return
                   Shift  Z X C V B N M , . / Shift Fn
                         Alt ◇      Space      ◇ Alt
  HHK: Delete = Delete,    ◇ = Muhenkan (left) / Henkan (right)
  Mac: Delete = Backspace, ◇ = Command, Alt = Option
  Win: Delete = Delete,    ◇ = Windows key

Fn, every mode:  1–= F1–F12  \ Ins  ` Del  I PrtSc  O ScrLk  P Pause
                 [ Up  ; Left  ' Right  / Down  K Home  L PgUp  , End  . PgDn
  HHK adds:  Delete Backspace  Right-◇ Stop
  Mac adds:  Esc Power  Tab Caps  Delete Clear  Return KP-Enter
             A Vol-  S Vol+  D Mute  F Eject  H KP*  J KP/  N KP+  M KP-
  Win adds:  Tab Caps  Delete Backspace  H KP*  J KP/  N KP+  M KP-

Mode keys (until the next plug-in):  Fn+Ctrl+M Mac · Fn+Ctrl+W Win (HHK when SW1/SW2 select HHK)
Virtual DIP switches (saved):        Fn+Ctrl+Shift+1…5 flips SW1…SW5; dark blink ×1 = off, ×2 = on;
                                     effective from the next plug-in, as on the Classic
Settings display:                    Fn+Ctrl+Shift+0 → 1.5 s now, then 1.5 s after-replug (if different);
                                     LED 1 = mode colour, LEDs 2–4 = SW3–SW5 (green on, dark off)
Mode colours:                        HHK purple · Mac pink · Win gold (also flashed by the mode keys)
  SW1/SW2  power-on mode: OFF/OFF HHK (factory) · ON/OFF Win · OFF/ON Mac · ON/ON acts as HHK
  SW3      Delete sends Backspace (Win/HHK)
  SW4      left ◇ = Fn
  SW5      swap Alt and ◇
```

Codes as the Classic sends them: Power = Keyboard Power (`0x66`), Stop
`0x78`, Clear `0x53`, volume/mute/eject on the consumer page. Keys a mode does
not support send the plain key. The switches act on top of the keymap, so VIA
shows the unchanged layers. Positions outside the HHKB layout are `KC_NO` and
can be assigned in VIA; all six VIA layers belong to the modes. An EEPROM
reset restores the factory switches (all off: HHK mode). On a Mac, HHK mode
has no Command key (◇ = Muhenkan/Henkan): switch to Mac mode with Fn+Ctrl+M
until the next plug-in, or with SW2 (Fn+Ctrl+Shift+2, then replug) for good.

The L1 reference image (`keymaps/default`, never flashed) has a simpler map:
QMK's own HHKB layout (one Fn layer, as
`qmk_firmware/keyboards/hhkb/ansi`) plus underglow keys on Fn+Z–B.
