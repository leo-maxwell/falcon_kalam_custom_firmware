# Falcon HS R2 — independent QMK firmware

A reproducible, independently maintained QMK firmware for the Falcon HS R2
(GD32F103C8T6, STM32duino/Maple DFU bootloader, USB `9DD1:9D89`), HHKB layout
by default, VIA-configurable. NKRO is supported by the base keyboard and disabled
by the release configuration (POLICY-NKRO).

> **Status: built and verified offline. Not flashed.** Flashing needs the
> separate review in [`docs/preflash-checklist.md`](docs/preflash-checklist.md).
> **Open issue L-1:** the physical layout is not settled ([`docs/layout.md`](docs/layout.md)).

## Structure: normal base + policy layer

```
originals/             immutable vendor artifacts + SHA256SUMS (read-only)
qmk_firmware/          upstream QMK, submodule, always the latest release (docs/policy.md)
keyboards/falcon_hs_r2/  BASE: a normal QMK keyboard (hardware facts, NKRO on, keymap "default" = HHKB)
userspace/             POLICY: QMK External Userspace
  keyboards/falcon_hs_r2/keymaps/falcon/   NKRO disabled (config); VIA, console, Command, LTO on; HHKB keymap; FALCON_READ
via/falcon_hs_r2.json  VIA definition (vendor JSON; HHKB is the default layout option)
bootloader/STM32duino-bootloader/  MIT bootloader source, pinned reference (never built or flashed here)
tools/                 vendor analysis, stock/release parity table, disassembly, read-only USB probe
scripts/               build.sh, verify_firmware.py, negative_tests.sh, qmk_policy.sh
docs/                  policy, architecture, parity (progress vs stock), hardware, bootloader,
                       layout, RGB cross-check, build, recovery, pre-flash checklist, risks
artifacts/             built images, logs, verification reports, device inspection
```

Progress against the stock firmware: [`docs/parity.md`](docs/parity.md). Policies:
[`docs/policy.md`](docs/policy.md). Project policy lives only in the userspace layer, tagged `POLICY-*`. To lift
the NKRO policy, edit two lines there; the base stays untouched
([`docs/architecture.md`](docs/architecture.md)).

## Quick start

```bash
scripts/build.sh --twice
```

```bash
scripts/negative_tests.sh
```

```bash
scripts/qmk_policy.sh --online
```

Release image: `artifacts/falcon_hs_r2_falcon.bin` (hash in the `.sha256` file
next to it). `falcon_hs_r2_default.bin` is the normal-firmware reference and
must **not** be flashed.

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
plus underglow controls on Z–B. Bootloader: hold Esc while plugging in (this
also clears EEPROM).
