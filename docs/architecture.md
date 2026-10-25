# Architecture: normal base + policy layer

```
qmk_firmware/            upstream QMK, always the latest release (docs/policy.md)
        ▲
keyboards/falcon_hs_r2/  BASE: a normal QMK keyboard for this hardware
        ▲                (what an upstream submission would look like)
userspace/               POLICY: this project's configuration (QMK External Userspace)
  keyboards/falcon_hs_r2/keymaps/falcon/   → release firmware falcon_hs_r2_falcon.bin
```

## Base — `keyboards/falcon_hs_r2/`

The base holds hardware facts only, each traced to the vendor image
(`docs/hardware.md`). Everything else is the normal QMK feature set, the same
as the vendor shipped:

* pins, `COL2ROW`, USB IDs, `stm32duino` bootloader, wear-leveling EEPROM, a
  linker script that keeps code out of the bootloader and EEPROM pages;
* bootmagic, extrakey, mousekey, **NKRO on**, rgblight with all effects, 4 × WS2812 on PA1;
* `LAYOUT_60_hhkb` (first, i.e. default) and `LAYOUT_all`; keymap `default` = HHKB;
* no VIA (QMK does not allow VIA in keyboard folders) and no project policy.

It passes `qmk lint` with upstream rules. `falcon_hs_r2_default.bin` is built
and verified as a reference ("normal firmware"). **It is not the release and
must not be flashed** — it has NKRO.

## Policy layer — `userspace/keyboards/falcon_hs_r2/keymaps/falcon/`

Every deviation from the base lives here, tagged so it can be found with `grep POLICY-`:

| Tag | Where | What | How to lift it |
|---|---|---|---|
| `POLICY-NKRO` | `rules.mk`, `config.h` | `NKRO_ENABLE = no` and a build-time `#error` guard | delete the rules line and the guard; set `scripts/verify_firmware.py` release checks accordingly; re-test macOS Caps Lock |
| `POLICY-VIA` | `rules.mk` | `VIA_ENABLE = yes` | — |
| `POLICY-FLASHREAD` | `keymap.c` | read-only raw-HID command to back up the bootloader, option bytes and UID (`tools/falcon_probe.py dump-bootloader`) | delete `via_command_kb` |
| keymap | `keymap.c` | HHKB base + Fn on `LAYOUT_all`, 4 VIA layers | — |

The base is never edited to express policy. When upstream fixes the NKRO
behaviour, only the `POLICY-NKRO` lines change.

## How it builds

`scripts/build.sh` copies the base into `qmk_firmware/keyboards/` (ignored by
the submodule, so the pinned tree stays clean) and sets
`QMK_USERSPACE=userspace/`. QMK then resolves `falcon_hs_r2:falcon` to the
userspace keymap. The build fails if the release keymap does not come from
`userspace/`.

## VIA definition — `via/falcon_hs_r2.json`

This is the vendor JSON with two changes: the name, and the backspace option
reordered so that **layout-option value 0 (VIA's default) is the HHKB split
`\` + `` ` ``**. The toggle is now labelled "2u Backspace (non-HHKB)". The
verifier checks that VIA's default view equals `LAYOUT_60_hhkb` exactly.
