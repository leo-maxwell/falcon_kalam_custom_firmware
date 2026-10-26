# falcon_kalam — independent QMK firmware

Independent, reproducible QMK firmware for **falcon_kalam**: the 迦蓝 clone of
MatrixLAB's Falcon (GD32F103C8T6,
STM32duino DFU bootloader, USB `9DD1:9D89`): HHKB layout, VIA, stock feature
parity, NKRO disabled by configuration for macOS.
Users see it as "Falcon"; the naming rules are in `docs/policy.md` (POLICY-NAMING).

**Current status:** [`docs/devnotes.md`](docs/devnotes.md).

## Documentation

| # | Document | Contents |
|---|---|---|
| 1 | [`docs/hardware.md`](docs/hardware.md) | hardware facts and conventions: pins, matrix and layout, lights, bootloader, memory |
| 2 | [`docs/stock-firmware.md`](docs/stock-firmware.md) | what the stock firmware does |
| 3 | [`docs/policy.md`](docs/policy.md) | design, policies, file organisation, documentation rules, design notes |
| 4 | [`docs/firmware.md`](docs/firmware.md) | our firmware's features and the current release |
| 5 | [`docs/devnotes.md`](docs/devnotes.md) | status, to-do, work in progress |
| 6 | [`docs/problems.md`](docs/problems.md) | risks, bugs, limitations |
| 7 | [`docs/improvements.md`](docs/improvements.md) | optional improvements |
| 8 | [`docs/operations.md`](docs/operations.md) | build, flash, test, recover, update QMK |

## Quick start

```bash
scripts/build.sh --twice
```

Two images are built (POLICY-VARIANTS): **ship** `artifacts/falcon_kalam_ship.bin`
for daily use, and **dev** `artifacts/falcon_kalam_dev.bin` with every debug
feature, for testing only. Flash only by following `docs/operations.md` §2.
