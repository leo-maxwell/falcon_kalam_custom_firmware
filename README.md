# Falcon HS R2 — independent QMK firmware

A reproducible, independently maintained QMK firmware for the Falcon HS R2
(GD32F103C8T6, STM32duino/Maple DFU bootloader, USB `9DD1:9D89`). It keeps
the HHKB ANSI layout and VIA configurability, and permanently disables NKRO to fix
the macOS Caps Lock / input-source problem.

> **Status: built and verified offline. Not flashed.** Do not flash until a
> separate review has approved the first hardware test:
> [`docs/preflash-checklist.md`](docs/preflash-checklist.md).

## Layout

```
originals/           immutable vendor artifacts + SHA256SUMS (read-only)
qmk_firmware/        upstream QMK, git submodule pinned to 180f394b (0.34.7)
keyboards/falcon_hs_r2/
  keyboard.json      board definition (pins, USB, EEPROM, RGB, layouts)
  config.h           NKRO guard, pinned flash size for wear-leveling
  rules.mk           selects the keyboard-local linker script
  ld/…falcon_hs_r2.ld  app region 0x08002000–0x0800F7FF (excludes bootloader + EEPROM)
  keymaps/default/   native HHKB, LAYOUT_60_hhkb
  keymaps/hhkb_via/  release keymap: same layers on LAYOUT_all + VIA
via/falcon_hs_r2.json  VIA definition (vendor JSON, renamed)
tools/               vendor-firmware analysis (re-derives every hardware fact)
scripts/             build.sh, verify_firmware.py, negative_tests.sh
docs/                hardware, build, recovery, pre-flash checklist, risks
artifacts/           release binary, ELF/map, logs, verification reports
```

## Quick start

```bash
scripts/build.sh --twice
```

```bash
scripts/negative_tests.sh
```

Release artifact: `artifacts/falcon_hs_r2_hhkb_via.bin`, SHA-256
`c811d03f6b6bb589cc891bcd4e61eadfe3b29eb28a01c81f9b0b1d8e10e28b45`, 42,084 of
55,296 bytes of application flash.

## Documents

* [docs/hardware.md](docs/hardware.md): what was verified in the vendor firmware, and where.
* [docs/build.md](docs/build.md): offline, reproducible build procedure and results.
* [docs/recovery.md](docs/recovery.md): restoring the manufacturer firmware.
* [docs/preflash-checklist.md](docs/preflash-checklist.md): gate and first-test plan.
* [docs/risks.md](docs/risks.md): unresolved risks and unsupported features.

## Default keymap (HHKB)

```
Esc  1  2  3  4  5  6  7  8  9  0  -  =  \  `
Tab   Q  W  E  R  T  Y  U  I  O  P  [  ]  Bspc
Ctrl   A  S  D  F  G  H  J  K  L  ;  '  Enter
Shift    Z  X  C  V  B  N  M  ,  .  /  Shift Fn
      Alt  GUI        Space        GUI  Alt
```

Fn layer: the HHKB Fn layer (F-keys, Ins/Del, Caps on Tab, PrtSc/ScrLk/Pause,
arrows on `[ ; ' /`, Home/End/PgUp/PgDn, volume on A/S/D, keypad operators),
plus underglow controls on Z–B (toggle, mode, hue, saturation, brightness).
Bootloader: hold Esc while plugging in (this also clears EEPROM).
