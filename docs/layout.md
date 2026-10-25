# OPEN ISSUE L-1: physical layout not settled

**Status: OPEN.** This stays open until the user explicitly records a
settlement in the "Settlement" section below. The firmware's HHKB default is
a design choice. It does **not** settle this issue.

## What is known

| Source | Says |
|---|---|
| Vendor VIA JSON | 63 matrix positions; HHKB base; options: split backspace (`0,13`+`0,14` vs 2u `0,14`), split left shift (`3,0`+`3,1`), split spacebar (`4,5`/`4,7`/`4,9`) |
| Vendor compiled keymap | also assigns `4,0` LCtrl, `4,13` RCtrl and `4,14` KC_NO; `4,11` RAlt, `4,12` MO(1); Caps position `LT(1,Caps)` |
| Vendor EEPROM | erased by Bootmagic before the backup, so no record of how the user's unit was configured |
| Hardware | never measured |

## What must be determined

1. Which matrix position each physical key produces: all keys, including
   the bottom row and the 1.5u blanks on either side of it (`4,0`, `4,13`, `4,14`?).
2. Which variants this unit has: backspace (HHKB split or 2u), left shift
   (2.25u or split), spacebar (7u or 3-1-3). These are physical keycaps and
   switches on a hot-swap PCB.
3. Whether any switch position is unpopulated or doubled.

## How to settle it (read-only, no flashing)

The manufacturer firmware is still on the keyboard. Unplug and replug it
normally (do not hold Esc), then:

```bash
tools/falcon_probe.py info
```

```bash
tools/falcon_probe.py matrix --log artifacts/layout_probe.csv
```

Press every key once, left to right and row by row, then Ctrl-C. The probe
only sends VIA read queries (enforced by a whitelist).

If `info` reports the matrix query as unsupported, or no presses appear (newer
QMK returns zeros unless built with `VIA_INSECURE`), use the
**VIA Key Tester** in the VIA app with `originals/manufacturer_jsonfile.json`.
It relies on the same query. Failing that, the matrix can only be read after
flashing our firmware (a matrix-test build would be needed, under a separate
approval).

## Settlement (to be filled in by the user)

| | |
|---|---|
| Date / method | |
| Backspace | HHKB split / 2u |
| Left shift | 2.25u / split |
| Spacebar | 7u / 3-1-3 |
| Positions observed (paste probe output) | |
| Unexpected positions (e.g. `4,0`, `4,13`, `4,14`) | |
| Settled by (user) | |

After settlement: adjust `LAYOUT_all` / keymaps if needed, and turn R5 in
`docs/risks.md` into "closed".
