# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | **ship** release 14 (`46841114…`), flashed 2026-10-27 (probe: release 14; the pre-flash readback equalled the release 12 image); the LED checks passed (B13 closed). Release 13 was never flashed; release 15 is built and not yet flashed. Before it on the same day: dev releases 1–4, 6, 7 and ship releases 8–11, each read back equal to its image (release 5 was never flashed) |
| Recovery | Esc at plug-in reaches DFU on every image tested; the pads are no longer needed |
| Note | Release 10 entered DFU 6 s after its flash, without an unplug: that was a deliberate Shift+Shift+B, not a crash. No image has entered DFU on its own |
| Phase | **alpha** |

## To do

1. **Flash and accept release 15** (ship; `firmware.md` §1): Esc→DFU test and `scripts/flash.sh verify ship`; the VIA app loads `via/falcon_kalam.json`, connects, shows no lighting menu; dev: Shift+Shift+L runs the light test, Shift+Shift+6 types 6 (B10), `dump-bootloader` writes a 20-byte `fsize_uid` file (B12); a raw-HID "effect 0" leaves the base green (B11, needs a one-off script, optional).

   Then ship is the daily image and the project leaves alpha.
2. **Decide the remaining review proposals** below (each changes firmware behaviour).

## In progress

* none

## Review proposals (2026-10-27, awaiting a decision; each is a behaviour change). P1, P2, P5 and P7 went into release 13 (B10–B12).

| # | Proposal | Change | Why |
|---|---|---|---|
| P3 | L7: fault counter in a spare backup register (DR1–DR9 are unused): first fault → `0x424D` (the app restarts), a second fault within ~10 s of boot → `0x424C` (DFU); the counter is cleared from housekeeping after a healthy start | a transient fault no longer silences the keyboard until a replug; a crash loop still parks in DFU | handler stays stack-free (immediates only); needs the dev fault test on the device |
| P4 | L7 LEDs: paint red from the fault handler with a register-only WS2812 bit-bang (no stack, no literal pool) | DFU after a fault shows red like every other DFU entry (POLICY-RGB) | more code in a fault handler: a second fault there locks the core up (a hang, R13); keep as is unless P3 is taken |
| P6 | `hhkb_modes.c`: on a mode switch with a Fn layer held, move the held layer to the new mode's Fn layer | Fn keys keep working after Fn+Ctrl+M/W without re-pressing Fn (policy §5) | cosmetic; the Classic's behaviour is unknown |
| P8 | L1 (ship key tester): `SECURE_ENABLE` with an unlock sequence instead of `VIA_INSECURE` | the ship key tester works after a deliberate unlock, the matrix stays hidden otherwise | config-only (POLICY-UPSTREAM); not investigated on the device |

L12 stays a limitation: PA1 floats for the bootloader's whole run (its source never touches GPIOA) and becomes a pulled-up input at ChibiOS' `halInit`, microseconds before `keyboard_pre_init_user` drives it; no upstream extension point runs earlier, and a custom board file would be a patched upstream file (POLICY-UPSTREAM). Whether the colour is line noise or the WS2812's own power-up state (which a pull-down would not cure) is unverified: it would show as the same symptom on the stock image, which sends its first frame later.

## Results awaiting a milestone

* Ship acceptance 2026-10-27 (ship releases 8–12): all green; Esc→DFU all red with an identical readback (release 8); selective Command passes; modes, switches and settings display as in dev; reset boot dark before the red blinks (release 10); normal plug-in lights once (release 11); LED 1's power-up flash accepted (L12; release 12 drives the line earlier). Logs: `artifacts/dev_session_2026-10-27_console.log`, `artifacts/ship_acceptance_2026-10-27_usb.txt`.
