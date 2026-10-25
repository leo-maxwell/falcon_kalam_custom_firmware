# ISSUE L-1: physical layout — SETTLED (2026-10-26)

**Settlement (by the user):** "my board has physically the same key layout as
HHKB classic". The unit is a 60-key HHKB classic: split `\` + `` ` `` top right,
1.5u Backspace on the Tab row, 2.25u left Shift (no split), Fn right of right
Shift, bottom row Alt · ◆ · Space · ◆ · Alt, blank corners.
→ Release keymap uses **`LAYOUT_60_hhkb`**.

## Evidence

Read-only matrix probe on the stock firmware (`tools/falcon_probe.py matrix`).
The user pressed every key once, row by row, left to right.

* Raw log: `artifacts/layout_probe.csv`. Decoded: `artifacts/layout_probe_decoded.txt`.
* The probe version used for the recording had a decoder bug: protocol-12
  firmware echoes a row-offset byte, so the matrix data starts at byte 3, not 2.
  Every reading was shifted by 8 columns across a row boundary, and real
  column 7 and real row 4 columns 0–7 were invisible. The fixed decoder is in
  `falcon_probe.py`. `falcon_probe.py decode-log` re-maps the recorded log
  exactly (`legacy_to_actual()` explains the mapping).
* Result: **53 presses, in strict press order, at exactly the
  `LAYOUT_60_hhkb` positions** except the 7 keys the old decoder could not see:
  `0,7 1,7 2,7 3,7` (7 U J M) and `4,1 4,2 4,7` (left Alt, left ◆, Space).
  53 + 7 = 60. No position outside `LAYOUT_60_hhkb` fired: `3,1`, `4,0`,
  `4,13`, `4,14` and `1,13` stayed silent.

| Row | Probed positions (decoded) | Not visible to the old decoder |
|---|---|---|
| 0 | 0,0–0,6, 0,8–0,14 | 0,7 |
| 1 | 1,0–1,6, 1,8–1,12, 1,14 | 1,7 |
| 2 | 2,0–2,6, 2,8–2,11, 2,14 | 2,7 |
| 3 | 3,0, 3,2–3,6, 3,8–3,12, 3,14 | 3,7 |
| 4 | 4,11, 4,12 | 4,1, 4,2, 4,7 |

The 7 unseen positions follow from the settlement and the vendor VIA JSON.
They are checked again on hardware in the first flash test (checklist §C5).

## Consequences

* The base keeps `LAYOUT_all` (63 VIA positions), because the PCB supports the split variants.
* The release keymap is `LAYOUT_60_hhkb`. Unused matrix positions are `KC_NO` and assignable in VIA.
* `via/falcon_hs_r2.json` already opens on HHKB (layout option 0).
* QMK/VIA draw the HHKB space as 7u; the real cap may differ. This is cosmetic only.
