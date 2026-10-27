# Operations runbook

Commands marked **[WRITES DEVICE]** change the keyboard and need an approval
(POLICY-FLASH-GATE). Everything else is read-only or offline.

## 1. Build (offline, reproducible)

Prerequisites (macOS): Homebrew `arm-none-eabi-gcc@8` (8.5.0) and
`arm-none-eabi-binutils` (2.41), keg-only and put on `PATH` by the scripts
(override with `FALCON_KALAM_GCC_BIN` / `FALCON_KALAM_BINUTILS_BIN`); QMK CLI
1.1.8; GNU Make 3.81; Python 3 (standard library; the USB tools
`falcon_kalam_probe.py` and `usb_watch.py` run on the QMK CLI's Python for its
`hid` and `pyusb` modules, through their shebang); rsync, shasum, dfu-util
0.11, dfu-suffix. No network access is needed. `scripts/env.sh` holds the
toolchain and QMK environment; source it to run `scripts/verify_firmware.py`
by hand.

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
7. verifies the three images (`verification_base.txt`, `verification.txt` = ship, `verification_dev.txt`), writes `feature_parity.txt` (stock vs ship), and runs `check_docs.py` (a failure here is only a warning in `build.sh`, because the new hashes are written into `firmware.md` after the build; §2 step 1 requires it to pass).

`negative_tests.sh` (log: `artifacts/negative_tests.log`) proves that the ship
build refuses `NKRO_ENABLE=yes`, FLASHREAD, `VIA_INSECURE`, debug traces and
`FALCON_KALAM_DEV`; that an image reaching the EEPROM fails to link; and that
`flash.sh` refuses the L1 image, a wrong hash and a short hash.

## 2. Flash

Always through `scripts/flash.sh` (POLICY-FLASH-GATE). It accepts only `ship`
or `dev`, requires the full approved SHA-256 (matched against the file, its
`.sha256` record and `docs/firmware.md`), hard-codes DFU alt 2, refuses a
device that is not in DFU, and saves a pre-flash readback.

**Approval** (recorded in `devnotes.md`): the variant (ship, or dev for
testing), its full SHA-256 from `firmware.md` §1, and a decision on every open
**HR** problem. The daily image is always ship.

1. Offline: `cd originals && shasum -a 256 -c SHA256SUMS`; `scripts/qmk_policy.sh --online`; `scripts/build.sh --twice`; `scripts/negative_tests.sh`; every report says `RESULT: PASS`, including `check_docs.py`.
2. `scripts/flash.sh check <variant> <sha256>` validates everything without touching the device.
3. Enter DFU: unplug, hold **Esc**, plug in, hold about 2 s (this clears EEPROM). The LEDs go red (our images) or stay dark (stock).
4. **[WRITES DEVICE]** `scripts/flash.sh flash <variant> <sha256>`: reads back first (`build/readback_pre_*.bin`), then flashes; log in `artifacts/flash_*.log`. The app starts by itself (`-R` makes the bootloader reset). If the download fails, do not unplug: the device is still in DFU, so repeat the flash (or the §4 restore) at once; a half-written image would run at the next plug-in, and only a fault in it leads back to DFU (L14).
5. Check that `9DD1:9D89` "Falcon" enumerates, then run `tools/falcon_kalam_probe.py info` (its firmware release must match `firmware.md`).
6. **Esc→DFU test**, while recovery is still easy: unplug, hold Esc, plug in → DFU (red), then `scripts/flash.sh verify <variant>` (read-only: flash content = image). Replug normally.
7. Run the open hardware tests from `devnotes.md` and record the results there.

## 3. Hardware test procedures

* **Probe** (VIA, read-only): `tools/falcon_kalam_probe.py info | dump-keymap OUT.json`. `matrix` works on stock and dev, not ship (problems L1); `dump-bootloader DIR` works on dev only (FLASHREAD).
* **Console** (keep a listener running):
  ```bash
  qmk console -t -d 9DD1:9D89
  ```
  Command combinations: hold **both Shifts** and tap the key. D = debug
  on/off, X = matrix print, K = keyboard reports; turn them off in reverse
  order. Dev has the full Command set and adds traces (key actions, EEPROM,
  scan rate) while debug is on.
* **Key check** (offline): `tools/console_keycheck.py LOG` compares the key presses in a dev console log (`ACTION_DEBUG` + wear-leveling traces) with the POLICY-HHKB table.
* **VIA**: load `via/falcon_kalam.json` (both variants).
* **Passive USB log**: `tools/usb_watch.py --seconds 60 --log FILE`.

## 4. Recovery

Ways into DFU: `docs/hardware.md` §3 (Esc at plug-in works on stock and ours;
otherwise the pads). There is no SWD probe, so DFU is the only recovery path;
that is why every flash ends with the Esc→DFU test (§2 step 6).

| Goal | Command |
|---|---|
| Restore stock **[WRITES DEVICE]** (the one case for a manual `dfu-util -D`, since `flash.sh` takes only our images; keep `-a 2`) | `dfu-util -d 1EAF:0003 -a 2 -D originals/manufacturer_firmware.bin -R` (suffix-checked; application region only) |
| Restore the exact original flash incl. EEPROM **[WRITES DEVICE]**; whether the bootloader accepts a download up to the last page is untested | `cp originals/falcon_flash_backup.bin build/restore_full.bin && chmod u+w build/restore_full.bin && dfu-suffix -v 1EAF -p 0003 -a build/restore_full.bin && dfu-util -d 1EAF:0003 -a 2 -D build/restore_full.bin -R` |
| Verify a restore (read-only, in DFU; `dfu-util` refuses to overwrite an upload file) | `rm -f build/readback_after.bin && dfu-util -d 1EAF:0003 -a 2 -U build/readback_after.bin -Z 57344`, then `cmp -n 43940 build/readback_after.bin originals/falcon_flash_backup.bin` |

## 5. QMK update (POLICY-QMK-LATEST)

1. `scripts/qmk_policy.sh --online` fails → `scripts/qmk_policy.sh --update` (fetches, checks out the newest tag, syncs submodules, stages the gitlink).
2. Read the release notes for a fix of NKRO on macOS (Caps Lock / input-source toggle); if one is claimed, record it in `devnotes.md` for a decision on lifting POLICY-NKRO (I8). Nothing is fixed here (POLICY-UPSTREAM).
3. `scripts/build.sh --twice`; `scripts/negative_tests.sh`.
4. Diff `artifacts/verification.txt` and `artifacts/feature_parity.txt` against the previous commit; changed stock-equivalence or parity rows need review. Re-derive `KEYMAP_BASE` in `tools/console_keycheck.py` (the VIA keymap's EEPROM offset: eeconfig size + VIA magic + layout options) from the new QMK's `quantum/nvm/eeprom/` headers; a wrong value shows as "lookup at r,c" mismatches in the key check.
5. Bump `VIA_FIRMWARE_VERSION`, update `firmware.md` §1, commit, and flash per §2 with a fresh approval. The build date changes with QMK, so VIA resets its saved keymap on the first boot.
