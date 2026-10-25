# Pre-flash validation checklist and first hardware test plan

**The release artifact has not been flashed.** Nobody flashes, erases, resets
persistent configuration or modifies the keyboard until a separate review has
approved the first hardware test and filled in section 0.

## 0. Approval

| | |
|---|---|
| Reviewer | |
| Date | |
| Artifact SHA-256 approved | |
| Scope approved (flash / bootloader readback / EEPROM reset) | |

## A. Offline checks (no keyboard needed)

- [ ] `cd originals && shasum -a 256 -c SHA256SUMS` → all `OK`.
- [ ] `scripts/build.sh --twice` exits 0. The log shows `Lint check passed!` twice and `reproducible: yes`.
- [ ] The hash in `artifacts/falcon_hs_r2_hhkb_via.bin.sha256` matches the one in section 0.
- [ ] `artifacts/vendor_analysis.txt` ends with `RESULT: PASS`.
- [ ] `artifacts/verification.txt` ends with `RESULT: PASS`. In particular:
  - image ends below `0x0800F800`, so the bootloader and EEPROM pages are untouched;
  - no NKRO report and no `-DNKRO_ENABLE`;
  - pin tables, clock constants, flash descriptor and bootloader flags match the vendor.
- [ ] `scripts/negative_tests.sh` → `RESULT: PASS` (the NKRO guard and the flash-limit guard both fire).
- [ ] Read `docs/risks.md` and accept every open risk, especially R1 (bootloader not backed up).

## B. Before the first flash (touches the keyboard, but read-only)

- [ ] A second keyboard is connected.
- [ ] Enter the bootloader (recovery.md §1, vendor Fn+Tab) → `dfu-util -l` lists `1eaf:0003` with `alt=2` at `0x08002000`. Record the alt-setting name strings.
- [ ] Take a fresh read-only upload and compare it with the originals. A difference means the device state changed since the backup (for example, VIA edits); decide whether to keep the new upload too.

```bash
dfu-util -d 1EAF:0003 -a 2 -U build/readback_pre_flash.bin -Z 57344
```

```bash
cmp -n 43940 build/readback_pre_flash.bin originals/falcon_flash_backup.bin && echo "application unchanged"
```

- [ ] Optional, recommended (R1): a full-chip SWD backup, including the bootloader, if the SWD pads are accessible.
- [ ] Confirm the restore image and command from recovery.md §2 are ready.

## C. First hardware test (only after approval) **[WRITES DEVICE]**

```bash
dfu-util -d 1EAF:0003 -a 2 -D artifacts/falcon_hs_r2_hhkb_via.bin -R
```

Expected: the application region is rewritten and the vendor EEPROM content is
reinitialised on first boot (R4). Then:

1. **Bootloader re-entry first.** Unplug, hold Esc, plug in → `dfu-util -l` shows `1eaf:0003`. If this fails, stop and restore (recovery.md §1/§2).
2. **Enumeration**: `system_profiler SPUSBDataType` shows `9dd1:9d89`, "Falcon HS R2".
3. **Matrix**: in VIA (load `via/falcon_hs_r2.json`; select the layout options that match the unit), run the key tester. Every physical key registers exactly once and at the expected position. Note any key at `4,0/4,13/4,14` (R5).
4. **macOS Caps Lock and input source**: Fn+Tab toggles Caps Lock reliably, and the input-source switch via Caps Lock works if enabled in System Settings (R8). Fast typing loses no keys within 6KRO.
5. **Fn layer**: F1–F12, arrows (Fn+[ ; ' /), Home/End/PgUp/PgDn, volume (Fn+A/S/D), Ins/Del.
6. **RGB**: Fn+Z toggles, Fn+X cycles modes, colours are correct (GRB order), no flicker (R6). Check the VIA lighting menu.
7. **Persistence**: change a key in VIA, unplug and replug → the change survives (wear-leveling at `0x0800F800`).
8. **Bootmagic reset**: hold Esc while plugging in → bootloader. Replug normally → the VIA change from step 7 is gone (EEPROM cleared, defaults back).
9. Record the results and any deviations in `docs/hardware.md` (turn "Unverified" into "Verified" or "Refuted").
