# falcon_kalam — independent QMK firmware for the 迦蓝 Falcon clone

Independent, reproducible [QMK](https://qmk.fm) firmware for **falcon_kalam**,
the 迦蓝 (Kalam) clone of MatrixLAB's Falcon: a 60-key HHKB-layout hot-swap
keyboard on a GigaDevice GD32F103C8T6 with the STM32duino DFU bootloader,
USB `9DD1:9D89`, sold with a QMK-based manufacturer firmware.

**This is not the firmware the keyboard was sold with**, and it is not
affiliated with the 迦蓝 team or MatrixLAB. It was written for one unit by
reverse-engineering its stock image; use it at your own risk. It is for the
GD32 clone only: MatrixLAB's own Falcon (`matrix/falcon` in QMK, an AVR board)
must never be flashed with it.

## What it does

* **Mimics the HHKB Professional Classic** in every way possible: its keys,
  HID codes, three setting modes (HHK, Mac, Win), mode keys and DIP switches
  (as saved, virtual switches); HHK mode from the factory, as the Classic.
* **Keeps every stock feature**: VIA, the HID console, Command key
  combinations, Bootmagic; NKRO is off because it breaks the macOS Caps Lock /
  input-source toggle.
* **Adds** crash-to-DFU recovery, a settings schema that resets stale
  configurations, a Caps Lock LED that ignores macOS's input-source flicker,
  and a four-LED **status indicator** instead of lighting effects (all green =
  normal; red = attention; blue = a state worth knowing).
* **Two images from one tree**: `ship` for daily use and `dev` with every
  debug feature; both bit-reproducible and verified offline on every build,
  with an audit trail from the manufacturer firmware to each built image.

Users see the keyboard as "Falcon"; `falcon_kalam` is the development name.

## Documentation

| # | Document | Contents |
|---|---|---|
| 1 | [`docs/manual.md`](docs/manual.md) | **user manual**: layout, modes, switches, the lights, key combinations, flashing, VIA |
| 2 | [`docs/firmware.md`](docs/firmware.md) | our firmware's features, the current images, what L2 changes against the L1 base |
| 3 | [`docs/hardware.md`](docs/hardware.md) | hardware facts and conventions: pins, matrix and layout, lights, bootloader, memory, the HHKB reference |
| 4 | [`docs/stock-firmware.md`](docs/stock-firmware.md) | what the stock firmware does |
| 5 | [`docs/policy.md`](docs/policy.md) | design, policies, file organisation, documentation rules, design notes |
| 6 | [`docs/operations.md`](docs/operations.md) | build, flash, test, recover, update QMK |
| 7 | [`docs/problems.md`](docs/problems.md) | risks, bugs, limitations |
| 8 | [`docs/improvements.md`](docs/improvements.md) | optional improvements |
| 9 | [`docs/devnotes.md`](docs/devnotes.md) | status, to-do, work in progress |

## Layout of the repository

```
keyboards/falcon_kalam/   L1: a normal QMK keyboard (hardware facts, reference keymap)
userspace/                L2: every decision (keymap, modes, lights, recovery; ship and dev variants)
qmk_firmware/             QMK 0.34.7, pinned submodule, never edited
bootloader/               STM32duino bootloader source, pinned submodule (reference only)
scripts/                  build, verify, flash, negative tests, docs check
tools/                    stock-firmware analysis, USB probe, console checks
via/                      VIA definition
artifacts/                built images with hashes, verification reports, device logs
originals/                the manufacturer firmware and device dumps, immutable
```

## Quick start (macOS)

```bash
git clone --recursive https://github.com/leo-maxwell/falcom_kalam_custom_firmware.git
```

```bash
scripts/build.sh --twice
```

The scripts are written for macOS; prerequisites are in `docs/operations.md`
§1 (Homebrew `arm-none-eabi-gcc@8`, the QMK CLI, `dfu-util`). The build writes `artifacts/falcon_kalam_ship.bin`
and `artifacts/falcon_kalam_dev.bin` and verifies both. **Flash only as
`docs/manual.md` §5 describes**: the keyboard has no power-on DFU window and
no programming header in use, so every flash ends with the test that the
bootloader is still reachable.

## Licence

GPL-2.0-or-later (`LICENSE`), like QMK. The STM32duino bootloader is MIT
(LeafLabs); its source is included as a submodule for reference and is
neither built nor flashed by this project. The files under `originals/` are
the manufacturer's firmware and dumps of this unit, kept as evidence; they are
not covered by this licence.
