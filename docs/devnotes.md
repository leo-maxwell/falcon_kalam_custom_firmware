# Development notes (status)

The only status document. When an item reaches a milestone, write its result
into `docs/firmware.md` (or the fact into `docs/hardware.md`) and delete it
here.

## Device state

| | |
|---|---|
| On the device | **dev** release 17 (`33e2336f…`), flashed 2026-10-27 for the release validation; dev round V1–V9 passed on releases 15–17 (dim level, cap and the Caps LED delay were changed on the way). Ship release 17 is built and not yet flashed. Releases 13 and 16 ship were never flashed. Before it on the same day: dev releases 1–4, 6, 7 and ship releases 8–11, each read back equal to its image (release 5 was never flashed) |
| Recovery | Esc at plug-in reaches DFU on every image tested; the pads are no longer needed |
| Note | Release 10 entered DFU 6 s after its flash, without an unplug: that was a deliberate Shift+Shift+B, not a crash. No image has entered DFU on its own |
| Phase | **alpha** |

## To do

1. **Release validation, dev image: done** (releases 15–17, 2026-10-27; V1–V9 all passed; log `artifacts/release_validation_2026-10-27_console.log`; V2: 60/60 keys and 27 Fn keys, 0 mismatches). Kept here until the ship round closes the milestone. The console is captured to `artifacts/release_validation_2026-10-27_console.log`; each step names its pass criterion.
   - V1 probe (tool): `info` reports release 15, 6 layers, 1,107 B macros; `dump-keymap` equals the compiled six layers; `dump-bootloader` writes a 20-byte `fsize_uid` file and the bootloader equals `originals/device_dump_2026-10-26/` (B12).
   - V2 key pass (keys): every key of the base layer once, then every Fn combination once, in HHK mode; Shift+Shift+D on first. Pass: `tools/console_keycheck.py` reports 60 of 60 base positions, the Fn positions, 0 mismatches.
   - V3 modes (keys): Fn+Ctrl+M, type Delete and ◇ (Backspace, Command), Fn+A/S/D/F/Esc/Return; Fn+Ctrl+W returns to HHK (the switches select HHK). Win mode needs SW1: Fn+Ctrl+Shift+1 (two off-blinks), replug, LED 1 green on Fn+Ctrl+Shift+0; type Delete, ◇ (Windows key), Fn+Tab (Caps Lock); Fn+Ctrl+M then Fn+Ctrl+W switches Mac and back to Win; SW1 off, replug. Pass: the host shows each mode's keys; the settings display shows red/blue/green on LED 1.
   - V4 switches (keys): Fn+Ctrl+Shift+3 (two off-blinks), Fn+Ctrl+Shift+0 shows HHK now, then after a dark gap HHK + SW3 blue on LED 2; replug: Delete sends Backspace, display shows SW3 only; flip SW3 off (one off-blink), SW4 on, replug: left ◇ acts as Fn (◇+1 = F1); SW5 on, replug: Alt and ◇ swapped; flip both off, replug: factory behaviour. Pass: each as stated; settings survive the replugs (R2 closed).
   - V5 lights (keys, dim room): Shift+Shift+L runs the light test; pass: every named state appears as named in `manual.md` §3, the three red blinks on dark, and the base returns green. Plug-in: dark then green, no LED 1 flash (B13). Shift+Shift+D / K: LEDs 1–2 blue / red; off again. Host sleep: all dark; wake: green.
   - V6 Command (keys): Shift+Shift+H lists QMK's commands (F and L are extras outside that list); V, S print; Shift+Shift+6 types 6 (B10); Shift+Shift+N, Caps, Z type normally.
   - V7 Caps Lock LED (keys): on macOS a long Fn+Tab toggles Caps Lock and the LED; a short one switches the input source.
   - V8 fault test (keys): Shift+Shift+F: console line, then all four LEDs keep their colour and the keyboard is in DFU (`1EAF:0003`); `scripts/flash.sh verify dev` (tool) equals the image. Replug: the app starts.
   - V9 Bootmagic (keys): unplug, hold Esc, plug in: all red, no yellow; `scripts/flash.sh verify dev` (tool); replug: three red blinks on dark, then green; switches back to factory (HHK).
2. **Release validation, ship image**: flash release 15 ship (`firmware.md` §1), then V1 (`info` only; `dump-keymap`), V2 abbreviated (one key per row, Fn row), V3, V4 (SW3 only), V5 plug-in and D/K only (no light test in ship), V6 (H D X K V S B work; Esc, Backspace, 1, F1, N, left ◇ type normally; Shift+Shift+6 types 6), V9, and the VIA app: loads `via/falcon_kalam.json`, connects, shows six layers and no lighting menu, the key tester shows nothing (L1). Pass = ship is the daily image, the project leaves alpha, and `firmware.md` §2 is updated from these results.
3. **Decide the remaining review proposals** below (each changes firmware behaviour).

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
