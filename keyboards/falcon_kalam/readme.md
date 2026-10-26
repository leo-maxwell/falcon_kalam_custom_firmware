# falcon_kalam

Independent QMK definition for falcon_kalam, the 迦蓝 clone of MatrixLAB's
Falcon (shown to users as "Falcon"; not the genuine `matrix/falcon`), a 60% HHKB-layout hot-swap
keyboard built on a GigaDevice GD32F103C8T6 with the STM32duino/Maple DFU
bootloader.

* Keyboard maintainer: [leomaxwell](https://github.com/leomaxwell)
* Hardware supported: the 迦蓝 Falcon clone PCB (GD32F103C8T6, USB `9DD1:9D89`)
* Hardware evidence and open questions: see `docs/hardware.md` in the
  parent project.

This is the normal (base) keyboard definition; its `default` keymap is HHKB.
The project's release firmware adds a policy layer from `userspace/`
(`falcon_kalam:ship`, `falcon_kalam:dev`); see `docs/policy.md`. Make example (after
`scripts/build.sh` has staged this folder into `qmk_firmware/keyboards`):

    make falcon_kalam:default

**Flash only per `docs/operations.md`.**

## Bootloader

* **Bootmagic reset**: hold the top-left key (Esc, matrix 0,0) while plugging
  in. This jumps to the bootloader *and clears the persistent configuration*.
* **No power-on window**: this unit's bootloader starts the application
  straight away, so a plain replug never shows `1EAF:0003`. Other ways in:
  `QK_BOOT`, or shorting the reset/boot pads as a last resort.
