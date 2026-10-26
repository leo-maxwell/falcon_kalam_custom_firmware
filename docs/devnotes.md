# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | **ship** release 11 (`a04a7115…`), flashed 2026-10-27. Before it on the same day: dev releases 1–4, 6, 7 and ship releases 8–10, each read back equal to its image (release 5 was never flashed) |
| Next | ship release 11: Esc→DFU test and readback, normal plug-in (one clean light-up), reset boot; VIA check; then ship is the daily image |
| Assembly | being reassembled; the pads are no longer needed (Esc→DFU works) |
| Phase | **alpha** |

## To do

1. **Dev image test session**: done 2026-10-27 (results below; the ordered script is in the git history, commit `a374beb`).
2. **Ship image acceptance.** Flash it (`scripts/flash.sh flash ship <sha256>`), run the Esc→DFU test and readback, then check:
   - selective Command: H D X K V S B work; Shift+Shift+Backspace, Esc, Caps, 1, F1 and N type normally, and in HHK mode so does Shift+Shift+left-◇;
   - POLICY-HHKB as in dev (modes and virtual DIP switches);
   - RGB: green at the cap, indicators as in dev, Fn+Z–B do nothing special, no lighting menu in VIA.
   - Bootmagic (Esc at plug-in) → all red; replug → dark, then red ×3 on dark, then green (release 8: no normal state before the blinks).

   Then keep ship as the daily image.

## Before shipping (leaving alpha)

* the ship image passes its acceptance test (To do 2);
* every hardware test in To do 1 passed.

FLASHREAD is already out of ship (POLICY-VARIANTS, enforced by the build).

## In progress

* none

## Results awaiting a milestone

* Ship acceptance 2026-10-27 (ship release 8): all green; Esc→DFU all red, readback = image; selective Command passes (`firmware.md`); modes, switches and settings display as in dev. Release 10: dark before the reset blinks. Open: the flicker at a normal plug-in (release 11), VIA without a lighting menu (not yet checked).
* Dev round 2026-10-27: recorded in `firmware.md` (Hardware column) and the `problems.md` ledger. Logs: `artifacts/dev_session_2026-10-27_console.log`, `artifacts/dev_session_2026-10-27_usb.txt`, key check `artifacts/dev_session_2026-10-27_keycheck.txt`. Open from it: dark-from-boot on a reset boot (release 8, ship acceptance) and the dim green before enumeration (too brief to judge).
* `artifacts/release_keymap_baseline.json` is a 4-layer dump of the retired image `9b61141e…`; it does not apply to the 6-layer images.
