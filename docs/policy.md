# Repository policy: design, organisation, documentation

## 1. Layers (POLICY-LAYERING)

```
qmk_firmware/                 L0 upstream QMK, latest release, never edited
keyboards/falcon_kalam/       L1 BASE: a normal QMK keyboard = hardware facts + normal QMK features
userspace/                    L2 CONFIGURATION: every project decision, tagged POLICY-*
  users/falcon_kalam/           shared by both variants (policy, keymap, modules)
  keyboards/…/keymaps/ship/     ship variant → falcon_kalam_ship.bin (daily image)
  keyboards/…/keymaps/dev/      dev variant  → falcon_kalam_dev.bin (debug image)
```

* A hardware fact goes in L1; a choice goes in L2. Upstream is never fixed
  here (POLICY-UPSTREAM): an upstream problem is configured around in L2, and
  when upstream fixes it (e.g. NKRO on macOS), only L2 changes.
* L1 passes `qmk lint` with upstream rules and builds `falcon_kalam_default.bin`,
  a reference that is **never flashed**.
* L2 is QMK External Userspace (`QMK_USERSPACE=userspace/`). Each variant's
  `rules.mk` sets `USER_NAME := falcon_kalam`, so QMK also builds
  `users/falcon_kalam/`. The variants differ **only** in their own three
  files. `grep -rn POLICY- userspace` lists every decision.
* The build copies L1 into `qmk_firmware/keyboards/` (hidden through the
  submodule's `info/exclude`, so the pinned tree stays clean). Edit the
  top-level copy only.

## 2. Policies

### Firmware (implemented in L2)

| ID | Rule | Implemented in | Enforced by |
|---|---|---|---|
| POLICY-NKRO | NKRO is a configuration decision: L1 supports it (`"nkro": true`); both variants disable it, because stock NKRO breaks the macOS Caps Lock / input-source toggle | `users/falcon_kalam/rules.mk` `NKRO_ENABLE = no` + `#error` guard in `users/falcon_kalam/config.h` | verifier (no NKRO report, flag or code); negative test. Lifted when an upstream QMK release fixes the problem (re-test on each update, I8): delete both, flip the verifier checks |
| POLICY-VIA | both variants are VIA-configurable | `users/falcon_kalam/rules.mk` | verifier |
| POLICY-PARITY | both variants keep every stock feature on top of L1: console, Command, LTO (stock RGB effects and defaults are superseded by POLICY-RGB). **Ship** restricts Command to H D X K V S B; dev keeps the full set | `users/falcon_kalam/rules.mk`, `users/falcon_kalam/config.h`, `ship/config.h` | verifier; `artifacts/feature_parity.txt` |
| POLICY-VARIANTS | **one tree, two images.** Ship = daily use, no debug-only feature. Dev = every debug feature that fits (FLASHREAD, `VIA_INSECURE`, full Command, `ACTION_DEBUG`, wear-leveling trace, scan rate, keycode strings), never the daily image. Shared code only in `users/falcon_kalam/`; no git branches for configuration | variant `rules.mk`/`config.h`; `#error` guards in `ship/keymap.c` | `build.sh` builds and verifies both every time (verifier profiles `release` = ship, `dev`); negative tests inject each debug feature into ship |
| POLICY-RGB | the RGB module is a **status indicator, not decoration** (§2.1), capped at **a quarter of full scale**: `RGBLIGHT_LIMIT_VAL 63` (full scale 255; 255 and 127 were too bright to look at). Decorative effects stay in L1 and are disabled by L2 for every variant | `users/falcon_kalam/rgb_indicators.c`; `#undef` of every effect in `users/falcon_kalam/config.h` | verifier: L1 has all 42 effect modes, ship and dev none, the indicator table is in both, the cap is 63, the VIA JSON has no lighting menu; `#error` in `rgb_indicators.c` if an effect is defined |
| POLICY-RECOVERY | a CPU fault never hangs the keyboard: it resets into DFU (the unit has no power-on DFU window). No watchdog (§5) | `users/falcon_kalam/recovery.c`; dev Shift+Shift+F fault test | verifier: in ship and dev all four fault vectors lead to the stack-free reset routine; base keeps ChibiOS's defaults |
| POLICY-FLASHREAD | **dev only**: read (never write) the bootloader, option bytes and F_SIZE/UID over raw HID. Not in ship, because it exposes the chip UID | `users/falcon_kalam/flashread.c`, compiled when `FALCON_KALAM_FLASHREAD = yes` | ship: `#error` guard + verifier (no `via_command_kb`); dev: verifier requires it |
| POLICY-HHKB | the keyboard **mimics the HHKB Professional Classic in every way possible** (§2.3): its layout, setting modes, keys, HID codes, mode keys and DIP switches; HHK mode from the factory, as the Classic | `keyboard.json`, `via/falcon_kalam.json`; `users/falcon_kalam/falcon_kalam_keymap.h`, `hhkb_modes.c/.h`, `config.h` (6 layers); L1 EEPROM sized for it | verifier §6: the compiled keymap equals a table written from the manual; no ship Command key is in any layer |

### 2.1 RGB policy (POLICY-RGB)

**Normal = all 4 LEDs green at the brightness cap.** Anything else means
something.

* **Colours: pure red, green and blue only.** Each lights one diode; mixed
  colours (white, yellow, magenta, …) blend poorly on this module and are
  hard to tell apart. Meaning by colour: green = normal, blue = a state worth
  knowing, red = attention.
* **Two levels:** bright = the cap, `RGBLIGHT_LIMIT_VAL 63` in
  `users/falcon_kalam/config.h` (a quarter of full scale 255; full and half
  were too bright to look at); dim = a quarter of the cap (15). Bright =
  active, dim = waiting. These are rgblight values; QMK's CIE 1931 brightness
  curve turns them into diode duty 12/255 and 2/255. rgblight clamps every
  colour to the cap, and the Bootmagic red passes the same cap and curve.
* **Boot:** dark until the host has configured the keyboard (at most 2 s),
  then the base lights once (or the reset blinks run first). rgblight is
  stored off (`RGBLIGHT_DEFAULT_ON false`) and switched on for the session
  only, so neither QMK's start-up drawing nor the host's suspend/resume
  during enumeration makes the LEDs flicker.
* States are rgblight layers over the base; the higher priority wins where
  they overlap. Channels A (LEDs 1–2) and B (LEDs 3–4) let one safety state
  and one connection state show together. LEDs are counted left to right.

| Priority | State | Shown as | LEDs | Trigger |
|---|---|---|---|---|
| 1 | bootloader (about to jump; kept while in DFU) | red, bright, steady | all | `shutdown_user(jump=true)` (Command B, `QK_BOOT`) and Bootmagic (Esc at plug-in) push the frame out; WS2812s latch it. Not after a CPU fault (POLICY-RECOVERY) |
| 2 | settings were reset | red, bright, 3 blinks on an all-off background, then the base | all | `eeconfig_init_user`; shown at the end of the dark boot start |
| 3 | virtual DIP switch flipped | off-blink: 1 = switched off, 2 = on | all | Fn+Ctrl+Shift+1…5 (POLICY-HHKB) |
| 4 | settings display | LED 1 = mode: HHK red, Mac blue, Win green; LEDs 2–4 = SW3–SW5: blue = on, off = off; 1.5 s now, an off gap, 1.5 s after the next plug-in if different | all | Fn+Ctrl+Shift+0 and the mode keys Fn+Ctrl+M/W (POLICY-HHKB) |
| 5 | keystroke printout on (privacy warning) | red, bright | A | debug + Shift+Shift+K |
| 6 | debug output on | blue, bright | A | Shift+Shift+D / X; always on from boot in dev (QMK's scan-rate option) |
| 7 | USB not configured by a host | green, dim | B | USB state ≠ configured/suspended; visible once the dark boot start ends (after 2 s without a host) |
| 8 | layer locked on | blue, bright | B | a layer other than the mode's base and its held Fn layer is on, or the default layer is not a mode base (POLICY-HHKB) |
| — | Fn held, Caps Lock | no change | — | Fn is momentary; Caps Lock has its own LED |
| — | host asleep | all off | all | `RGBLIGHT_SLEEP`; indicators are suppressed too |
| — | dev only (FLASHREAD): request served | blue blink | all | `falcon_kalam_rgb_activity()` |

* Both variants behave the same. Every effect is compiled out, and the base is
  re-set to on, static green on every main-loop pass, so old EEPROM values and
  VIA lighting commands (colour, mode, off) cannot change it. The VIA
  definition has no lighting menu, and no default key switches the base off.
* Dim green on channel B ("powered but not configured") covers charge-only
  cables, power-only ports (charger, power bank, some hubs and monitors), a
  host that doesn't enumerate; it shows once the 2 s dark start has ended. It stays,
  although it also shows on a port that keeps power while the computer is off.
* No separate LED test tool is needed: red, green and blue each check one
  diode (and the WS2812 colour order), and Shift+Shift+D shows channel A.

### 2.2 Naming (POLICY-NAMING)

| Context | Name | Where |
|---|---|---|
| **Seen by an ordinary user** (brand) | **Falcon** | USB manufacturer and product strings (`keyboard.json` `manufacturer`, `keyboard_name`), the VIA definition's `name` |
| **Development only** | **falcon_kalam** (`FALCON_KALAM_` for macros) | QMK keyboard id and folders (`keyboards/falcon_kalam`, `userspace/keyboards/falcon_kalam`, `userspace/users/falcon_kalam`), image and VIA file names, C identifiers and macros, tool names, environment variables (`FALCON_KALAM_GCC_BIN`), dev prose about this unit |
| Variants | **ship**, **dev** | keymap names; images `falcon_kalam_ship.bin`, `falcon_kalam_dev.bin` (plus the L1 reference `falcon_kalam_default.bin`) |
| The original board | **MatrixLAB Falcon** | prose; QMK's `matrix/falcon` is that board (AVR) and is never flashed here |
| Stock facts | quoted verbatim | e.g. stock USB strings "Falcon", stock VIA name "Falcon_HS" |

"kalam" stands for 迦蓝, the clone's maker; the suffix keeps every dev name
apart from MatrixLAB's Falcon. Exceptions: the immutable originals keep their
file names (`originals/falcon_flash_backup.bin`, POLICY-ORIGINALS), and the
repository directory name (`falcon-keyboard`) is outside the repo's control.
The USB ID `9DD1:9D89` is unchanged; VIA matches on it.

Enforced by `verify_firmware.py` (both USB string descriptors are exactly
"Falcon"; no `kalam`, `falcon_` or retired name in any USB string;
`keyboard.json` and the VIA name are "Falcon") and by `check_docs.py`
(retired names appear nowhere else; device logs under `artifacts/` recorded
before the rename keep the strings they captured).

History (the only place retired names may appear): until 2026-10-26 the dev
id was `falcon_hs_r2`, the product string "Falcon HS R2", and the variant
keymaps `keymaps/falcon/` (ship) and `keymaps/falcon_dev` (dev); before that
the release keymap was `hhkb_via`.

### 2.3 HHKB policy (POLICY-HHKB)

**The keyboard mimics the HHKB Professional Classic in every way possible**
(reference: `hardware.md` §7). Where the hardware or a policy prevents a
copy, the closest equivalent is used, and every deviation is listed here.

| Point | How |
|---|---|
| Physical layout | HHKB classic 60 keys (`LAYOUT_60_hhkb`, VIA layout option 0) |
| Keymap | the Classic's six layers in its own storage order: HHK base/Fn, Mac base/Fn, Win base/Fn; unsupported Fn keys send the plain key, as on the Classic |
| HID codes | as the Classic sends them: Power = Keyboard Power `0x66` (not QMK's usual System Power; kept although macOS ignores it, since the Classic sends the same), Stop `0x78`, Clear `0x53`, volume/mute and Eject `0xB8` on the consumer page (QMK's `KC_EJCT` sends `0xCC`; `hhkb_modes.c` sends `0xB8` through `process_record_user`, an upstream hook) |
| Mode keys | Fn+Control+M → Mac mode; Fn+Control+W → Win mode, or HHK mode when SW1/SW2 select HHK; effective until the next plug-in, as SW1/SW2 decide the power-on mode |
| DIP switches | **virtual**, since the unit has none: SW1–SW5 saved in EEPROM, all off from the factory (HHK mode) as on the Classic, flipped with Fn+Control+Shift+1…5 (acknowledged by an off-blink, POLICY-RGB) and, as on the Classic, effective from the next plug-in. SW1/SW2 = power-on mode; SW3 = Delete sends Backspace (not in Mac mode); SW4 = left ◇ becomes Fn; SW5 = swap Alt and ◇. Like the Classic's, they act on top of the keymap (VIA shows the unchanged layers) |
| Display (additions) | the Classic has none; on the RGB module (itself a deviation): Fn+Control+Shift+0 and the mode keys show the settings in effect and after the next plug-in |

Deviations, each because a copy is impossible or conflicts with a policy;
all are accepted:

| Deviation | Reason |
|---|---|
| SW1/SW2 ON/ON acts as HHK mode | the manual defines no such setting |
| No SW6 (power saving); the keyboard can always wake the host, as with SW6 on (the Classic ships with SW6 off: a key press cannot wake the host) | QMK's wake-up check (`suspend_wakeup_condition()`) has no configuration hook; emulating SW6 would mean patching QMK (POLICY-UPSTREAM) |
| USB identity `9DD1:9D89` "Falcon", not PFU's `04FE:0020` "HHKB-Classic" | the board must not pass as PFU's product (POLICY-NAMING); VIA matches on our ID |
| VIA instead of PFU's keymap tool and vendor interface | PFU's protocol is proprietary; VIA gives the same remapping (POLICY-VIA) |
| RGB status lights | the Classic has none; the unit's light module serves as a status indicator (POLICY-RGB) |
| Console, Command, Bootmagic | stock features of this unit (POLICY-PARITY) |
| 6KRO only; the Classic also declares an NKRO report (unused) | **temporary**: POLICY-NKRO keeps NKRO off until upstream fixes the macOS problem |

The policy lives in L2, since the keymap is a choice. L1 holds the physical
layout and sizes the EEPROM for six layers; its own default keymap stays a
plain QMK-style HHKB reference.

### Project

| ID | Rule | Enforced by |
|---|---|---|
| POLICY-UPSTREAM | upstream (QMK, ChibiOS, the bootloader) is used as released and **never fixed here**. A problem upstream is configured around in L2 (e.g. POLICY-NKRO disables NKRO instead of fixing it), recorded, and the configuration is lifted when an upstream release fixes it. Only upstream's own extension points are used: config macros, `rules.mk` options, `*_user`/weak hooks, the keyboard's linker script. Behaviour without such a hook is a deviation, not a patch | `qmk_firmware` is never edited (`build.sh` requires a clean pinned tree); review |
| POLICY-QMK-LATEST | `qmk_firmware` sits exactly on the newest upstream QMK release tag; being newer than stock is intended | `scripts/qmk_policy.sh` (offline in every build, fatal; `--online` before each flash) |
| POLICY-ORIGINALS | `originals/` is immutable evidence: mode 444, hashes in `SHA256SUMS`. New device dumps may be **added**, never changed | `build.sh` hash check |
| POLICY-REPRODUCIBLE | offline build, pinned toolchain and sources, identical bytes on rebuild on any day (`SKIP_VERSION=yes`, `SOURCE_DATE_EPOCH` = pinned QMK commit time). Not the real date, because an approved SHA-256 must be re-derivable, and VIA wipes the saved keymap whenever the build date changes; pinned, that happens only with a QMK update, and our own layout changes use the schema stamp (§5) | `build.sh --twice`; the verifier checks the embedded build date |
| POLICY-EVIDENCE | hardware parameters come only from verified evidence; anything unproven is marked so, never invented | analysis tool (exact-address checks); status column in `hardware.md` |
| POLICY-READONLY-DEVICE | without an approval, only read the device: enumeration, DFU **upload**, whitelisted VIA queries | `falcon_kalam_probe.py` whitelist |
| POLICY-FLASH-GATE | flash only an L2 image (ship, or dev for testing) whose approval names the **variant and its SHA-256**, via DFU **alt 2**, then test Esc→DFU: with no SWD probe, DFU is the only recovery path and must stay reachable. The daily image is always ship | `scripts/flash.sh` (refuses other images, wrong hashes, devices not in DFU; alt 2 hard-coded; pre-flash readback) + its negative tests; `operations.md` §2 |
| POLICY-OPEN-ISSUES | an open issue stays open until the owner closes it explicitly | `problems.md` |
| POLICY-NAMING | §2.2 | verifier, `check_docs.py` |
| POLICY-RELEASE | every change of the ship/dev images bumps the release number `VIA_FIRMWARE_VERSION` (`users/falcon_kalam/config.h`), readable over VIA; `firmware.md` states it | `scripts/check_docs.py` (matches `firmware.md`; fails if the images changed since HEAD without a bump) |
| POLICY-DOCS | §4 | `scripts/check_docs.py` (run by `build.sh`) |

## 3. File organisation

| Path | Content |
|---|---|
| `keyboards/falcon_kalam/` | L1: `keyboard.json` (MCU, bootloader, USB IDs, pins, COL2ROW, Caps LED A10 active-low, wear-leveling 4096/2048, WS2812 A1, rgblight 4 LEDs + all effects + sleep, layouts `LAYOUT_60_hhkb` (default) and `LAYOUT_all`); `config.h` (flash size pinned to 64 KB); `rules.mk` + `ld/…falcon_kalam.ld` (app region `0x08002000–0x0800EFFF`, exports `__falcon_kalam_app_*`); `keymaps/default/` (QMK-style HHKB reference); `readme.md` |
| `userspace/` | L2: `qmk.json` (targets `falcon_kalam:ship`, `falcon_kalam:dev`); `users/falcon_kalam/`: `rules.mk` (shared policy, FLASHREAD switch), `config.h` (guards, RGB layer settings, layer count), `falcon_kalam_keymap.h` (the keymap), `hhkb_modes.c/.h` (POLICY-HHKB layers, mode keys, virtual DIP switches), `user_config.h` (the saved user word: schema stamp + DIP switches), `falcon_kalam.c` (the only file defining QMK `*_user` hooks; `via_command_kb` lives in `flashread.c`, the fault vectors in `recovery.c`), `rgb_indicators.c/.h` (POLICY-RGB), `recovery.c` (POLICY-RECOVERY), `flashread.c`; `userspace/keyboards/falcon_kalam/keymaps/ship/` (selective Command, debug guards) and `…/dev/` (all debug features) |
| `via/falcon_kalam.json` | VIA definition for both variants: the stock JSON with HHKB as layout option 0, without the lighting menu |
| `qmk_firmware/` | submodule, pinned (POLICY-QMK-LATEST) |
| `bootloader/STM32duino-bootloader/` | MIT bootloader source, pinned submodule; reference only, never built or flashed |
| `originals/` | stock BIN, readback, VIA JSON, `device_dump_2026-10-26/` (bootloader, F_SIZE/UID, option bytes), `SHA256SUMS` |
| `scripts/` | `env.sh` (shared toolchain and QMK environment), `build.sh` (the only build entry), `flash.sh` (the only flash entry: `check`/`flash`/`verify`), `verify_firmware.py` (`--profile base\|release\|dev`), `negative_tests.sh`, `qmk_policy.sh`, `check_docs.py` |
| `tools/` | `analyze_vendor_firmware.py`, `feature_parity.py`, `falcon_kalam_probe.py` (read-only VIA probe), `usb_watch.py` (passive USB log), `console_keycheck.py` (checks logged key presses against POLICY-HHKB), `disassemble_vendor.sh` |
| `artifacts/` | built images (`.bin` with DFU suffix, `.elf`, `.map`, cflags, `info_config.h`, `.sha256`), build/verification reports, device logs |
| `build/` | scratch, git-ignored |

## 4. Documentation (POLICY-DOCS)

One document per category; add a document only when strictly required, and
record it here.

| # | File | Holds | Update rule |
|---|---|---|---|
| 1 | `docs/hardware.md` | hardware facts and conventions (pins, matrix, layout, lights, bootloader, memory, the HHKB reference) | when a fact is established; give its evidence |
| 2 | `docs/stock-firmware.md` | stock image capabilities and feature list, RE method | when new RE results arrive |
| 3 | `docs/policy.md` | this file: design, policies, file organisation, doc rules, design notes | when a rule or the file layout changes |
| 4 | `docs/firmware.md` | our firmware's features, the current images, what is verified on hardware | at a milestone (from devnotes) |
| 5 | `docs/devnotes.md` | **the only status document**: device state, to-do, work in progress, results awaiting a milestone | most often. When an item reaches a milestone, write the result into `firmware.md` and **delete** it here |
| 6 | `docs/problems.md` | open risks, bugs, limitations; closed items as a one-line ledger | when found or resolved |
| 7 | `docs/improvements.md` | optional improvements, not committed to | when an idea arises; move it to devnotes when work starts |
| 8 | `docs/operations.md` | procedures: build, flash, tests, recovery, QMK update | when a procedure changes |

Rules:
* Status (what is flashed, what is pending) appears **only** in `devnotes.md`;
  the current image hashes appear **only** in `firmware.md` (`devnotes.md`
  names the flashed image by its hash prefix). Other docs refer to them. `README.md` is an index.
* Decisions are written as the rule or fact they establish, with their
  reason, not as who decided what when.
* `scripts/check_docs.py` checks that exactly these docs exist, that every
  referenced repo path exists, that `firmware.md` matches `artifacts/`, and
  that retired names stay in §2.2.

## 5. Design notes (non-obvious code)

**`via_command_kb()` / `falcon_kalam_read_allowed()`** (`users/falcon_kalam/flashread.c`, dev only).
QMK's VIA layer calls `via_command_kb()` first for every raw-HID packet; if it
returns true, VIA skips its own dispatch and the handler must reply. The
handler claims only command `0xF0`: a big-endian address and a length (≤ 24,
so the reply fits in 32 bytes), a range check, then `volatile` reads. Allowed:
`0x08000000–0x08001FFF`, `0x1FFFF7E0–0x1FFFF7F3`, `0x1FFFF800–0x1FFFF80F`.
`addr + len >= addr` rejects wrap-around; a refused range is answered with
length 0. `noinline` keeps the symbol visible under LTO.

**Shared keymap (`users/falcon_kalam/falcon_kalam_keymap.h`).** The header
*defines* the `keymaps` array and is included exactly once, from each
variant's `keymap.c`. It can't be a `.c` under `users/`: QMK's keymap
introspection compiles the variant's `keymap.c` itself and derives the layer
count from that array.

**`hhkb_modes.c` (POLICY-HHKB).** Each mode is a base layer (0 HHK, 2 Mac,
4 Win) whose Fn key is `MO()` of its own Fn layer (1, 3, 5); the active mode
is QMK's (non-persistent) default layer. At post-init the virtual SW1/SW2 pick
it; the mode keys change it until the next plug-in. A press with any Fn layer
on and Control held is judged by its **base-layer** keycode (on the Fn layers
M is keypad minus, 0–5 are F10, F1–F5): M and W switch mode and show the
settings, Shift+1…5 flip a saved switch, Shift+0 shows the settings;
these presses are swallowed. The switches in effect are read
once at post-init, so a flip waits for the next plug-in, as on the Classic. SW3–SW5 act only on keys that resolve on
a base layer (`layer_switch_get_layer`), so Fn+\` stays Delete: SW4 turns the
left ◇ (`KC_LGUI`, or `KC_INT5` in HHK mode) into the mode's Fn layer, SW5
swaps Alt with the mode's ◇ code, SW3 turns base-layer Delete into Backspace
outside Mac mode. `KC_EJCT` is sent as consumer `0xB8` (the
Classic's Eject) with `host_consumer_send()`; its release falls through to
QMK, which clears the consumer report. SW4 is applied before SW5, so with both on the left Alt
becomes ◇. Each substituted key's replacement is stored per matrix position
at press and replayed at release, so a release matches its press even if the
mode changed in between. A Fn layer held during a mode switch stays the old
mode's layer: QMK caches each key's source layer, so the `MO()` release still
turns it off, but after a switch to a higher mode (HHK→Mac/Win, Mac→Win) the
new base layer shadows it until Fn is pressed again, since the base layers
have no transparent keys; the Fn+Control combinations still work, as they
test `layer_state`.

**Selective Command (ship `config.h`).** Unwanted `MAGIC_KEY_*` commands point
at keys no layer sends (`INTERNATIONAL_1`–`3` and `6`–`9`, `LANGUAGE_1`–`3`),
so LShift+RShift+that-key reaches no case in `command.c`'s switch and types
normally. `INTERNATIONAL_4`/`5` are avoided because HHK mode's ◇ keys send
them (Shift+Shift+◇ would otherwise jump to the bootloader); the verifier
checks that no layer contains a Command key. Each command needs a distinct key
(duplicate cases don't compile). Names such as `F13` are unusable: QMK defines
`F13` as a pin macro (GPIOF pin 13), which breaks the `KC_##key` paste. Layer
switching is off (`MAGIC_KEY_SWITCH_LAYER_WITH_{FKEYS,NKEYS} false`) in both
variants: with six layers, Shift+Shift+6–9 would select a layer without a
keymap and silence the keyboard until the next plug-in.

**`falcon_kalam.c` (hook dispatcher).** QMK's `*_user` hooks and the shared
weak hooks (`bootmagic_reset_eeprom`, `command_extra`) can each be defined
once per firmware. They live here and call module functions
(`falcon_kalam_rgb_*`, `falcon_kalam_hhkb_*`), so modules never collide; the
two single-owner hooks stay with their module (`via_command_kb` in
`flashread.c`, the fault vectors in `recovery.c`). It also owns the **settings schema stamp**. The eeconfig user word
(`user_config.h`) holds a 16-bit schema and the virtual DIP byte;
`eeconfig_init_user` writes `FALCON_KALAM_EEPROM_SCHEMA` and the factory
switches (all off = HHK mode), and `keyboard_pre_init_user` (after the EEPROM driver
starts, before QMK reads the settings) marks the settings invalid when the
schema differs. QMK then
re-initialises everything, VIA keymaps and macros included; its init calls
our hook last, so the new stamp sticks. Bump the stamp whenever stored data
changes meaning (layer count or order). VIA's own check cannot do this: it
keys on the build date, which is pinned.

**`recovery.c` (POLICY-RECOVERY).** QMK writes `BKP_DR10 = 0x424C` (stay in
DFU) at every boot, and the bootloader honours it after any reset. The four
fault vectors alias one naked routine: write `0x424C` again, set
`SCB->AIRCR = 0x05FA0004` (system reset), wait. Immediates only (no stack, no
literal pool), because a second fault inside a HardFault handler locks the
Cortex-M3 up. `_Static_assert`s pin the hard-coded addresses to the CMSIS
definitions. Rejected: a watchdog (the IWDG keeps running across the reset
and the bootloader never feeds it, so it would abort DFU sessions). Not
covered: `chSysHalt()`, whose hook (`CH_CFG_SYSTEM_HALT_HOOK`) is upstream's
`chconf.h`; QMK lets only a keyboard-level (L1) `chconf.h` override it, and a
choice may not go into L1.

**`rgb_indicators.c`.** `falcon_kalam_rgb_init` (post-init) installs the layer
table. `falcon_kalam_rgb_task` (every main-loop pass) polls the USB state,
debug flags and layer state, and calls `rgblight_set_layer_state()` only for
layers that changed, since each call schedules a redraw; it also re-sets the
green base, and advances the settings display (now → gap → after replug), a
timed sequence of rgblight layers: the mode colour on LED 1, an off layer over
LEDs 2–4, and a blue layer per switch on top; a new request restarts it. The USB-state callback is avoided because it can run in interrupt
context; the debug flags have no callback. The layer-locked state (blue on
channel B) treats the
mode bases (default layers 0/2/4; an empty default state means layer 0) and a
held Fn layer (1/3/5) as normal. `falcon_kalam_rgb_shutdown` sets the red layer and
calls `rgblight_set()` directly, since no main-loop pass follows. Bootmagic
runs before rgblight exists, so `falcon_kalam_rgb_paint_bootloader` drives the
WS2812 driver directly (`ws2812_init`, a 1 ms low gap as reset, `set_color_all`
with the red that `hsv_to_rgb({0, 255, cap})` gives, 12/255 after the CIE curve,
`flush` twice).
EEPROM initialisation runs before the LEDs exist, so it only sets a flag; the
boot is dark: `keyboard_pre_init_user` drives the data pin and sends a dark frame twice (`falcon_kalam_rgb_pre_init`, before rgblight starts; until then LED 1 can show noise), rgblight is stored off, so `rgblight_init` draws nothing, the all-off layer (`FALCON_KALAM_RGB_DARK`, also used for the switch-flip blinks) is on from post-init, and the dark frame is sent twice (LED 1 garbles the first frame after the data pin is set up; the Bootmagic red is sent twice for the same reason). When the host has configured the keyboard, or after 2 s, the base is switched on with `rgblight_enable_noeeprom()` (the stored setting stays off), or on a reset the red blinks run first. A stored "on" from older images is rewritten once. rgblight blinks share one repeat timer, so overlapping blinks interfere: a switch flip during the reset blinks or the dark start cuts them short (accepted), and in dev a FLASHREAD dump at boot does the same. Layer changes are applied in `rgblight_task()`, which exists because
`RGBLIGHT_LAYERS` forces `RGBLIGHT_USE_TIMER` even with no effects compiled.

**EEPROM placement.** ChibiOS describes the F103 flash as 128 × 1 KB sectors
(the xB registry). QMK drops the sectors at or above the flash size, then takes
sectors from the top until it has the backing size: 4 KB gives `0x0800F000`
(stock: 2 KB at `0x0800F800`). L1 pins the flash size, so a GD32's F_SIZE
cannot move it; the linker script reserves the same 4 KB. Why 4 KB: six
layers × 150 B leave under 100 B of a 1 KB settings area for macros, and QMK
refuses that; 2 KB leaves 1,107 B. On the first boot after the move, the old
2 KB sits where the new write log starts; the new base image is erased, so
its checksum fails and wear-leveling clears its cache, QMK's settings magic
fails, and the schema stamp forces a clean re-initialisation anyway.

**`analyze_vendor_firmware.py`.** The stock image has no symbols. Each check
names an address and the exact instruction/data bytes expected there
(`Image.thumb()` compares objdump-style halfwords); it never searches for a
"best match". Pin tables are decoded from ChibiOS `ioline_t` (port base | pad).
The wear-leveling log is decoded by inverting each 16-bit cell and splitting
the 2-bit entry type from the address and value.

**`verify_firmware.py`.**
- Container: valid DFU suffix (CRC32); the payload equals `objcopy -O binary` of the ELF.
- Placement: every `LOAD` segment lies in `[0x08002000, 0x0800F000)`; RAM within 20 KB.
- Vector table: SP = `__main_stack_end__`, reset = `Reset_Handler`, every vector points into the image.
- Profiles: base = NKRO present, no VIA. Ship and dev = no NKRO (only ChibiOS's empty `send_nkro` stub may exist); VIA, console, Command and LTO present; keymap from the right variant directory. Ship has none of the dev features; dev has all of them (the FLASHREAD symbol, the `-D` flags, the trace strings of `ACTION_DEBUG`, `WEAR_LEVELING_DEBUG_OUTPUT` and the scan rate). `VIA_INSECURE` leaves no binary trace, so only the ship guard and its negative test enforce it.
- Instruction checks (`thumb2_find`, `movw_find`) match opcode and immediate but ignore register fields, because each variant allocates registers differently. They are validated against the stock image and against constants that must not match.
- Stock equivalence: pin tables, flash descriptor, clock literal and PLL instruction, BKP flags, JTAG-disable instruction, Caps LED config; VIA's default view = `LAYOUT_60_hhkb`. The EEPROM size is checked as the enlarged 4096/2048.
- POLICY-HHKB (ship and dev): reads the `keymaps` array from the ELF and compares all six layers, in the Classic's order, with `hhkb_expected()`, built from the manual's Fn table (`HHKB_FN`) and the base differences (`HHKB_MODES`), written independently of the C source; keycode values come from the pinned `quantum/keycodes.h`. Ship: the Command keys in `ship/config.h` must be in no layer.

**`feature_parity.py`.** `config_descriptor()` finds a USB configuration
descriptor by its header and accepts it only if walking the length-prefixed
sub-descriptors lands exactly on `wTotalLength` with the declared interface
count. `peripherals()` lists the peripheral blocks each image references, so
hardware outputs are compared as well.

**`falcon_kalam_probe.py`.** Uses the `hid` module of Homebrew's QMK CLI
Python. `send()` refuses everything outside a read-only whitelist (`0x01,
0x02, 0x08, 0x0C, 0x0D, 0x11, 0x12, 0xF0`; for `0x02` only uptime, layout
options, matrix and firmware version). `matrix()` reads row data from byte 3
on VIA protocol ≥ 12 (which echoes a row offset in byte 2), from byte 2 on
older ones. `legacy_to_actual()` re-maps logs recorded with the earlier
off-by-one decoder. QMK returns an all-zero matrix to VIA unless built with
`VIA_INSECURE`, which only dev sets; on ship the matrix is tested through the
console.
