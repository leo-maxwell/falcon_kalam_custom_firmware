# Recovery procedure

Every command that writes to the keyboard is marked **[WRITES DEVICE]**. None
has been run by this project. Run them only once the first hardware test has been
approved (see `preflash-checklist.md`), or to recover from a failed test.

## What is preserved

| File | Content | Use |
|---|---|---|
| `originals/manufacturer_firmware.bin` | vendor application + DFU suffix (`1EAF:0003`) | **primary restore image** |
| `originals/falcon_flash_backup.bin` | raw `0x08002000–0x0800FFFF`, incl. vendor EEPROM pages, no suffix | forensic reference; exact-state restore |
| `originals/manufacturer_jsonfile.json` | vendor VIA definition | load into VIA after restoring |
| `originals/SHA256SUMS` | hashes of the three files | `cd originals && shasum -a 256 -c SHA256SUMS` |

The originals are read-only (`chmod 444`) and committed to git. The bootloader
(`0x08000000–0x08001FFF`), F_SIZE/UID and option bytes were dumped on
2026-10-26 into `originals/device_dump_2026-10-26/`. The bootloader is upstream
`generic-none`; RDP is off, so a full SWD restore is possible. A fresh read-only upload on 2026-10-26 was byte-identical
to `falcon_flash_backup.bin` (`artifacts/device_inspection_2026-10-26.txt`).

**Always use DFU alt 2.** A download to alt 1 erases `0x08002000`. After any
download the keyboard probably stays in DFU (upstream bootloader flag bug, R7):
unplug and replug it.

## 1. Get into the bootloader

Use whichever applies, then check with `dfu-util -l`. Expect `1eaf:0003` with an
`alt=2` entry for flash at `0x08002000`.

1. **Our firmware**: unplug, hold **Esc** (matrix 0,0), plug in. This is
   Bootmagic: it clears EEPROM and calls `bootloader_jump()`.
2. **Vendor firmware**: Bootmagic (hold Esc while plugging in; the vendor has
   BOOTMAGIC compiled in and the EEPROM backup shows it was used), or Fn+Tab
   (`QK_BOOT` in the vendor's compiled keymap), or QMK Command
   LShift+RShift+B ("Jump to Bootloader" is compiled in; the key combo is QMK's
   default and unverified).
3. **Power-on window: not available.** A watched replug showed no DFU window
   (risk R13), so a plain replug does not reach the bootloader.
4. **None of the key methods work** (the firmware does not boot or scan): short the
   reset/boot pads as before, or use SWD (step 5).

## 2. Restore the manufacturer firmware **[WRITES DEVICE]**

```bash
cd originals && shasum -a 256 -c SHA256SUMS
```

```bash
dfu-util -d 1EAF:0003 -a 2 -D originals/manufacturer_firmware.bin -R
```

dfu-util checks the suffix (`1EAF:0003`) and strips it. Only the application
region is rewritten. The bootloader decides how many pages to erase; the EEPROM
pages normally stay as they are.

## 3. Optional: restore the exact original flash state **[WRITES DEVICE]**

This writes the full readback, vendor EEPROM pages included (57,344 bytes up to
`0x0800FFFF`). It is untested whether the Maple bootloader accepts a download
that reaches the last page; prefer step 2 unless the original EEPROM content is
needed.

```bash
mkdir -p build && cp originals/falcon_flash_backup.bin build/restore_full.bin && chmod u+w build/restore_full.bin
```

```bash
dfu-suffix -v 1EAF -p 0003 -a build/restore_full.bin
```

```bash
dfu-util -d 1EAF:0003 -a 2 -D build/restore_full.bin -R
```

## 4. Verify a restore (read-only)

A DFU upload only reads the device:

```bash
dfu-util -d 1EAF:0003 -a 2 -U build/readback_after.bin -Z 57344
```

```bash
cmp -n 43940 build/readback_after.bin originals/falcon_flash_backup.bin && echo "application matches vendor"
```

## 5. Last resort: SWD

Both the vendor firmware and this firmware leave SW-DP enabled (`SWJ_CFG=010`),
so PA13 (SWDIO) and PA14 (SWCLK) work with an ST-Link or a CMSIS-DAP probe. You
would need to identify the SWD pads on the PCB, which has not been done. If
read-out protection is off, read the whole 64 KB first. If the bootloader is
gone, a STM32duino ("generic_boot20") bootloader binary would have to be found
and validated. Candidates are in `bootloader/STM32duino-bootloader/bootloader_only_binaries/`;
the installed variant is not identified (risk R1). Treat the part as a GD32:
use a probe/OpenOCD target that supports GD32F1 flash programming.
