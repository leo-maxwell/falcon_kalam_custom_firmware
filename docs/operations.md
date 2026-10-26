# Operations runbook

Commands marked **[WRITES DEVICE]** change the keyboard and need an approval
(POLICY-FLASH-GATE). Everything else is read-only or offline.

## 1. Build (offline, reproducible)

Prerequisites (installed): Homebrew `arm-none-eabi-gcc@8` (8.5.0) and
`arm-none-eabi-binutils` (2.41), keg-only, put on `PATH` by the scripts
(override with `FALCON_GCC_BIN` / `FALCON_BINUTILS_BIN`); QMK CLI 1.1.8;
GNU Make 3.81; Python 3 (standard library); rsync, shasum, dfu-util 0.11,
dfu-suffix. No network access is needed.

```bash
scripts/build.sh --twice
```

```bash
scripts/negative_tests.sh
```

`build.sh` (log: `artifacts/build.log`):

1. records tool versions;
2. checks `originals/SHA256SUMS` and re-runs the stock analysis;
3. checks the QMK pin, its submodules, a clean tree, and POLICY-QMK-LATEST offline;
4. stages L1 and runs `qmk lint`;
5. builds base (`falcon_hs_r2:default`), ship (`falcon_hs_r2:falcon`) and dev (`falcon_hs_r2:falcon_dev`) from clean with `SKIP_VERSION=yes LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=<QMK commit time>`, and asserts each variant's keymap comes from its `userspace/` directory;
6. with `--twice`, rebuilds ship and dev and requires identical hashes;
7. verifies the three images (`verification_base.txt`, `verification.txt` = ship, `verification_dev.txt`), writes `feature_parity.txt` (stock vs ship), and runs `check_docs.py`.

`negative_tests.sh` proves that `NKRO_ENABLE=yes` fails the ship build, that an
image reaching the EEPROM pages fails to link, and that the ship build refuses
FLASHREAD, `VIA_INSECURE` and debug traces (POLICY-VARIANTS).

## 2. Flash

**Approval** (in chat or `devnotes.md`): the variant (`falcon_hs_r2_falcon.bin`
= ship, or `falcon_hs_r2_falcon_dev.bin` = dev for testing), its full SHA-256,
and the decisions on any open **HR** problems. The daily image is always ship.

1. Offline: `cd originals && shasum -a 256 -c SHA256SUMS`; `scripts/qmk_policy.sh --online`; `scripts/build.sh --twice`; `scripts/negative_tests.sh`; all reports `RESULT: PASS`; `scripts/check_docs.py` passes; the hash equals the approved one.
2. Enter DFU: unplug, hold **Esc**, plug in, hold about 2 s (this clears EEPROM). The LEDs stay dark. Check with `dfu-util -l`: expect `1eaf:0003` with alt 2 "Upload to Flash 0x8002000".
3. Read-only pre-flash readback (keep it):
   ```bash
   dfu-util -d 1EAF:0003 -a 2 -U build/readback_pre_flash.bin -Z 57344
   ```
4. **[WRITES DEVICE]** flash the approved variant, alt 2 only (dev: `falcon_hs_r2_falcon_dev.bin`):
   ```bash
   dfu-util -d 1EAF:0003 -a 2 -D artifacts/falcon_hs_r2_falcon.bin -R
   ```
   A `LIBUSB_ERROR_TIMEOUT` on reset is normal. The app starts by itself.
5. Check `9DD1:9D89` "Falcon HS R2" enumerates, then run `tools/falcon_probe.py info`.
6. **Esc→DFU test**, while recovery is still easy: unplug, hold Esc, plug in → DFU; read back and compare (step 7); replug normally.
7. Verify the write (read-only):
   ```bash
   dfu-util -d 1EAF:0003 -a 2 -U build/readback_post_flash.bin -Z 57344
   ```
   The first `size − 16` bytes must equal the release payload.
8. Run the open hardware tests from `devnotes.md`, and record the results there.

## 3. Hardware test procedures

* **Probe** (VIA, read-only): `tools/falcon_probe.py info | dump-keymap OUT.json`. `matrix` works on stock and dev (not ship, problems L1); `dump-bootloader DIR` works on dev only (FLASHREAD).
* **Console** (keep a listener running):
  ```bash
  qmk console -t -d 9DD1:9D89
  ```
  Command combos: hold **both Shifts** and tap the key. D = debug on/off,
  X = matrix print, K = keyboard reports. Turn them off in reverse order. Dev
  adds traces (key actions, EEPROM, scan rate) while debug is on, and the full
  Command set.
* **Passive USB log**: `tools/usb_watch.py --seconds 60 --log FILE`.

## 4. Recovery

Ways into DFU: `docs/hardware.md` §3 (Esc at plug-in works on stock and ours;
otherwise the pads or SWD).

| Goal | Command |
|---|---|
| Restore stock **[WRITES DEVICE]** | `dfu-util -d 1EAF:0003 -a 2 -D originals/manufacturer_firmware.bin -R` (suffix-checked; application region only) |
| Restore the exact original flash incl. EEPROM **[WRITES DEVICE]**, untested whether the bootloader accepts a download up to the last page | `cp originals/falcon_flash_backup.bin build/restore_full.bin && chmod u+w build/restore_full.bin && dfu-suffix -v 1EAF -p 0003 -a build/restore_full.bin && dfu-util -d 1EAF:0003 -a 2 -D build/restore_full.bin -R` |
| Verify a restore (read-only) | upload as in §2 step 7, then `cmp -n 43940 build/readback_after.bin originals/falcon_flash_backup.bin` |
| Last resort: SWD | PA13/PA14, RDP off, so a full read and write is possible. Use a GD32F1-capable probe/OpenOCD target. The bootloader image is `originals/device_dump_2026-10-26/bootloader_0x08000000.bin` (= upstream `generic-none`). The SWD pads are not located yet |

## 5. QMK update (POLICY-QMK-LATEST)

1. `scripts/qmk_policy.sh --online` fails → `scripts/qmk_policy.sh --update` (fetches, checks out the newest tag, syncs submodules, stages the gitlink).
2. `scripts/build.sh --twice`; `scripts/negative_tests.sh`.
3. Diff `artifacts/verification.txt` and `artifacts/feature_parity.txt` against the previous commit. Changes in stock-equivalence or parity rows need review.
4. Update `firmware.md` §1, commit, and flash per §2 with a fresh approval.
