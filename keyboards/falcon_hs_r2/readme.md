# Falcon HS R2

Independent QMK definition for the Falcon HS R2, a 60% HHKB-layout hot-swap
keyboard built on a GigaDevice GD32F103C8T6 with the STM32duino/Maple DFU
bootloader.

* Keyboard maintainer: [leomaxwell](https://github.com/leomaxwell)
* Hardware supported: Falcon HS R2 PCB (USB `9DD1:9D89`)
* Hardware evidence and open questions: see `docs/hardware.md` in the
  parent project.

This is the normal (base) keyboard definition; its `default` keymap is HHKB.
The project's release firmware adds a policy layer from `userspace/`
(`falcon_hs_r2:falcon`); see `docs/policy.md`. Make example (after
`scripts/build.sh` has staged this folder into `qmk_firmware/keyboards`):

    make falcon_hs_r2:default

**Flash only per `docs/operations.md`.**

## Bootloader

* **Bootmagic reset**: hold the top-left key (Esc, matrix 0,0) while plugging
  in. This jumps to the bootloader *and clears the persistent configuration*.
* **Power-on window**: the Maple bootloader enumerates as `1EAF:0003` for a
  short time after every power-on, before starting the application.
