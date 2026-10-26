# Our firmware: features

## 1. Current images (POLICY-VARIANTS)

Both are built from the same tree: QMK 0.34.7 (`180f394b`) + `keyboards/falcon_hs_r2`
(L1) + `userspace/` (L2: shared `users/falcon/` plus one keymap per variant).
Both are reproducible, also across days (`SOURCE_DATE_EPOCH` = the pinned QMK
commit time).

| Variant | Image | SHA-256 | Size (of 55,296 B) | Use |
|---|---|---|---|---|
| **ship** | `artifacts/falcon_hs_r2_falcon.bin` | `8682681bcc54c9d8a53b230b6a5b1a2eedc81c0efe9261b858a1fc9ac3f9209a` | 43,968 B | the daily image |
| **dev** | `artifacts/falcon_hs_r2_falcon_dev.bin` | `08a97bb9b52cd0c8cbdb9f2b56ae4ca0ce23dd2cc7371c493064a88098b2a583` | 49,400 B | testing and diagnosis only; never the daily image |
| (base) | `artifacts/falcon_hs_r2_default.bin` | — | — | L1 reference with NKRO, no VIA; **never flash** |

Stock image: 43,940 B. What is on the device and what is being tested:
`docs/devnotes.md`.

## 2. Features

vs stock: **=** same · **+** same function, done better · **cfg** supported by
L1, disabled by L2 · **new** not in stock.
Hardware: **✓ date** = verified on the device (on any image with the same
feature code) · **pending** = see devnotes.

| Feature | Ship | Dev | vs stock | Hardware |
|---|---|---|---|---|
| Platform | QMK 0.34.7, ChibiOS (pinned), GCC 8.5, LTO | same | + newer QMK | ✓ 2026-10-26 |
| USB identity | `9DD1:9D89`, bcdDevice `0x0001`, "Falcon" / "Falcon HS R2", 500 mA, remote wakeup | same | = (product string differs) | ✓ 2026-10-26 |
| HID interfaces | keyboard 6KRO, VIA, shared mouse/system/consumer, console; 1 ms | same | = (no NKRO report) | ✓ 2026-10-26 |
| NKRO | off (POLICY-NKRO) | off | cfg (the macOS fix) | Caps Lock and input switching OK (user, 2026-10-26) |
| Matrix, layout | COL2ROW, stock pins, 5 ms debounce; HHKB classic (`LAYOUT_60_hhkb`) | same | = / + | layout settled 2026-10-26; full key test pending |
| Keymap | HHKB base + HHKB Fn (§3), 4 VIA layers | same | + (stock: generic 60%) | pending |
| VIA | protocol `0x000D`, 4 layers, 16 macros (383-byte buffer), opens as HHKB | same + **`VIA_INSECURE`** (VIA key tester and `falcon_probe.py matrix` work) | + | ✓ 2026-10-26 (probe) |
| Bootmagic | Esc at plug-in → clear EEPROM → DFU | same | = | ✓ 2026-10-26 |
| Command (LShift+RShift+) | **selective**: H help, D debug, X matrix print, K key reports, V version, S status, B bootloader. All other combos type normally | **full** QMK set (adds Esc/bootloader, Backspace/clear EEPROM, Caps/lock, E, M, C, N, Z, `` ` ``, 0–9, F1–F10) | = (ship: safer subset) | pending |
| Console | HID console (`qmk console`) | same + traces: key actions/tapping (`ACTION_DEBUG`), EEPROM backing store (`WEAR_LEVELING_DEBUG_OUTPUT`), matrix scan rate, readable keycode names | = / dev + | connects ✓; output pending |
| Bootloader read (FLASHREAD) | — | read-only dump of bootloader, option bytes, F_SIZE/UID (`falcon_probe.py dump-bootloader`) | new | ✓ 2026-10-26 |
| Caps Lock LED | A10, active-low | same | = | pending |
| RGB | rgblight, 4 × WS2812 on A1, GRB, all 43 effects; rainbow swirl, brightness 127; no cap; off while the host sleeps | same | = (+ sleep) | pending |
| EEPROM | wear-leveling 2 KB at `0x0800F800`, flash size pinned | same | + | pending (persistence) |
| Flash safety | the linker refuses to grow into the EEPROM; the bootloader is never written | same | new | ✓ readback 2026-10-26 |
| Keycode features | layer keys, one-shot, tri-layer, magic, macros, lighting, grave-escape, space-cadet, send-string, mouse/media keys | same | = | — |

## 3. Keymap (ship, dev and the L1 default)

```
Base:  Esc 1 2 3 4 5 6 7 8 9 0 - = \ `
       Tab  Q W E R T Y U I O P [ ] Bspc
       Ctrl  A S D F G H J K L ; ' Enter
       Shift  Z X C V B N M , . / Shift Fn
             Alt ◆      Space      ◆ Alt
Fn:    Esc=Power  1–=:F1–F12  \=Ins  `=Del
       Tab=Caps  I=PrtSc O=ScrLk P=Pause [=Up
       A=Vol- S=Vol+ D=Mute  H=KP*  J=KP/  K=Home  L=PgUp  ;=Left  '=Right  Enter=KP Enter
       Z=RGB toggle X=RGB mode C=Hue+ V=Sat+ B=Bright+  N=KP+  M=KP-  ,=End  .=PgDn  /=Down
```

Layers 2–3 are transparent. Positions outside the HHKB layout are `KC_NO` and
can be assigned in VIA. Fn+Esc sends Power, which opens the macOS shut-down
dialog.
