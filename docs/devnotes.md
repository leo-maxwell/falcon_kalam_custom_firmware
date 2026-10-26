# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | dev release 4 (`531c45ce…`, R/G/B-only indicators), flashed 2026-10-27. Before it on the same day: releases 1–3, each read back equal to its image |
| Next to flash | dev release 5 (`docs/firmware.md` §1: Bootmagic red sent twice) at the session's next DFU entry (step 18), so step 19 tests it |
| Then | **ship** as the acceptance test, after the dev session. Each flash needs an approval naming the variant and its hash |
| Assembly | being reassembled; the pads are no longer needed (Esc→DFU works) |
| Phase | **alpha** |

## To do

1. **Dev image test session** (about 30 min, 7 plug-ins; record results in `firmware.md`). Colours: `policy.md` §2.1. Ordered so that each plug-in and each DFU entry serves several checks. A console log (`qmk console`) and a USB log (`tools/usb_watch.py`) run in the background throughout. **U** = done at the keyboard, **C** = done at the computer. Debug output stays on from step 6 to step 15 (Shift+Shift+D, then K).
   - **A. Flash** (plug-in 1)
     1. U: unplug, hold Esc, plug in (the old image stays dark in DFU). C: `scripts/flash.sh flash dev <sha256>`.
     2. U: the new image starts by itself: the reset blink (schema reset after the EEPROM move), then the normal base. C: `falcon_kalam_probe.py info` → the release in `firmware.md`, VIA `0x000D`, 6 layers, 16 macros / 1,107 B.
   - **B. Esc→DFU test** (plug-ins 2–3; required after every flash)
     3. U: unplug, hold Esc, plug in → **red, and does it stay red in DFU?** (Bootmagic red, R15). C: `scripts/flash.sh verify dev` (readback = image).
     4. U: replug normally → the reset blink (Bootmagic cleared the settings), then the normal base; note the not-configured indicator before enumeration.
   - **C. One debug pass in HHK mode, no replug**
     5. U: Shift+Shift+D → channel A shows debug: **which two LEDs?** (= channel A, I20). Then K → channel A shows keystroke printout.
     6. U: press every key once, row by row (60). C: all positions arrive, including `0,7 1,7 2,7 3,7 4,1 4,2 4,7`; HHK base codes (Delete = Delete, ◇ = Muhenkan/Henkan).
     7. U: hold Fn, press every key once. C: HHK Fn layer (F-keys, Ins/Del, PrtSc/ScrLk/Pause, arrows, Home/End/PgUp/PgDn, Fn+Delete = Backspace, Fn+Right-◇ = Stop, the rest plain).
     8. U: Fn+Ctrl+Shift+0 → HHK colour for 1.5 s on **one LED: which?** (= "LED 1"), the other three off; no second phase. Fn+Ctrl+W → the display again, still HHK (SW1/SW2 select HHK).
     9. U: Fn+Ctrl+M → display: Mac now, then HHK after a replug. Fn+Tab → Caps Lock on, **Caps Lock LED lit?** (B1); hold Fn+Tab 2 s (C: Caps stays in the report); Fn+Tab again → off. Fn+A/S/D (volume overlay), Fn+F (eject overlay), Fn+Delete (Clear), Fn+Return (keypad Enter), Fn+H/J/N/M (keypad). **Fn+Esc: note what macOS does** (Power `0x66`), then cancel (L2).
     10. U: Fn+Ctrl+Shift+0 → Mac, an off gap, then HHK (after a replug the switches give HHK).
     11. U: Fn+Ctrl+Shift+2, 3, 4, 5 → two off-blinks each, mode unchanged. C: wear-leveling writes in the log (R2). Fn+Ctrl+Shift+0 → Mac with LEDs 2–4 off, then Mac with LEDs 2–4 blue.
     12. U: hold a key while pressing Fn+Ctrl+M: no stuck key.
     13. U (optional, VIA at hand): change one key in VIA. C: `dump-keymap` before and after.
     14. U: put the Mac to sleep → LEDs off; press a key → the Mac wakes (no SW6), the normal base again.
     15. U: Shift+Shift+1 → **layer-locked on the other two LEDs** (channel B; dev Command makes layer 1 the default layer). The keymap is now odd until the replug.
   - **D. Switches after a replug** (plug-in 4)
     16. U: replug normally: no reset blink (settings kept), Mac mode (SW2). Shift+Shift+D, K. Fn+Ctrl+Shift+0 → Mac + three blue, one phase. C: SW4 left ◇ acts as Fn (left-◇+1 = F1); SW5 left Alt sends Command, right ◇ sends Alt; SW3 leaves Mac's Delete = Backspace. Fn+Ctrl+W → display (Win), Win: Delete = Backspace (SW3), Fn+A types a. Optional VIA change from step 13 persisted.
   - **E. Recovery paths** (plug-ins 5–7, ends normal)
     17. U: Shift+Shift+F. C: "fault test" in the log, `1EAF:0003` appears (R13); LEDs keep their last colours.
     18. U: replug normally (plug-in 5) → app back. Shift+Shift+B → **red, and does it stay red in DFU?** (R15, Command path). C: `dfu-util -l`.
     19. U: unplug, hold Esc, plug in (plug-in 6), then replug normally (plug-in 7) → the reset blink, factory switches (HHK).

   Not covered by this session: long-term wear-leveling (R2: watch over weeks), the ship acceptance (To do 2).
2. **Ship image acceptance.** Flash it (`scripts/flash.sh flash ship <sha256>`), run the Esc→DFU test and readback, then check:
   - selective Command: H D X K V S B work; Shift+Shift+Backspace, Esc, Caps, 1, F1 and N type normally, and in HHK mode so does Shift+Shift+left-◇;
   - POLICY-HHKB as in dev (modes and virtual DIP switches);
   - RGB: green at the cap, indicators as in dev, Fn+Z–B do nothing special, no lighting menu in VIA.

   Then keep ship as the daily image.

## Before shipping (leaving alpha)

* the ship image passes its acceptance test (To do 2);
* every hardware test in To do 1 passed.

FLASHREAD is already out of ship (POLICY-VARIANTS, enforced by the build).

## In progress

* none

## Results awaiting a milestone

* Session 2026-10-27, steps 1–7 done (dev releases 1–4). Release 4: green base, channel A blue (debug from boot); at plug-in the LEDs light, go off and come back once (USB suspend/resume during enumeration, `RGBLIGHT_SLEEP`). Key passes (steps 6–7, `artifacts/dev_session_2026-10-27_keycheck.txt`): all 60 base positions with the HHK codes, all 27 HHK Fn keys incl. Stop `0x78`, no mismatch. Shift+Shift+B (release 3): all four red in DFU. Earlier (dev releases 1–3; release 3 entered DFU by Esc): flash, enumeration, probe (release, 6 layers, 1,107 B macros) and both readbacks OK. Debug is on from boot in dev (QMK's `DEBUG_MATRIX_SCAN_RATE` sets it), so channel A shows yellow at once: **channel A = LEDs 1–2** (counted left to right); LED 4 white, LED 3's colour hard to judge next to the yellow (check with debug off). In DFU: LED 1 yellow, LEDs 2–4 red (R15). At plug-in LED 1 lights a moment before the others (Inferred: the same undriven-line noise). No orange after the DFU entry (B5). Scan rate ≈ 3,677 Hz. Brightness 127 still too bright → release 3 (63, comfortable). Mixed colours (white, yellow, magenta, …) blend poorly on this module → release 4 uses pure red, green and blue only. Logs: `artifacts/dev_session_2026-10-27_console.log`, `artifacts/dev_session_2026-10-27_usb.txt`.
* Image `9b61141e…`: the console connects (`artifacts/functional_console_2026-10-26.log`); no debug traces captured yet. Everything else from that session is recorded in `firmware.md` (✓) or `problems.md` (B1).
* `artifacts/release_keymap_baseline.json` is a 4-layer dump of image `9b61141e…`; it does not apply to the 6-layer images (the persistence test makes its own dumps).
