# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the relevant fact into `docs/hardware.md`) and
delete it here.

## Device state

| | |
|---|---|
| On the device | `9b61141e383bfd5aad74f66c48be2b40cf2decc0073225454897d1dde117de63`, flashed 2026-10-26; no Caps Lock LED support |
| Next to flash | the current release in `docs/firmware.md` §1 (adds the Caps Lock LED). Not urgent (user, 2026-10-26) |
| Assembly | the keyboard is being reassembled; the pads are no longer needed (Esc→DFU works) |

## To do

1. **Flash the current release** (`operations.md` §2). Then: Esc→DFU test, readback compare, Caps Lock LED follows Caps Lock (also after a replug).
2. **Remaining hardware tests** (on the next image; record results in `firmware.md`):
   - console + Command: Shift+Shift+D, then Shift+Shift+X, then press every key → all 60 positions (the 7 the stock probe missed: `0,7 1,7 2,7 3,7 4,1 4,2 4,7`);
   - Caps Lock trace: Shift+Shift+D, then Shift+Shift+K, then hold Fn+Tab → Caps stays in the report for the whole hold;
   - RGB: Fn+Z/X, a red/green/blue check in a static mode (GRB order), the VIA lighting menu;
   - persistence: change a key in VIA, replug, compare with `artifacts/release_keymap_baseline.json` (`falcon_probe.py dump-keymap`), then revert;
   - RGB off while the Mac sleeps;
   - LShift+RShift+B → DFU.
3. **Decide** R12 (console + Command in the daily image) and R10 (keep or remove FLASHREAD now that the dump exists); see `problems.md`.

## In progress

* none

## Results awaiting a milestone

* Image `9b61141e…`: the console connects (`artifacts/functional_console_2026-10-26.log`), but no debug traces have been captured yet. Everything else from that session is recorded in `firmware.md` (✓) or `problems.md` (B1).
