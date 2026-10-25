# Bootloader: identification, licence, backup

## What is on the device (read-only inspection, `artifacts/device_inspection_2026-10-26.txt`)

| Field | Value |
|---|---|
| USB | `1EAF:0003`, bcdDevice `0x0201`, "LeafLabs" / "Maple 003", serial "LLM 003" |
| DFU alts | 0 = "ERROR. Upload to RAM not supported.", 1 = flash `0x8005000`, 2 = flash `0x8002000` |
| Name string | "STM32duino bootloader v1.0" |
| Transfer size | 1024 |
| Size | occupies `0x08000000–0x08001FFF` (8 KB; the application starts at `0x08002000`) |

This matches Roger Clark's **STM32duino-bootloader** ("boot20" family, derived from
LeafLabs' Maple bootloader), pinned here as a reference submodule at
`bootloader/STM32duino-bootloader` (`df689808`, 2020-03-10). The source's
device descriptor has bcdDevice bytes `0x01, 0x02` (= `0x0201`).

**Licence: open source, MIT** ("Copyright (c) 2010 LeafLabs LLC", in every
source file). It is not proprietary. Redistributing it, including inside this
project, is allowed if the MIT notice is kept.

## Which build variant? (unresolved)

Upstream ships about 25 prebuilt variants (`bootloader_only_binaries/`). They
differ in LED pin, button pin and crystal. Evidence so far:

* The app's clocks (PLL ×9, USB ÷1.5) prove an **8 MHz** crystal, and the
  bootloader does USB on the same crystal. Upstream's GD32-specific targets
  (`gd32f1_generic_boot20_pc13`, `gd32f1_frankenmaple`) are built for 12 MHz
  (`XTAL12M`), so they are **unlikely**. A generic 8 MHz STM32 build (for
  example `generic_boot20_pc13`) running on the GD32 is the likely case. This
  is unverified.
* `generic_boot20_pa1` blinks an LED on **PA1**, which is the WS2812 data line
  here. If that variant is installed, the LEDs may glitch while in DFU. That
  is cosmetic.
* A vendor-modified build is also possible.

## Known upstream behaviour that affects QMK

In `hardware.c`, `checkAndClearBootloaderFlag()` stores its result in a
`bool`, so `RTC_BOOTLOADER_JUST_UPLOADED` (`0x424D`, value 2) collapses to 1
and behaves like `RTC_BOOTLOADER_FLAG` (`0x424C`): **stay in DFU**. If the
device runs this code:

* `QK_BOOT` / Bootmagic → DFU (intended);
* `QK_REBOOT` / `mcu_reset()` (writes `0x424D`) → **also DFU**, instead of
  restarting the app. A replug fixes it, and the vendor firmware behaves the
  same way.
* A DFU **download** writes `0x424D` when it starts (`dfu.c`). After the
  post-download reset the bug makes the bootloader **stay in DFU**. Expect to
  replug the keyboard after every flash. That is not a failed flash. (This
  assumes the backup domain loses power on unplug, i.e. VBAT is not
  battery-backed; unverified.)
* **Never download to alt 1.** Selecting alt 1 erases the page at
  `0x08002000` (the first page of our app) and writes to `0x08005000`. Every
  command in this project uses `-a 2`.

## Backup — why it was not possible yet, and how it will be done

The DFU upload of alt 1/2 starts at `0x08005000`/`0x08002000`. The bootloader
has no way to read itself over USB. There are two routes:

1. **Our firmware (planned, POLICY-FLASHREAD).** The release firmware answers
   a read-only raw-HID command limited to `0x08000000–0x08001FFF`, the option
   bytes and the chip UID. Right after the first approved flash:

   ```bash
   tools/falcon_probe.py dump-bootloader artifacts/bootloader_dump
   ```

   The tool reads everything twice and compares. Then hash-compare the dump
   with `bootloader/STM32duino-bootloader/bootloader_only_binaries/*.bin` to
   identify the variant, and commit it to `originals/` read-only.
   *Trade-off:* the backup exists only after the first flash. That flash
   rewrites only `0x08002000+`; the bootloader stays untouched by design (and
   by the linker script).
2. **SWD**, before any flash: PA13/PA14 stay enabled. This needs pad access,
   a probe, and read-out protection off (unknown).

## Should the bootloader be part of our code?

Its **source** is now part of the project (pinned submodule, MIT). Its
**binary** will be once dumped. Replacing or rebuilding the bootloader is out
of scope: it can only be written over SWD, and a bad bootloader bricks USB
recovery. Revisit only with SWD access and a verified dump in hand.
