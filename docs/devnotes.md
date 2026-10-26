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

1. **Dev image test round** (flash with `scripts/flash.sh flash dev <sha256>`, `operations.md` §2; record results in `firmware.md`):
   - Esc→DFU test and readback compare (as after every flash);
   - Caps Lock LED follows Caps Lock, also after a replug;
   - all 60 keys in VIA's key tester (works in dev thanks to `VIA_INSECURE`), or the console with Shift+Shift+D then X; includes the 7 positions the stock probe missed: `0,7 1,7 2,7 3,7 4,1 4,2 4,7`;
   - Caps Lock trace: Shift+Shift+D, then K, then hold Fn+Tab → Caps stays in the report for the whole hold;
   - RGB channels and colour order: Shift+Shift+D → note which two LEDs turn yellow (channel A); the other two are channel B. Check that red, yellow, magenta, blue and green look right (this also proves the GRB order); adjust `rgb_indicators.h` only if another split reads better (I20);
   - RGB indicators (POLICY-RGB): white base; Shift+Shift+D → yellow on channel A; then K → magenta; layer lock → green on B; blue while unplugged-and-replugged before enumeration; Shift+Shift+B → **red, and does it stay red in DFU** (WS2812 latch, unverified); Bootmagic → orange blinks on the next boot (settings reset);
   - persistence: change a key in VIA, replug, compare `falcon_kalam_probe.py dump-keymap` with `artifacts/release_keymap_baseline.json`, then revert;
   - RGB off while the Mac sleeps;
   - spot-check the dev traces (action, wear-leveling, scan rate) in `qmk console`.
   - Bootmagic (Esc at plug-in) → LEDs **red** in DFU (R15; same WS2812-latch question as Shift+Shift+B);
   - **fault recovery** (R13, POLICY-RECOVERY): Shift+Shift+F → `qmk console` shows "fault test", the keyboard drops into DFU (`dfu-util -l` shows `1eaf:0003`), and a replug brings it back;
   - **wear-leveling** (R2): with Shift+Shift+D on, change several keys in VIA and replug a few times; the console shows backing-store writes, and the changes persist;
   - **EEPROM move** (4 KB at `0x0800F000`): the first boot shows orange blinks (settings reset by the schema stamp); VIA then shows 6 layers and the macro buffer; the console wear-leveling trace shows no errors;
   - **HHKB modes** (POLICY-HHKB): Mac mode at first boot; Fn+F Eject, Fn+Delete Clear, Fn+Return keypad Enter, Fn+A/S/D volume, Fn+H/J/N/M keypad; **Fn+Esc Power: note what macOS does** (Keyboard Power `0x66`; expect a shut-down/sleep prompt, so cancel it); Fn+Ctrl+W → Win mode (Delete deletes forward, Fn+Delete = Backspace, Fn+A types a); Fn+Ctrl+H → HHK mode (◇ is no longer Command, Fn+Tab types Tab, Fn+Right-◇ = Stop in the console trace); Fn+Ctrl+M → Mac; the mode survives a replug; Bootmagic → Mac mode; Shift+Shift+left-◇ in HHK mode types normally (ship)
2. **Ship image acceptance**: flash it (`scripts/flash.sh flash ship <sha256>`), run the Esc→DFU test and readback, then check the selective Command (H D X K V S B work; Shift+Shift+Backspace, Esc, Caps, 1, F1 and N type normally) and the RGB policy (white at full brightness, indicators as in dev, Fn+Z–B type nothing special, lighting menu absent). Keep it as the daily image.

## Before shipping (leaving alpha)

* the ship image passes its acceptance test (To do 2);
* all hardware tests in To do 1 passed;
* FLASHREAD is already excluded from ship by POLICY-VARIANTS (enforced by the build).

## In progress

* none

## Results awaiting a milestone

* Image `9b61141e…`: the console connects (`artifacts/functional_console_2026-10-26.log`), but no debug traces have been captured yet. Everything else from that session is recorded in `firmware.md` (✓) or `problems.md` (B1).
