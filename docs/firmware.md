# Our firmware: features

## 1. Current release

| | |
|---|---|
| Image | `artifacts/falcon_hs_r2_falcon.bin` (with DFU suffix) |
| SHA-256 | `ed93dff4f7bba3dcb2ceedb620780dd995d524a0658c5976e3995e15f8f04819` |
| Size | 44,196 B of 55,296 B application flash (stock: 43,940 B) |
| Built from | QMK 0.34.7 (`180f394b`) + `keyboards/falcon_hs_r2` (L1) + `userspace/…/falcon` (L2); reproducible, also across days (`SOURCE_DATE_EPOCH` = pinned QMK commit time) |
| Reference image | `artifacts/falcon_hs_r2_default.bin`: L1 only, with NKRO, no VIA; **never flash** |

Which image is on the device and what is still being tested: `docs/devnotes.md`.

## 2. Features

Stock column: **=** same as stock · **+** same function, done better ·
**cfg** supported by L1, disabled by L2 configuration · **new** not in stock.
Hardware column: **✓ date** = verified on the device · **pending** = see devnotes.

| Feature | Our firmware | vs stock | Hardware |
|---|---|---|---|
| Platform | QMK 0.34.7, ChibiOS (pinned), GCC 8.5, LTO | + newer QMK | ✓ 2026-10-26 |
| USB identity | `9DD1:9D89`, bcdDevice `0x0001`, "Falcon" / "Falcon HS R2", 500 mA, remote wakeup | = (product string differs) | ✓ 2026-10-26 |
| HID interfaces | keyboard 6KRO (EP 81), VIA (EP 82/03), shared mouse/system/consumer (EP 84), console (EP 85), all 1 ms | = (no NKRO report) | ✓ 2026-10-26 |
| NKRO | supported in L1, **disabled in the release** (POLICY-NKRO) | cfg (the macOS fix) | Caps Lock and input switching OK (user, 2026-10-26) |
| Matrix | COL2ROW, stock pin tables, 5 ms debounce | = | partly (layout probe on stock; full console test pending) |
| Layout | HHKB classic (`LAYOUT_60_hhkb`); L1 also has `LAYOUT_all` for the PCB variants | + | settled 2026-10-26 |
| Keymap | HHKB base + HHKB Fn layer (§3), 4 VIA layers | + (stock was generic 60%) | pending |
| VIA | protocol `0x000D`, 4 layers, 16 macros (383-byte buffer), opens as HHKB (layout option 0) | + | ✓ 2026-10-26 (probe) |
| Bootmagic | Esc (`0,0`) at plug-in → clear EEPROM → DFU | = | ✓ 2026-10-26 |
| Command | LShift+RShift+key, QMK defaults (B = DFU, D = debug, X = matrix print, K = key reports, Backspace = clear EEPROM, …; N has no effect) | = | pending |
| Console | HID console for `qmk console` | = | connects ✓; output pending |
| Caps Lock LED | A10, active-low | = | pending (missing in the previously flashed image) |
| RGB | rgblight, 4 × WS2812 on A1, GRB, all 43 effects; defaults rainbow swirl, brightness 127, hue 0, sat 255; no cap; off while the host sleeps | = (+ sleep) | pending |
| EEPROM | wear-leveling 2 KB at `0x0800F800`, flash size pinned | + | pending (persistence) |
| Flash safety | linker refuses to grow into the EEPROM; bootloader never written | new | ✓ readback 2026-10-26 |
| Bootloader read | FALCON_READ: read-only dump of the bootloader, option bytes, F_SIZE/UID (`tools/falcon_probe.py dump-bootloader`) | new | ✓ 2026-10-26 |
| Keycode features | layer keys, one-shot, tri-layer, magic, macros, lighting, grave-escape, space-cadet, send-string, mouse/media keys | = | — |

## 3. Keymap (release and L1 default)

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
