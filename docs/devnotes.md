# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | `9b61141e383bfd5aad74f66c48be2b40cf2decc0073225454897d1dde117de63`, flashed 2026-10-26: a pre-variant build (FLASHREAD, full Command, no Caps Lock LED, one HHKB Fn layer instead of the three modes) |
| Next to flash | the **dev** image (`docs/firmware.md` §1) for the test round below, then **ship** as the acceptance test; no hurry. Each flash needs an approval naming the variant and its hash |
| Assembly | being reassembled; the pads are no longer needed (Esc→DFU works) |
| Phase | **alpha** |

## To do

1. **Dev image test round.** Flash with `scripts/flash.sh flash dev <sha256>` (`operations.md` §2); record the results in `firmware.md`.
   - **After every flash:** Esc→DFU test and readback compare.
   - **EEPROM move** (4 KB at `0x0800F000`): the first boot shows orange blinks (schema-stamp reset); VIA shows 6 layers and the 1,107 B macro buffer; the wear-leveling trace shows no errors.
   - **Keys:** all 60 in VIA's key tester (dev has `VIA_INSECURE`) or the console (Shift+Shift+D, then X), including the 7 positions the stock probe missed: `0,7 1,7 2,7 3,7 4,1 4,2 4,7`.
   - **Caps Lock:** the LED follows Caps Lock, also after a replug (B1); with Shift+Shift+D then K, holding Fn+Tab keeps Caps in the report for the whole hold.
   - **HHKB modes** (POLICY-HHKB): Mac mode at first boot. Mac Fn: F Eject, Delete Clear, Return keypad Enter, A/S/D volume, H/J/N/M keypad. **Fn+Esc Power: note what macOS does** (Keyboard Power `0x66`; expect a shut-down or sleep prompt, and cancel it). Fn+Ctrl+W → Win (Delete deletes forward, Fn+Delete = Backspace, Fn+A types a). Fn+Ctrl+H → HHK (◇ is not Command, Fn+Tab types Tab, Fn+Right-◇ = Stop in the console trace). Fn+Ctrl+M → Mac. The mode survives a replug; Bootmagic returns to Mac.
   - **RGB** (POLICY-RGB): white base. Shift+Shift+D → yellow on channel A; note which two LEDs that is (the other two are B; adjust `rgb_indicators.h` only if another split reads better, I20). Then K → magenta. A locked layer → green on B. Blue between plug-in and enumeration. Orange blinks after Bootmagic. Red, yellow, magenta, blue and green look right (this proves the GRB order). Off while the Mac sleeps.
   - **Red in DFU** (R15): after Shift+Shift+B and after Bootmagic, do the LEDs stay red in DFU (WS2812 latch, unverified)?
   - **Fault recovery** (R13): Shift+Shift+F → `qmk console` shows "fault test", the keyboard drops into DFU (`dfu-util -l` shows `1eaf:0003`), and a replug brings it back.
   - **Persistence and wear-leveling** (R2): save `falcon_kalam_probe.py dump-keymap` output, change several keys in VIA with Shift+Shift+D on, replug a few times; the console shows backing-store writes, a new dump shows the changes, and reverting restores the first dump.
   - **Dev traces:** spot-check key actions, wear-leveling and scan rate in `qmk console`.
2. **Ship image acceptance.** Flash it (`scripts/flash.sh flash ship <sha256>`), run the Esc→DFU test and readback, then check:
   - selective Command: H D X K V S B work; Shift+Shift+Backspace, Esc, Caps, 1, F1 and N type normally, and in HHK mode so does Shift+Shift+left-◇;
   - RGB: white at full brightness, indicators as in dev, Fn+Z–B do nothing special, no lighting menu in VIA.

   Then keep ship as the daily image.

## Before shipping (leaving alpha)

* the ship image passes its acceptance test (To do 2);
* every hardware test in To do 1 passed.

FLASHREAD is already out of ship (POLICY-VARIANTS, enforced by the build).

## In progress

* none

## Results awaiting a milestone

* Image `9b61141e…`: the console connects (`artifacts/functional_console_2026-10-26.log`); no debug traces captured yet. Everything else from that session is recorded in `firmware.md` (✓) or `problems.md` (B1).
* `artifacts/release_keymap_baseline.json` is a 4-layer dump of image `9b61141e…`; it does not apply to the 6-layer images (the persistence test makes its own dumps).
