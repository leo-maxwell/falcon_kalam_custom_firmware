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
4. Checks the QMK pin, its submodules and a clean tree, and enforces **POLICY-QMK-LATEST** offline (`scripts/qmk_policy.sh --offline`).
5. Stages the base keyboard and runs `qmk lint` (upstream rules) on it.
6. Builds, each from clean, with `SKIP_VERSION=yes` and `LC_ALL=C TZ=UTC`:
   - the **base** `falcon_hs_r2:default`: the normal firmware, reference only, with NKRO;
   - the **release** `falcon_hs_r2:falcon`: base plus the userspace policy layer (`QMK_USERSPACE=userspace/`). The build fails if this keymap is not taken from `userspace/`.
7. Copies `.bin` (with DFU suffix), `.elf`, `.map`, compiler flags and generated `info_config.h` for both into `artifacts/`.
8. With `--twice`, rebuilds the release from clean and requires an identical SHA-256.
9. Verifies the base (`--profile base`: NKRO present, no VIA) and the release (`--profile release`: NKRO absent, VIA present, HHKB as VIA's default view, keymap from userspace).

## Results of this revision

| | Release `falcon_hs_r2_falcon.bin` | Base `falcon_hs_r2_default.bin` |
|---|---|---|
| SHA-256 | see `artifacts/falcon_hs_r2_falcon.bin.sha256` | see `artifacts/falcon_hs_r2_default.bin.sha256` |
| Payload | 42,320 B (76.5% of 55,296), 12,976 B free | 38,152 B |
| NKRO / VIA | absent / present | present / absent |
| Flash? | the only image that may be flashed after approval | **never**: reference only |

Reproducibility: identical hash across independent clean builds. Lint, both
verifications and the negative tests pass (see `artifacts/`).
