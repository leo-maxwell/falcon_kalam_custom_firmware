# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | dev release 2, `3621ae1ef0893fdf14a34aff759f04b3f1cace3f1e9b94c31cfb54a531e2766e`, flashed 2026-10-27 (readback = image); release 1 (`485d3e67…`) ran before it, readback = image |
| Next to flash | dev release 3 (`docs/firmware.md` §1: quarter brightness, B5 fix) to continue the session, then **ship** as the acceptance test. Each flash needs an approval naming the variant and its hash |
| Assembly | being reassembled; the pads are no longer needed (Esc→DFU works) |
| Phase | **alpha** |

## To do

1. **Dev image test session** (about 30 min, 7 plug-ins; record results in `firmware.md`). Ordered so that each plug-in and each DFU entry serves several checks. A console log (`qmk console`) and a USB log (`tools/usb_watch.py`) run in the background throughout. **U** = done at the keyboard, **C** = done at the computer. Debug output stays on from step 6 to step 15 (Shift+Shift+D, then K).
   - **A. Flash** (plug-in 1)
     1. U: unplug, hold Esc, plug in (the old image stays dark in DFU). C: `scripts/flash.sh flash dev <sha256>`.
     2. U: the new image starts by itself: orange ×3 (schema reset after the EEPROM move), then white. C: `falcon_kalam_probe.py info` → the release in `firmware.md`, VIA `0x000D`, 6 layers, 16 macros / 1,107 B.
   - **B. Esc→DFU test** (plug-ins 2–3; required after every flash)
     3. U: unplug, hold Esc, plug in → **red, and does it stay red in DFU?** (Bootmagic red, R15). C: `scripts/flash.sh verify dev` (readback = image).
     4. U: replug normally → orange ×3 (Bootmagic cleared the settings), white; note any blue flash before enumeration.
   - **C. One debug pass in HHK mode, no replug**
     5. U: Shift+Shift+D → yellow: **which two LEDs?** (= channel A, I20). Then K → magenta on the same two.
     6. U: press every key once, row by row (60). C: all positions arrive, including `0,7 1,7 2,7 3,7 4,1 4,2 4,7`; HHK base codes (Delete = Delete, ◇ = Muhenkan/Henkan).
     7. U: hold Fn, press every key once. C: HHK Fn layer (F-keys, Ins/Del, PrtSc/ScrLk/Pause, arrows, Home/End/PgUp/PgDn, Fn+Delete = Backspace, Fn+Right-◇ = Stop, the rest plain).
     8. U: Fn+Ctrl+Shift+0 → purple for 1.5 s on **one LED: which?** (= "LED 1"), the other three dark; no second phase. Fn+Ctrl+W → purple flash, still HHK (SW1/SW2 select HHK).
     9. U: Fn+Ctrl+M → pink flash. Fn+Tab → Caps Lock on, **Caps Lock LED lit?** (B1); hold Fn+Tab 2 s (C: Caps stays in the report); Fn+Tab again → off. Fn+A/S/D (volume overlay), Fn+F (eject overlay), Fn+Delete (Clear), Fn+Return (keypad Enter), Fn+H/J/N/M (keypad). **Fn+Esc: note what macOS does** (Power `0x66`), then cancel (L2).
     10. U: Fn+Ctrl+Shift+0 → pink, then a dark gap, then purple (after a replug the switches give HHK).
     11. U: Fn+Ctrl+Shift+2, 3, 4, 5 → two dark blinks each, mode unchanged. C: wear-leveling writes in the log (R2). Fn+Ctrl+Shift+0 → pink, then pink with LEDs 2–4 green.
     12. U: hold a key while pressing Fn+Ctrl+M: no stuck key.
     13. U (optional, VIA at hand): change one key in VIA. C: `dump-keymap` before and after.
     14. U: put the Mac to sleep → LEDs off; press a key → the Mac wakes (no SW6), LEDs white.
     15. U: Shift+Shift+1 → **green on the other two LEDs** (channel B; dev Command makes layer 1 the default layer). The keymap is now odd until the replug.
   - **D. Switches after a replug** (plug-in 4)
     16. U: replug normally: no orange (settings kept), Mac mode (SW2). Shift+Shift+D, K. Fn+Ctrl+Shift+0 → pink + three green, one phase. C: SW4 left ◇ acts as Fn (left-◇+1 = F1); SW5 left Alt sends Command, right ◇ sends Alt; SW3 leaves Mac's Delete = Backspace. Fn+Ctrl+W → gold flash, Win: Delete = Backspace (SW3), Fn+A types a. Optional VIA change from step 13 persisted.
   - **E. Recovery paths** (plug-ins 5–7, ends normal)
     17. U: Shift+Shift+F. C: "fault test" in the log, `1EAF:0003` appears (R13); LEDs keep their last colours.
     18. U: replug normally (plug-in 5) → app back. Shift+Shift+B → **red, and does it stay red in DFU?** (R15, Command path). C: `dfu-util -l`.
     19. U: unplug, hold Esc, plug in (plug-in 6), then replug normally (plug-in 7) → orange ×3, factory switches (HHK).

   Not covered by this session: long-term wear-leveling (R2: watch over weeks), the ship acceptance (To do 2).
2. **Ship image acceptance.** Flash it (`scripts/flash.sh flash ship <sha256>`), run the Esc→DFU test and readback, then check:
   - selective Command: H D X K V S B work; Shift+Shift+Backspace, Esc, Caps, 1, F1 and N type normally, and in HHK mode so does Shift+Shift+left-◇;
   - POLICY-HHKB as in dev (modes and virtual DIP switches);
   - RGB: white at the cap, indicators as in dev, Fn+Z–B do nothing special, no lighting menu in VIA.

   Then keep ship as the daily image.

## Before shipping (leaving alpha)

* the ship image passes its acceptance test (To do 2);
* every hardware test in To do 1 passed.

FLASHREAD is already out of ship (POLICY-VARIANTS, enforced by the build).

## In progress

* none

## Results awaiting a milestone

* Session 2026-10-27, so far (dev releases 1–2): flash, enumeration, probe (release, 6 layers, 1,107 B macros) and both readbacks OK. Debug is on from boot in dev (QMK's `DEBUG_MATRIX_SCAN_RATE` sets it), so channel A shows yellow at once: **channel A = LEDs 1–2** (counted left to right); LED 4 white, LED 3's colour hard to judge next to the yellow (check with debug off). In DFU: LED 1 yellow, LEDs 2–4 red (R15). At plug-in LED 1 lights a moment before the others (Inferred: the same undriven-line noise). No orange after the DFU entry (B5). Scan rate ≈ 3,677 Hz. Brightness 127 still too bright → release 3. Logs: `artifacts/dev_session_2026-10-27_console.log`, `artifacts/dev_session_2026-10-27_usb.txt`.
* Image `9b61141e…`: the console connects (`artifacts/functional_console_2026-10-26.log`); no debug traces captured yet. Everything else from that session is recorded in `firmware.md` (✓) or `problems.md` (B1).
* `artifacts/release_keymap_baseline.json` is a 4-layer dump of image `9b61141e…`; it does not apply to the 6-layer images (the persistence test makes its own dumps).
