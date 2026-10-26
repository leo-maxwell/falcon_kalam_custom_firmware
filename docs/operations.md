# Operations runbook

Commands marked **[WRITES DEVICE]** change the keyboard and need an approval
(POLICY-FLASH-GATE). Everything else is read-only or offline.

## 1. Build (offline, reproducible)

Prerequisites (installed): Homebrew `arm-none-eabi-gcc@8` (8.5.0) and
`arm-none-eabi-binutils` (2.41), keg-only, put on `PATH` by the scripts
(override with `FALCON_KALAM_GCC_BIN` / `FALCON_KALAM_BINUTILS_BIN`); QMK CLI 1.1.8;
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
5. builds base (`falcon_kalam:default`), ship (`falcon_kalam:ship`) and dev (`falcon_kalam:dev`) from clean with `SKIP_VERSION=yes LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=<QMK commit time>`, and asserts each variant's keymap comes from its `userspace/` directory;
6. with `--twice`, rebuilds ship and dev and requires identical hashes;
7. verifies the three images (`verification_base.txt`, `verification.txt` = ship, `verification_dev.txt`), writes `feature_parity.txt` (stock vs ship), and runs `check_docs.py`.

`negative_tests.sh` proves that `NKRO_ENABLE=yes` fails the ship build, that an
image reaching the EEPROM pages fails to link, and that the ship build refuses
FLASHREAD, `VIA_INSECURE` and debug traces (POLICY-VARIANTS).

## 2. Flash

Always through `scripts/flash.sh` (POLICY-FLASH-GATE). It accepts only `ship`
or `dev`, requires the full approved SHA-256 (matched against the file, its
`.sha256` record and `docs/firmware.md`), hard-codes DFU alt 2, refuses a
device that is not in DFU, and saves a pre-flash readback.

**Approval** (in chat or `devnotes.md`): the variant (ship, or dev for
testing), its full SHA-256 from `firmware.md` §1, and the decisions on any open
**HR** problems. The daily image is always ship.

1. Offline: `cd originals && shasum -a 256 -c SHA256SUMS`; `scripts/qmk_policy.sh --online`; `scripts/build.sh --twice`; `scripts/negative_tests.sh`; every report `RESULT: PASS`, including `check_docs.py`.
2. `scripts/flash.sh check <variant> <sha256>` validates everything without touching the device.
3. Enter DFU: unplug, hold **Esc**, plug in, hold about 2 s (this clears EEPROM). The LEDs go red (our images) or stay dark (stock).
4. **[WRITES DEVICE]** `scripts/flash.sh flash <variant> <sha256>`. It reads back first (`build/readback_pre_*.bin`), then flashes; the log goes to `artifacts/flash_*.log`. The app starts by itself.
5. Check that `9DD1:9D89` "Falcon" enumerates, then run `tools/falcon_kalam_probe.py info`.
6. **Esc→DFU test**, while recovery is still easy: unplug, hold Esc, plug in → DFU (red), then `scripts/flash.sh verify <variant>` (read-only: flash content = image). Replug normally.
7. Run the open hardware tests from `devnotes.md`, and record the results there.

## 3. Hardware test procedures

* **Probe** (VIA, read-only): `tools/falcon_kalam_probe.py info | dump-keymap OUT.json`. `matrix` works on stock and dev (not ship, problems L1); `dump-bootloader DIR` works on dev only (FLASHREAD).
* **Console** (keep a listener running):
  ```bash
  qmk console -t -d 9DD1:9D89
  ```
  Command combos: hold **both Shifts** and tap the key. D = debug on/off,
  X = matrix print, K = keyboard reports. Turn them off in reverse order. Dev
  adds traces (key actions, EEPROM, scan rate) while debug is on, the full
  Command set.
* **VIA**: load `via/falcon_kalam.json` (both variants).
* **Passive USB log**: `tools/usb_watch.py --seconds 60 --log FILE`.

## 4. Recovery

Ways into DFU: `docs/hardware.md` §3 (Esc at plug-in works on stock and ours;
otherwise the pads or SWD).

| Goal | Command |
|---|---|
| Restore stock **[WRITES DEVICE]** (the one case for a manual `dfu-util -D`, since `flash.sh` only takes our images; keep `-a 2`) | `dfu-util -d 1EAF:0003 -a 2 -D originals/manufacturer_firmware.bin -R` (suffix-checked; application region only) |
| Restore the exact original flash incl. EEPROM **[WRITES DEVICE]**, untested whether the bootloader accepts a download up to the last page | `cp originals/falcon_flash_backup.bin build/restore_full.bin && chmod u+w build/restore_full.bin && dfu-suffix -v 1EAF -p 0003 -a build/restore_full.bin && dfu-util -d 1EAF:0003 -a 2 -D build/restore_full.bin -R` |
| Verify a restore (read-only) | upload as in §2 step 7, then `cmp -n 43940 build/readback_after.bin originals/falcon_flash_backup.bin` |
| Last resort: SWD | PA13/PA14, RDP off, so a full read and write is possible. Use a GD32F1-capable probe/OpenOCD target. The bootloader image is `originals/device_dump_2026-10-26/bootloader_0x08000000.bin` (= upstream `generic-none`). The SWD pads are not located yet |

## 5. QMK update (POLICY-QMK-LATEST)

1. `scripts/qmk_policy.sh --online` fails → `scripts/qmk_policy.sh --update` (fetches, checks out the newest tag, syncs submodules, stages the gitlink).
2. `scripts/build.sh --twice`; `scripts/negative_tests.sh`.
3. Diff `artifacts/verification.txt` and `artifacts/feature_parity.txt` against the previous commit. Changes in stock-equivalence or parity rows need review.
4. Update `firmware.md` §1, commit, and flash per §2 with a fresh approval.
