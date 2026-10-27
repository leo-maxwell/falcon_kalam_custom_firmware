# Our firmware: features

At a glance: QMK 0.34.7 firmware for this GD32F103 unit that copies the HHKB
Professional Classic (three setting modes, its HID codes, mode keys and
virtual DIP switches; HHK mode from the factory), keeps every stock feature
(VIA, console, selective Command, Bootmagic), adds crash-to-DFU recovery and
a red/green/blue status light, and leaves NKRO off for macOS. Two images,
ship (daily) and dev (diagnosis), are bit-reproducible and verified offline
on every build; nearly every feature is also verified on the device (§2).

## 1. Current images (POLICY-VARIANTS)

Built from one tree: QMK 0.34.7 (`180f394b`) + `keyboards/falcon_kalam` (L1)
+ `userspace/` (L2: shared `users/falcon_kalam/` plus one keymap per
variant). Both images are reproducible on any day (`SOURCE_DATE_EPOCH` =
the pinned QMK commit time). **Release 18** (`VIA_FIRMWARE_VERSION`, shown by
`falcon_kalam_probe.py info`; bumped whenever the images change). The table's
layout is parsed by `scripts/flash.sh` and `scripts/check_docs.py` (variant in
bold, then file, hash and size); keep it.

| Variant | Image | SHA-256 | Size (of 53,248 B) | Use |
|---|---|---|---|---|
| **ship** | `artifacts/falcon_kalam_ship.bin` | `df75f34c3856a057b81c70efec0b1b73f515b92109f562f58e9cbcf614cf0a7c` | 38,332 B | the daily image |
| **dev** | `artifacts/falcon_kalam_dev.bin` | `6897f988c8cb40357200ee8480278facada4e7c9517862c7702646f86b6a8923` | 44,756 B | testing and diagnosis only; never the daily image |
| (base) | `artifacts/falcon_kalam_default.bin` | — | — | L1 reference with NKRO, no VIA; **never flash** |

Stock image: 43,940 B. What is on the device: `docs/devnotes.md`.

## 2. Features

vs stock: **=** same · **+** same function, done better · **cfg** supported
by L1, disabled by L2 · **new** not in stock.
Hardware: **✓** = verified on the device (2026-10-26/27), on an image with the
same feature code.

**Release validation (2026-10-27, releases 15–17).** Dev image: probe (release, layers, macros, the VIA keymap equal to the compiled layers, bootloader and option bytes equal to the originals, 20-byte UID dump); every base key and all 27 HHK Fn combinations on the console with 0 mismatches; the three modes (Win via SW1 and a replug), the five switches with replugs; the light test (every indicator state; the settings display's second half since release 18, B14); Command H/V/S; the Caps LED; the fault test and Bootmagic, each with an identical readback. Ship image: probe and keymap dump, the plug-in (dark, then green, no LED 1 flash), the ship Command list (H), the VIA app (loads `via/falcon_kalam.json`, connects as "Falcon", six layers, no lighting menu), Esc→DFU with an identical readback. Log: `artifacts/release_validation_2026-10-27_console.log`.

| Feature | Ship | Dev | vs stock | Hardware |
|---|---|---|---|---|
| Platform | QMK 0.34.7, ChibiOS (pinned), GCC 8.5, LTO | same | + newer QMK | ✓ |
| USB identity | `9DD1:9D89`, bcdDevice `0x0001`, "Falcon" / "Falcon" (POLICY-NAMING), 500 mA, remote wakeup | same | = | ✓ |
| HID interfaces | keyboard 6KRO, VIA, shared mouse/system/consumer, console; 1 ms | same | = (no NKRO report) | ✓ |
| NKRO | off (POLICY-NKRO) | off | cfg (the macOS fix) | ✓ Caps Lock and input-source switching work on macOS, also after a replug |
| Matrix, layout | COL2ROW, stock pins, 5 ms debounce, scan rate ≈ 3,677 Hz (dev trace); HHKB classic (`LAYOUT_60_hhkb`) | same | = / + | ✓ all 60 keys (2026-10-27, dev console, `artifacts/dev_session_2026-10-27_keycheck.txt`) |
| Keymap (POLICY-HHKB) | a copy of the HHKB Professional Classic: its six layers (HHK, Mac, Win; base + Fn each) with its HID codes, its mode keys Fn+Ctrl+M/W, and its DIP switches SW1–SW5 as virtual switches (Fn+Ctrl+Shift+1…5); HHK mode from the factory, as the Classic (§3) | same | new (stock: generic 60%) | ✓ all three modes (HHK base/Fn, Mac Fn incl. Eject: macOS answers Control+Eject; Win Fn), mode keys incl. the Classic's Fn+Ctrl+W rule, SW1–SW5 (dev, 2026-10-27) |
| Settings display (additions) | Fn+Ctrl+Shift+0 and the mode keys show the mode on LED 1 (HHK red, Mac blue, Win green) and SW3–SW5 on LEDs 2–4 (blue = on), first as in effect, then as after the next plug-in if different (§3) | same | new | ✓ both phases with a dark gap (dev release 6) |
| Release number | `VIA_FIRMWARE_VERSION`, readable over VIA | same | new | ✓ (probe) |
| VIA | protocol `0x000D`, 6 layers (the three modes, in the Classic's order), 16 macros in a 1,107 B buffer (stock: 4 layers, 384 B), opens as HHKB | same + **`VIA_INSECURE`** (VIA key tester and `falcon_kalam_probe.py matrix` work) | + | ✓ (probe); the VIA app loads the definition, connects, shows six layers and no lighting menu (ship release 17) |
| Bootmagic | Esc at plug-in → clear EEPROM → DFU, LEDs painted red | same | = (+ red) | ✓ all four LEDs red in DFU (dev release 6, ship releases 8–9) |
| Fault recovery (POLICY-RECOVERY) | any CPU fault resets into DFU instead of hanging | same + Shift+Shift+F fault test | new | ✓ Shift+Shift+F drops into DFU; the console line arrives first (dev release 7) |
| Command (LShift+RShift+) | **selective**: H help, D debug, X matrix print, K key reports, V version, S status, B bootloader; every other combination types normally | **full** QMK set (adds Esc/bootloader, Backspace/clear EEPROM, / help, E, M, C, `` ` `` (base layer 0 = HHK mode); N, Caps and Z reach no compiled command and type normally, L4; layer switching by 0–9/F1–F10 is off as in ship) + F fault test | = (ship: safer subset) | ✓ ship (release 8): H D X K V S B work, help lists the remapped keys; Esc, Backspace, Caps, 1, F1, N, left ◇ type normally (the console shows only QMK's "?"; no action, no DFU entry) |
| Console | HID console (`qmk console`) | same + traces: key actions/tapping (`ACTION_DEBUG`), EEPROM backing store (`WEAR_LEVELING_DEBUG_OUTPUT`), matrix scan rate, readable keycode names | = / dev + | ✓ connects; dev traces (key actions, wear-leveling, scan rate) seen in the dev round |
| Bootloader read (FLASHREAD) | — | read-only dump of bootloader, option bytes, F_SIZE/UID (`falcon_kalam_probe.py dump-bootloader`) | new | ✓ |
| Caps Lock LED (POLICY-CAPSLED) | A10, active-low; lights 200 ms after the host sets Caps Lock, off at once (hides macOS's input-source flicker) | same | + | ✓ follows the host's Caps Lock with the delay: short taps that macOS asserts and reverts no longer flash it, a long press lights it (dev release 17). On macOS a short Caps Lock press (Fn+Tab) switches the input source and a long press toggles Caps Lock, as intended (the macOS setting this firmware was built to keep working) |
| RGB (POLICY-RGB) | **status indicator in pure red, green and blue only**, bright (cap 69 of 255) or dim (an eighth of the cap): all green bright = normal; all red = bootloader; red ×3 on dark = settings reset; channel A red = keystroke printout, blue = debug; channel B dim green = not configured by a host, blue = layer locked; off-blinks = virtual DIP switch flipped; settings display (`policy.md` §2.1); effects compiled out (L1 keeps them); no light toggle key; off while the host sleeps | same + blue blink while FLASHREAD serves a request + **light test** Shift+Shift+L (every state in turn, named on the console) | deliberate change (stock: decorative, rainbow swirl) | ✓ green base, channel A blue/red, channel B blue (layer lock), red in DFU (both routes, dev and ship), red ×3 on dark with nothing before it (release 10), off while the Mac sleeps; a normal plug-in lights once (release 11), with no flash on LED 1 (B13); not tested: dim green on a power-only port (after the 2 s dark start) |
| EEPROM | wear-leveling 4 KB at `0x0800F000` (2 KB of settings; stock 2 KB / 1 KB), flash size pinned; a schema stamp resets stale settings once after a layout change | same | + | ✓ switch flips persist across replugs; long-term wear-leveling: R2 |
| Flash safety | the linker refuses to grow into the EEPROM; the bootloader is never written | same | new | ✓ readback |
| Keycode features | layer keys, one-shot, tri-layer, magic, macros, lighting, grave-escape, space-cadet, send-string, mouse/media keys | same | = | — |

## 3. Keymap (ship and dev)

A copy of the HHKB Professional Classic (POLICY-HHKB; reference:
`hardware.md` §7). Layers in the Classic's order: 0/1 HHK, 2/3 Mac, 4/5 Win.

```
Base (all modes):  Esc 1 2 3 4 5 6 7 8 9 0 - = \ `
                   Tab  Q W E R T Y U I O P [ ] Delete
                   Ctrl  A S D F G H J K L ; ' Return
                   Shift  Z X C V B N M , . / Shift Fn
                         Alt ◇      Space      ◇ Alt
  HHK: Delete = Delete,    ◇ = Muhenkan (left) / Henkan (right)
  Mac: Delete = Backspace, ◇ = Command, Alt = Option
  Win: Delete = Delete,    ◇ = Windows key

Fn, every mode:  1–= F1–F12  \ Ins  ` Del  I PrtSc  O ScrLk  P Pause
                 [ Up  ; Left  ' Right  / Down  K Home  L PgUp  , End  . PgDn
  HHK adds:  Delete Backspace  Right-◇ Stop
  Mac adds:  Esc Power  Tab Caps  Delete Clear  Return KP-Enter
             A Vol-  S Vol+  D Mute  F Eject  H KP*  J KP/  N KP+  M KP-
  Win adds:  Tab Caps  Delete Backspace  H KP*  J KP/  N KP+  M KP-

Mode keys (until the next plug-in):  Fn+Ctrl+M Mac · Fn+Ctrl+W Win (HHK when SW1/SW2 select HHK)
Virtual DIP switches (saved):        Fn+Ctrl+Shift+1…5 flips SW1…SW5; off-blink ×1 = off, ×2 = on;
                                     effective from the next plug-in, as on the Classic
Settings display:                    Fn+Ctrl+Shift+0 (and every mode key) → 1.5 s now, then 1.5 s
                                     after-replug (if different); LED 1 = mode: HHK red · Mac blue ·
                                     Win green; LEDs 2–4 = SW3–SW5 (blue on, off off)
  SW1/SW2  power-on mode: OFF/OFF HHK (factory) · ON/OFF Win · OFF/ON Mac · ON/ON acts as HHK
  SW3      Delete sends Backspace (Win/HHK)
  SW4      left ◇ = Fn
  SW5      swap Alt and ◇
```

Codes as the Classic sends them: Power = Keyboard Power (`0x66`), Stop
`0x78`, Clear `0x53`, volume/mute and Eject (`0xB8`) on the consumer page.
QMK's `KC_EJCT` would send `0xCC` (Stop/Eject), so `hhkb_modes.c` sends
`0xB8` for it. Keys a mode does
not support send the plain key. The switches act on top of the keymap, so VIA
shows the unchanged layers. Positions outside the HHKB layout are `KC_NO` and
can be assigned in VIA; all six VIA layers belong to the modes, and a `DF()`
key assigned in VIA must target a mode base (layer 0, 2 or 4), since the mode
logic reads the highest default layer. An EEPROM
reset restores the factory switches (all off: HHK mode). On a Mac, HHK mode
has no Command key (◇ = Muhenkan/Henkan): switch to Mac mode with Fn+Ctrl+M
until the next plug-in, or with SW2 (Fn+Ctrl+Shift+2, then replug) for good.

The L1 reference image (`keymaps/default`, never flashed) has a simpler map:
QMK's default keymap for the `60_hhkb` community layout (one Fn layer, as
`qmk_firmware/layouts/default/60_hhkb`) plus underglow keys on Fn+Z–B.

## 4. Release history

Release = `VIA_FIRMWARE_VERSION` (POLICY-RELEASE); ship and dev are rebuilt
together. Each release changed only what is listed; earlier rounds used
unnumbered images (`devnotes.md` history, `problems.md` ledger).

| Release | Commit | Change |
|---|---|---|
| 1 | `71e9252` | settings display (Fn+Ctrl+Shift+0, mode-key feedback), release number |
| 2 | `742c7dc` | brightness cap half of full scale |
| 3 | `8a1041e` | cap a quarter of full scale (63); settings-reset blink waits for the configured host (B5) |
| 4 | `77b2fb6` | indicators in pure red, green and blue only; green normal |
| 5 | `a02b9f6` | Bootmagic red: line held low, frame sent twice (R15); never flashed |
| 6 | `5087a01` | Eject sends `0xB8` (B6); settings-display gap dark (B7) |
| 7 | `d82e0b7` | reset blinks on a dark background; dev fault test faults from the main loop (B8) |
| 8 | `e64abde` | dark from post-init on a reset boot |
| 9 | `1ee19b1` | rgblight stored off on a reset, so `rgblight_init` draws nothing |
| 10 | `666a24a` | boot frame sent twice (LED 1 garbles the first frame) |
| 11 | `ff0538d` | dark until the host is ready at every boot; base lit once (B9) |
| 12 | `cd83517` | dark frame at `keyboard_pre_init` (L12) |
| 13 | `9bafd99` | dev: Command layer switching off (B10); the base light is switched back on if a raw-HID command turned it off (B11); FLASHREAD window covers the whole UID (B12); Eject handler sends on presses only |
| 14 | `6c13af9` | data line settled low for two reset periods before the first frame at pre-init and before the Bootmagic red (B13, L12) |
| 15 | `0e201f8` | dev: light test Shift+Shift+L shows every indicator state in turn |
| 16 | `f4e0ac5` (with 17) | dim level an eighth of the cap (1/255 duty): a quarter was too close to bright |
| 17 | `f4e0ac5` | brightness cap 69 (63 raised by a tenth, 14/255 duty); Caps Lock LED on-delay 200 ms (POLICY-CAPSLED) |
| 18 | `a3c0409` | dev light test runs the settings display and starts from a clean slate (B14); **the first official release**, tag `release-18` |

## 5. What L2 changes against the L1 base

L1 (`keyboards/falcon_kalam/`) is a plain QMK keyboard: the hardware facts
and QMK's normal features, buildable as `falcon_kalam:default` (never
flashed). L2 (`userspace/`) turns it into this firmware. Everything below is
an L2 decision (POLICY-* in `policy.md`); L1 is untouched by it.

| Area | L1 base | L2 (ship and dev) | Where |
|---|---|---|---|
| NKRO | on (`"nkro": true`) | **off**, and a build error if re-enabled (POLICY-NKRO) | `users/falcon_kalam/rules.mk`, `config.h` |
| VIA | off | **on**, 6 dynamic layers, 1,107 B of macros; release number as the VIA firmware version | `rules.mk`, `config.h` |
| Console, Command, LTO | off | **on** (stock parity); ship keeps only H D X K V S B of Command, dev the full set minus layer switching | `rules.mk`, `ship/config.h`, `dev/config.h` |
| Keymap | QMK's `60_hhkb` default + underglow keys, 2 layers | the HHKB Classic's six layers, modes, mode keys, virtual DIP switches, settings display (POLICY-HHKB) | `falcon_kalam_keymap.h`, `hhkb_modes.c` |
| Lights | rgblight with all 42 effects, rainbow default | effects compiled out; status indicator in red, green and blue at a low fixed brightness; dark start; stored off, switched on per session (POLICY-RGB) | `config.h`, `rgb_indicators.c` |
| Saved settings | QMK defaults | a schema stamp in the user word resets stale settings once after a layout change; the virtual switches live there too | `user_config.h`, `falcon_kalam.c` |
| Bootmagic | Esc at plug-in clears EEPROM and enters DFU | same, and paints all LEDs red first | `falcon_kalam.c`, `rgb_indicators.c` |
| Caps Lock LED | follows the host at once | 200 ms on-delay, off at once (POLICY-CAPSLED) | `caps_led.c` |
| CPU faults | ChibiOS default: hang | reset into DFU (POLICY-RECOVERY) | `recovery.c` |
| Dev only | — | FLASHREAD bootloader dump, `VIA_INSECURE` key tester, key-action / EEPROM / scan-rate traces, keycode names, fault test, light test | `dev/rules.mk`, `dev/config.h`, `flashread.c` |
| USB identity | "Falcon" / "Falcon", `9DD1:9D89` | unchanged (POLICY-NAMING) | L1 `keyboard.json` |
| EEPROM | 4 KB wear-leveling at the top of flash (sized by L1 for six layers) | used as is | L1 `keyboard.json`, `config.h`, `ld/` |
