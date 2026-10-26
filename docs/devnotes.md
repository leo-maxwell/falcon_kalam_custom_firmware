# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | **ship** release 12 (`62072341…`), flashed 2026-10-27. Before it on the same day: dev releases 1–4, 6, 7 and ship releases 8–11, each read back equal to its image (release 5 was never flashed) |
| Recovery | Esc at plug-in reaches DFU on every image tested; the pads are no longer needed |
| Phase | **alpha** |

## To do

1. **Finish the ship acceptance on release 12** (everything else passed on releases 8–11, `firmware.md`):
   - Esc→DFU test and `scripts/flash.sh verify ship` (required after every flash);
   - the VIA app: loads `via/falcon_kalam.json`, connects, shows no lighting menu.

   Then ship is the daily image and the project leaves alpha.

## In progress

* none

## Results awaiting a milestone

* Ship acceptance 2026-10-27 (ship releases 8–12): all green; Esc→DFU all red with an identical readback (release 8); selective Command passes; modes, switches and settings display as in dev; reset boot dark before the red blinks (release 10); normal plug-in lights once (release 11); LED 1's power-up flash accepted (L12; release 12 drives the line earlier). Logs: `artifacts/dev_session_2026-10-27_console.log`, `artifacts/ship_acceptance_2026-10-27_usb.txt`.
