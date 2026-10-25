# Pre-flash validation checklist and first hardware test plan

**Nothing has been flashed.** Nobody flashes, erases, resets persistent
configuration or modifies the keyboard until a separate review has approved
the first hardware test and filled in section 0. Before approval, only
read-only inspection is allowed (docs/policy.md, POLICY-READONLY-DEVICE).

## 0. Approval (human review)

| | |
|---|---|
| Reviewer | |
| Date | |
| Image approved (must be `falcon_hs_r2_falcon.bin`, **not** `_default`) | |
| SHA-256 approved | |
| R1 decision: flash first and dump via FALCON_READ / SWD backup first | |
| R10: POLICY-FLASHREAD code reviewed (`userspace/.../falcon/keymap.c`) | |
| L-1 layout: **settled 2026-10-26 (HHKB classic)** | done |
| R13: no power-on DFU window: pads accessible for the first test | |
| R12: console + Command in release (keep / debug-only variant) | |

## A. Offline checks (no keyboard needed)

- [ ] `cd originals && shasum -a 256 -c SHA256SUMS` → all `OK`.
- [ ] `scripts/qmk_policy.sh --online` → `policy (online): OK` (QMK is the latest release).
- [ ] `scripts/build.sh --twice` exits 0. The log shows `Lint check passed!`, `reproducible: yes`, and the release keymap coming from `userspace/`.
- [ ] `artifacts/falcon_hs_r2_falcon.bin.sha256` equals the hash in section 0.
- [ ] `artifacts/vendor_analysis.txt`, `verification_base.txt` and `verification.txt` all end with `RESULT: PASS`.
- [ ] `scripts/negative_tests.sh` → `RESULT: PASS`.
- [ ] Every risk marked **HR** in `docs/risks.md` has been decided.

## B. Read-only, with the keyboard (allowed before approval)

- [x] Stock probe 2026-10-26: `artifacts/stock_probe_info.txt`, `artifacts/stock_live_keymap.json` (live layers 0–1 = compiled stock keymap, layers 2–3 transparent).
- [x] Layout probe 2026-10-26 → L-1 settled (`docs/layout.md`).
- [x] Power-on DFU window measured: none (`artifacts/usb_watch_replug.txt`, R13).
- [ ] Enter DFU (Bootmagic: hold Esc while plugging in; this erases the vendor EEPROM, which is accepted) → `dfu-util -l` shows `1eaf:0003` with alt 2 = "Upload to Flash 0x8002000".
- [ ] Fresh upload equals the backup:

```bash
dfu-util -d 1EAF:0003 -a 2 -U build/readback_pre_flash.bin -Z 57344
```

```bash
cmp -n 43940 build/readback_pre_flash.bin originals/falcon_flash_backup.bin && echo "application unchanged"
```

- [ ] (If R1 = SWD first) full-chip SWD dump, stored read-only in `originals/`.

## C. First hardware test (only after approval) **[WRITES DEVICE]**

Use **alt 2 only**. Alt 1 erases `0x08002000`.

```bash
dfu-util -d 1EAF:0003 -a 2 -D artifacts/falcon_hs_r2_falcon.bin -R
```

The keyboard probably stays in DFU afterwards (R7). **Unplug and replug** it.
The first boot reinitialises EEPROM (R4, accepted).

1. **Back up the bootloader first** (R1, route A):
   `tools/falcon_probe.py dump-bootloader artifacts/bootloader_dump` → the two reads are identical → identify the variant (`docs/bootloader.md`) → copy it read-only into `originals/` and add it to `SHA256SUMS`.
2. **Bootloader re-entry (do this second, R13)**: unplug, hold Esc, plug in → `dfu-util -l` shows `1eaf:0003`. Replug to return. If this fails, stop and restore (`recovery.md`).
3. **Enumeration**: `system_profiler SPUSBDataType` shows `9dd1:9d89` "Falcon HS R2". `tools/falcon_probe.py info` → protocol `0x000D`, 4 layers, layout options `0x00000000` (HHKB).
4. **macOS Caps Lock / input source** (R8): Fn+Tab toggles Caps Lock reliably. Switching the input source with Caps Lock works if enabled. Both still work after a replug.
5. **Matrix and keymap**: VIA with `via/falcon_hs_r2.json` (it opens as HHKB) → key tester. Every key registers once at the expected position (L-1).
6. **RGB**: Fn+Z toggles, Fn+X cycles modes, red/green/blue appear in the right order (R6). The VIA lighting menu works.
7. **Persistence**: change a key in VIA, replug → the change survives.
8. **Parity extras**: LShift+RShift+B → DFU (Command); `qmk console` shows output after LShift+RShift+D; `tools/falcon_probe.py info` on our firmware → compare with `artifacts/stock_probe_info.txt`.
9. **Bootmagic reset**: hold Esc while plugging in → DFU. Replug → defaults are back.
10. Record the results in `docs/hardware.md` and `docs/layout.md` ("Unverified" → "Verified"/"Refuted").
