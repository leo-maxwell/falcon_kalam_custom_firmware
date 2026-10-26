# Our firmware: features

## 1. Current images (POLICY-VARIANTS)

Built from one tree: QMK 0.34.7 (`180f394b`) + `keyboards/falcon_kalam` (L1)
+ `userspace/` (L2: shared `users/falcon_kalam/` plus one keymap per
variant). Both images are reproducible on any day (`SOURCE_DATE_EPOCH` =
the pinned QMK commit time). **Release 8** (`VIA_FIRMWARE_VERSION`, shown by
`falcon_kalam_probe.py info`; bumped whenever the images change).

| Variant | Image | SHA-256 | Size (of 53,248 B) | Use |
|---|---|---|---|---|
| **ship** | `artifacts/falcon_kalam_ship.bin` | `90d022ac15a3c3dfbedabd1e72cad74d2113af0ea036b7728bbc2c51d41f3790` | 37,980 B | the daily image |
| **dev** | `artifacts/falcon_kalam_dev.bin` | `3a2bb854d5db607911144441839411a961fc4e6f51158cceb57842d23141ccb0` | 43,544 B | testing and diagnosis only; never the daily image |
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
| Matrix, layout | COL2ROW, stock pins, 5 ms debounce, scan rate ≈ 3,677 Hz (dev trace); HHKB classic (`LAYOUT_60_hhkb`) | same | = / + | ✓ all 60 keys (2026-10-27, dev console, `artifacts/dev_session_2026-10-27_keycheck.txt`) |
| Keymap (POLICY-HHKB) | a copy of the HHKB Professional Classic: its six layers (HHK, Mac, Win; base + Fn each) with its HID codes, its mode keys Fn+Ctrl+M/W, and its DIP switches SW1–SW5 as virtual switches (Fn+Ctrl+Shift+1…5); HHK mode from the factory, as the Classic (§3) | same | new (stock: generic 60%) | ✓ all three modes (HHK base/Fn, Mac Fn incl. Eject: macOS answers Control+Eject; Win Fn), mode keys incl. the Classic's Fn+Ctrl+W rule, SW1–SW5 (dev, 2026-10-27) |
| Settings display (additions) | Fn+Ctrl+Shift+0 and the mode keys show the mode on LED 1 (HHK red, Mac blue, Win green) and SW3–SW5 on LEDs 2–4 (blue = on), first as in effect, then as after the next plug-in if different (§3) | same | new | ✓ both phases with a dark gap (dev release 6) |
| Release number | `VIA_FIRMWARE_VERSION`, readable over VIA | same | new | ✓ (probe) |
| VIA | protocol `0x000D`, 6 layers (the three modes, in the Classic's order), 16 macros in a 1,107 B buffer (stock: 4 layers, 383 B), opens as HHKB | same + **`VIA_INSECURE`** (VIA key tester and `falcon_kalam_probe.py matrix` work) | + | ✓ (probe) |
| Bootmagic | Esc at plug-in → clear EEPROM → DFU, LEDs painted red | same | = (+ red) | ✓ all four LEDs red in DFU (dev release 6) |
| Fault recovery (POLICY-RECOVERY) | any CPU fault resets into DFU instead of hanging | same + Shift+Shift+F fault test | new | ✓ Shift+Shift+F drops into DFU; the console line arrives first (dev release 7) |
| Command (LShift+RShift+) | **selective**: H help, D debug, X matrix print, K key reports, V version, S status, B bootloader; every other combination types normally | **full** QMK set (adds Esc/bootloader, Backspace/clear EEPROM, Caps/lock, E, M, C, N, Z, `` ` ``, 0–9, F1–F10) + F fault test | = (ship: safer subset) | pending |
| Console | HID console (`qmk console`) | same + traces: key actions/tapping (`ACTION_DEBUG`), EEPROM backing store (`WEAR_LEVELING_DEBUG_OUTPUT`), matrix scan rate, readable keycode names | = / dev + | connects ✓; output pending |
| Bootloader read (FLASHREAD) | — | read-only dump of bootloader, option bytes, F_SIZE/UID (`falcon_kalam_probe.py dump-bootloader`) | new | ✓ |
| Caps Lock LED | A10, active-low | same | = | ✓ follows the host's Caps Lock. On macOS a short Caps Lock press (Fn+Tab) switches the input source and a long press toggles Caps Lock, as intended (the macOS setting this firmware was built to keep working) |
| RGB (POLICY-RGB) | **status indicator in pure red, green and blue only**, bright (cap 63, a quarter of full scale) or dim (a quarter of the cap): all green bright = normal; all red = bootloader; red ×3 on dark = settings reset; channel A red = keystroke printout, blue = debug; channel B dim green = not configured by a host, blue = layer locked; off-blinks = virtual DIP switch flipped; settings display (`policy.md` §2.1); effects compiled out (L1 keeps them); no light toggle key; off while the host sleeps | same + blue blink while FLASHREAD serves a request | deliberate change (stock: decorative, rainbow swirl) | ✓ green base, channel A blue/red, channel B blue (layer lock), red in DFU (both routes), red ×3 on dark (release 7), off while the Mac sleeps; pending: dark from boot on a reset boot (release 8), dim green before enumeration (too brief to judge so far) |
| EEPROM | wear-leveling 4 KB at `0x0800F000` (2 KB of settings; stock 2 KB / 1 KB), flash size pinned; a schema stamp resets stale settings once after a layout change | same | + | ✓ switch flips persist across replugs; long-term wear-leveling: R2 |
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
Virtual DIP switches (saved):        Fn+Ctrl+Shift+1…5 flips SW1…SW5; off-blink ×1 = off, ×2 = on;
                                     effective from the next plug-in, as on the Classic
Settings display:                    Fn+Ctrl+Shift+0 (and every mode key) → 1.5 s now, then 1.5 s
                                     after-replug (if different); LED 1 = mode: HHK red · Mac blue ·
                                     Win green; LEDs 2–4 = SW3–SW5 (blue on, off off)
  SW1/SW2  power-on mode: OFF/OFF HHK (factory) · ON/OFF Win · OFF/ON Mac · ON/ON acts as HHK
  SW3      Delete sends Backspace (Win/HHK)
  SW4      left ◇ = Fn
  SW5      swap Alt and ◇
```

Codes as the Classic sends them: Power = Keyboard Power (`0x66`), Stop
`0x78`, Clear `0x53`, volume/mute and Eject (`0xB8`) on the consumer page.
QMK's `KC_EJCT` would send `0xCC` (Stop/Eject), so `hhkb_modes.c` sends
`0xB8` for it. Keys a mode does
not support send the plain key. The switches act on top of the keymap, so VIA
shows the unchanged layers. Positions outside the HHKB layout are `KC_NO` and
can be assigned in VIA; all six VIA layers belong to the modes. An EEPROM
reset restores the factory switches (all off: HHK mode). On a Mac, HHK mode
has no Command key (◇ = Muhenkan/Henkan): switch to Mac mode with Fn+Ctrl+M
until the next plug-in, or with SW2 (Fn+Ctrl+Shift+2, then replug) for good.

The L1 reference image (`keymaps/default`, never flashed) has a simpler map:
QMK's own HHKB layout (one Fn layer, as
`qmk_firmware/keyboards/hhkb/ansi`) plus underglow keys on Fn+Z–B.
