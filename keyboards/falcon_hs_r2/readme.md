# Falcon HS R2

Independent QMK definition for the Falcon HS R2, a 60% HHKB-layout hot-swap
keyboard built on a GigaDevice GD32F103C8T6 with the STM32duino/Maple DFU
bootloader.

* Keyboard maintainer: [leomaxwell](https://github.com/leomaxwell)
* Hardware supported: Falcon HS R2 PCB (USB `9DD1:9D89`)
* Hardware evidence and open questions: see `docs/hardware.md` in the
  parent project.

Make example for this keyboard (after staging it into `qmk_firmware/keyboards`,
which `scripts/build.sh` does):

    make falcon_hs_r2:hhkb_via SKIP_VERSION=yes

**Do not flash without completing `docs/preflash-checklist.md`.**

## Bootloader

* **Bootmagic reset**: hold the top-left key (Esc, matrix 0,0) while plugging
  in. This jumps to the bootloader *and clears the persistent configuration*.
* **Power-on window**: the Maple bootloader enumerates as `1EAF:0003` for a
  short time after every power-on, before starting the application.
