# Our firmware: features

## 1. Current images (POLICY-VARIANTS)

Built from one tree: QMK 0.34.7 (`180f394b`) + `keyboards/falcon_kalam` (L1)
+ `userspace/` (L2: shared `users/falcon_kalam/` plus one keymap per
variant). Both images are reproducible on any day (`SOURCE_DATE_EPOCH` =
the pinned QMK commit time).

| Variant | Image | SHA-256 | Size (of 53,248 B) | Use |
|---|---|---|---|---|
| **ship** | `artifacts/falcon_kalam_ship.bin` | `c7997042fafa5639099f3a129fbda8fcd718031fdb8bfe24e410749001e738e9` | 36,588 B | the daily image |
| **dev** | `artifacts/falcon_kalam_dev.bin` | `373dee73d643d5407080c00b3dce17182d46df890512a704594087aa4128d024` | 42,092 B | testing and diagnosis only; never the daily image |
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
| Keymap (POLICY-HHKB) | the HHKB Professional Classic's three setting modes, **Mac** (default), Win and HHK, each a base + Fn layer with the real keyboard's HID codes; Fn+Ctrl+M/W/H switch, the mode survives replugging (§3) | same | new (stock: generic 60%) | pending |
| VIA | protocol `0x000D`, 6 layers (the three modes), 16 macros in a 1,107 B buffer (stock: 4 layers, 383 B), opens as HHKB | same + **`VIA_INSECURE`** (VIA key tester and `falcon_kalam_probe.py matrix` work) | + | ✓ (probe) |
| Bootmagic | Esc at plug-in → clear EEPROM → DFU, LEDs painted red | same | = (+ red) | ✓ (red: pending) |
| Fault recovery (POLICY-RECOVERY) | any CPU fault resets into DFU instead of hanging | same + Shift+Shift+F fault test | new | pending |
| Command (LShift+RShift+) | **selective**: H help, D debug, X matrix print, K key reports, V version, S status, B bootloader; every other combination types normally | **full** QMK set (adds Esc/bootloader, Backspace/clear EEPROM, Caps/lock, E, M, C, N, Z, `` ` ``, 0–9, F1–F10) + F fault test | = (ship: safer subset) | pending |
| Console | HID console (`qmk console`) | same + traces: key actions/tapping (`ACTION_DEBUG`), EEPROM backing store (`WEAR_LEVELING_DEBUG_OUTPUT`), matrix scan rate, readable keycode names | = / dev + | connects ✓; output pending |
| Bootloader read (FLASHREAD) | — | read-only dump of bootloader, option bytes, F_SIZE/UID (`falcon_kalam_probe.py dump-bootloader`) | new | ✓ |
| Caps Lock LED | A10, active-low | same | = | pending |
| RGB (POLICY-RGB) | **status indicator**: white at full brightness = normal; red = bootloader, orange blinks = settings reset, magenta = keystroke printout, yellow = debug, blue = powered but not configured by a host, green = layer locked (`policy.md` §2.1); effects compiled out (L1 keeps them); no light toggle key; off while the host sleeps | same + cyan blink while FLASHREAD serves a request | deliberate change (stock: decorative, rainbow swirl) | pending |
| EEPROM | wear-leveling 4 KB at `0x0800F000` (2 KB of settings; stock 2 KB / 1 KB), flash size pinned; a schema stamp resets stale settings once after a layout change | same | + | pending (persistence) |
| Flash safety | the linker refuses to grow into the EEPROM; the bootloader is never written | same | new | ✓ readback |
| Keycode features | layer keys, one-shot, tri-layer, magic, macros, lighting, grave-escape, space-cadet, send-string, mouse/media keys | same | = | — |

## 3. Keymap (ship and dev)

A copy of the HHKB Professional Classic (POLICY-HHKB; reference:
`hardware.md` §7). The mode is stored; an EEPROM reset returns to Mac mode.

```
Base (all modes):  Esc 1 2 3 4 5 6 7 8 9 0 - = \ `
                   Tab  Q W E R T Y U I O P [ ] Delete
                   Ctrl  A S D F G H J K L ; ' Return
                   Shift  Z X C V B N M , . / Shift Fn
                         Alt ◇      Space      ◇ Alt
  Mac: Delete = Backspace, ◇ = Command, Alt = Option
  Win: Delete = Delete,    ◇ = Windows key
  HHK: Delete = Delete,    ◇ = Muhenkan (left) / Henkan (right)

Fn, every mode:  1–= F1–F12  \ Ins  ` Del  I PrtSc  O ScrLk  P Pause
                 [ Up  ; Left  ' Right  / Down  K Home  L PgUp  , End  . PgDn
  Mac adds:  Esc Power  Tab Caps  Delete Clear  Return KP-Enter
             A Vol-  S Vol+  D Mute  F Eject  H KP*  J KP/  N KP+  M KP-
  Win adds:  Tab Caps  Delete Backspace  H KP*  J KP/  N KP+  M KP-
  HHK adds:  Delete Backspace  Right-◇ Stop
Fn+Ctrl+M Mac mode · Fn+Ctrl+W Win mode · Fn+Ctrl+H HHK mode (our addition)
```

Codes as the real keyboard sends them: Power = Keyboard Power (`0x66`), Stop
`0x78`, Clear `0x53`, volume/mute/eject on the consumer page. Keys a mode does
not support send the plain key. Positions outside the HHKB layout are `KC_NO`
and can be assigned in VIA; all six VIA layers belong to the modes.

The L1 reference image (`keymaps/default`, never flashed) has a simpler map:
QMK's own HHKB layout (one Fn layer, as
`qmk_firmware/keyboards/hhkb/ansi`) plus underglow keys on Fn+Z–B.
