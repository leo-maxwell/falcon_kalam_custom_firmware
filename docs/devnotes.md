# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the relevant fact into `docs/hardware.md`) and
delete it here.

## Device state

| | |
|---|---|
| On the device | `9b61141e383bfd5aad74f66c48be2b40cf2decc0073225454897d1dde117de63`, flashed 2026-10-26: a pre-variant build (FLASHREAD, full Command, no Caps Lock LED) |
| Next to flash | proposed: the **dev** image (`docs/firmware.md` §1) for the remaining tests, then the **ship** image as the acceptance test. Needs an approval naming variant and hash. Not urgent (user, 2026-10-26) |
| Assembly | the keyboard is being reassembled; the pads are no longer needed (Esc→DFU works) |
| Phase | **alpha** |

## To do

1. **Dev image test round** (flash per `operations.md` §2, record results in `firmware.md`):
   - Esc→DFU test and readback compare (as after every flash);
   - Caps Lock LED follows Caps Lock, also after a replug;
   - all 60 keys in VIA's key tester (works in dev thanks to `VIA_INSECURE`), or the console with Shift+Shift+D then X; includes the 7 positions the stock probe missed: `0,7 1,7 2,7 3,7 4,1 4,2 4,7`;
   - Caps Lock trace: Shift+Shift+D, then K, then hold Fn+Tab → Caps stays in the report for the whole hold;
   - RGB hardware: **LED-order test** (Shift+Shift+L steps LED 0→3; note where each lights; then map channels A/B in `rgb_indicators.h`); GRB order via RGB test mode (Fn+X to it, or VIA lighting menu with `via/falcon_hs_r2_dev.json`);
   - RGB indicators (POLICY-RGB): white base; Shift+Shift+D → yellow on channel A; then K → magenta; layer lock → green on B; blue while unplugged-and-replugged before enumeration; Shift+Shift+B → **red, and does it stay red in DFU** (WS2812 latch, unverified); Bootmagic → orange blinks on the next boot (settings reset);
   - persistence: change a key in VIA, replug, compare `falcon_probe.py dump-keymap` with `artifacts/release_keymap_baseline.json`, then revert;
   - RGB off while the Mac sleeps;
   - spot-check the dev traces (action, wear-leveling, scan rate) in `qmk console`.
2. **Ship image acceptance**: flash it, run the Esc→DFU test and readback, then check the selective Command (H D X K V S B work; Shift+Shift+Backspace, Esc, Caps, 1, F1 and N type normally) and the RGB policy (white at full brightness, indicators as in dev, Fn+Z toggles the white base, Fn+X–B do nothing, effects and lighting menu absent). Keep it as the daily image.

## Before shipping (leaving alpha)

* the ship image passes its acceptance test (To do 2);
* all hardware tests in To do 1 passed;
* FLASHREAD is already excluded from ship by POLICY-VARIANTS (enforced by the build).

## In progress

* none

## Results awaiting a milestone

* Image `9b61141e…`: the console connects (`artifacts/functional_console_2026-10-26.log`), but no debug traces have been captured yet. Everything else from that session is recorded in `firmware.md` (✓) or `problems.md` (B1).
