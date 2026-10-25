# Build procedure (offline, reproducible)

## Prerequisites (already installed on the development Mac)

| Tool | Version used | Source |
|---|---|---|
| arm-none-eabi-gcc | 8.5.0 (Homebrew `arm-none-eabi-gcc@8`, keg-only) | `/opt/homebrew/opt/arm-none-eabi-gcc@8/bin` |
| arm-none-eabi-binutils | 2.41 | `/opt/homebrew/opt/arm-none-eabi-binutils/bin` |
| QMK CLI | 1.1.8 | Homebrew `qmk` |
| GNU Make | 3.81 | macOS |
| Python | 3.x, standard library only, for the project scripts | — |
| rsync, shasum, dfu-suffix | system / Homebrew | — |

To use other toolchain locations, set `FALCON_GCC_BIN` / `FALCON_BINUTILS_BIN`.

The build needs no network access: QMK and all its submodules (ChibiOS,
ChibiOS-contrib, …) are already checked out under `qmk_firmware/`, and no step
downloads anything. (Not tested with the network physically disabled.)

## Source pinning

`qmk_firmware/` is a git submodule pinned to
`180f394b489c238cb231c2bf8b080262155e03c0` (tag 0.34.7, 2026-10-16). The
build refuses to run if that commit, or any nested submodule commit, differs,
or if the QMK tree has local modifications.

The keyboard definition lives in `keyboards/falcon_hs_r2/` (outside QMK). For
each build it is copied into `qmk_firmware/keyboards/falcon_hs_r2/`, and that path
is listed in the submodule's `info/exclude`, so the pinned tree stays clean.
Always edit the top-level copy; the staged copy is overwritten every time.

## Commands

```bash
scripts/build.sh --twice
```

```bash
scripts/negative_tests.sh
```

`build.sh` steps, all logged to `artifacts/build.log`:

1. Records the tool versions.
2. Checks `originals/SHA256SUMS`.
3. Re-runs `tools/analyze_vendor_firmware.py` and writes `artifacts/vendor_analysis.txt`.
4. Checks that the QMK commit and its submodules match the pins and that the tree is clean.
5. Stages the keyboard and runs `qmk lint` for both keymaps.
6. Builds `falcon_hs_r2:default` as a compile check and `falcon_hs_r2:hhkb_via` as the release, each from clean, with `SKIP_VERSION=yes` (fixed version and date strings) and `LC_ALL=C TZ=UTC`.
7. Copies the `.bin` (with DFU suffix), `.elf`, `.map`, compiler flags and the generated `info_config.h` into `artifacts/`.
8. With `--twice`, rebuilds from clean and requires an identical SHA-256.
9. Runs `scripts/verify_firmware.py` and writes `artifacts/verification.txt`.

## Results of this revision

| | |
|---|---|
| Release image | `artifacts/falcon_hs_r2_hhkb_via.bin` |
| SHA-256 | `c811d03f6b6bb589cc891bcd4e61eadfe3b29eb28a01c81f9b0b1d8e10e28b45` |
| Payload | 42,084 bytes at `0x08002000–0x0800C463` (76.1% of 55,296), plus a 16-byte DFU suffix |
| Free application flash | 13,212 bytes |
| Reproducibility | identical hash across independent clean builds |
| Lint | `qmk lint` passed for `default` and `hhkb_via` |
| Verification | `artifacts/verification.txt`: PASS |
| Negative tests | `artifacts/negative_tests.log`: PASS |
| Default keymap (compile check) | 37,502 bytes |

## Keymaps

* `default`: native HHKB US ANSI on `LAYOUT_60_hhkb`, 2 layers, no VIA.
* `hhkb_via` (**release**): the same base and Fn layers on `LAYOUT_all`, so
  the split backspace, left shift and spacebar variants all have defaults, plus
  2 transparent layers for VIA (4 total). Use with `via/falcon_hs_r2.json`
  (the vendor JSON with only the name changed) or with the original vendor JSON.
