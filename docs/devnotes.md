# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | **ship** release 18 (`df75f34c…`), flashed 2026-10-27 (probe: release 18). Release 18 changes only the dev light test (B14), so the ship acceptance of release 17 carries over; the post-flash Esc→DFU test and `scripts/flash.sh verify ship` are the remaining step |
| Recovery | Esc at plug-in reaches DFU on every image tested; the pads are no longer needed |
| Phase | **released** (2026-10-27): release 18, tag `release-18`, is the first official release and the daily image; every further change bumps the release and repeats `operations.md` §2 |

## To do

1. **Decide the remaining review proposals** below (each changes firmware behaviour).

## In progress

* none

## Review proposals (2026-10-27, awaiting a decision; each is a behaviour change). P1, P2, P5 and P7 went into release 13 (B10–B12).

| # | Proposal | Change | Why |
|---|---|---|---|
| P3 | L7: fault counter in a spare backup register (DR1–DR9 are unused): first fault → `0x424D` (the app restarts), a second fault within ~10 s of boot → `0x424C` (DFU); the counter is cleared from housekeeping after a healthy start | a transient fault no longer silences the keyboard until a replug; a crash loop still parks in DFU | handler stays stack-free (immediates only); needs the dev fault test on the device |
| P4 | L7 LEDs: paint red from the fault handler with a register-only WS2812 bit-bang (no stack, no literal pool) | DFU after a fault shows red like every other DFU entry (POLICY-RGB) | more code in a fault handler: a second fault there locks the core up (a hang, R13); keep as is unless P3 is taken |
| P6 | `hhkb_modes.c`: on a mode switch with a Fn layer held, move the held layer to the new mode's Fn layer | Fn keys keep working after Fn+Ctrl+M/W without re-pressing Fn (policy §5) | cosmetic; the Classic's behaviour is unknown |
| P8 | problem L1 (ship key tester): `SECURE_ENABLE` with an unlock sequence instead of `VIA_INSECURE` | the ship key tester works after a deliberate unlock, the matrix stays hidden otherwise | config-only (POLICY-UPSTREAM); not investigated on the device |

