# Repository policy: design, organisation, documentation

## 1. Layers (POLICY-LAYERING)

```
qmk_firmware/                 L0 upstream QMK, latest release, never edited
keyboards/falcon_kalam/       L1 BASE: a normal QMK keyboard = hardware facts + normal QMK features
userspace/                    L2 CONFIGURATION: every project decision, tagged POLICY-*
  users/falcon_kalam/                 shared by both variants (policy + keymap + FLASHREAD module)
  keyboards/…/keymaps/ship/      ship variant  → falcon_kalam_ship.bin (daily image)
  keyboards/…/keymaps/dev/  dev variant   → falcon_kalam_dev.bin (debug image)
```

* A hardware fact goes in L1; a choice goes in L2. When upstream fixes
  something (e.g. NKRO on macOS), only L2 changes.
* L1 passes `qmk lint` with upstream rules and builds `falcon_kalam_default.bin`,
  a reference that is **never flashed**.
* L2 is QMK External Userspace (`QMK_USERSPACE=userspace/`). Each variant's
  `rules.mk` sets `USER_NAME := falcon_kalam`, so QMK also builds `users/falcon_kalam/`.
  A variant differs from the other **only** in its own three files.
  `grep -rn POLICY- userspace` lists every decision.
* The build copies L1 into `qmk_firmware/keyboards/` (hidden through the
  submodule's `info/exclude`, so the pinned tree stays clean). Edit the
  top-level copy only.

## 2. Policies

### Firmware (implemented in L2 only)

| ID | Rule | Implemented in | Enforced by |
|---|---|---|---|
| POLICY-NKRO | NKRO is a configuration decision: L1 supports it (`"nkro": true`), both variants disable it at build time (stock NKRO breaks the macOS Caps Lock / input-source toggle) | `users/falcon_kalam/rules.mk` `NKRO_ENABLE = no` + `users/falcon_kalam/config.h` `#error` guard | verifier (no NKRO report, flag or code); negative test. To lift: delete both, flip the verifier checks |
| POLICY-VIA | both variants are VIA-configurable | `users/falcon_kalam/rules.mk` | verifier |
| POLICY-PARITY | both variants provide every stock feature: console, Command, LTO, on top of L1 (stock RGB effects and defaults are superseded by POLICY-RGB). **Ship** restricts Command to H D X K V S B (decided 2026-10-26); dev keeps the full set | `users/falcon_kalam/rules.mk`, `users/falcon_kalam/config.h`, `ship/config.h` | verifier; `artifacts/feature_parity.txt` |
| POLICY-VARIANTS | **one tree, two images.** Ship = daily use, no debug-only features. Dev = every debug feature that fits (FLASHREAD, `VIA_INSECURE`, full Command, `ACTION_DEBUG`, wear-leveling trace, scan rate, keycode strings), never the daily image. Shared code lives only in `users/falcon_kalam/`; no git branches are used for configuration | variant `rules.mk`/`config.h`; `#error` guards in `ship/keymap.c` | `build.sh` builds and verifies both every time; verifier profiles `release` (ship) / `dev`; negative tests inject each debug feature into ship |
| POLICY-RGB | the RGB module is a **status indicator, not decoration** (§2.1). Decorative effects stay in L1 and are disabled by L2 for every variant | `users/falcon_kalam/rgb_indicators.c`; `users/falcon_kalam/config.h` (`#undef` of every effect) | verifier: L1 image has all 43 effects, ship and dev have none, the indicator table is in both, the VIA JSON has no lighting menu; `#error` in `rgb_indicators.c` if an effect is defined |
| POLICY-FLASHREAD | **dev only**: read (never write) the bootloader, option bytes and F_SIZE/UID over raw HID. Removed from ship by the user's decision (2026-10-26) | `users/falcon_kalam/flashread.c`, compiled when `FALCON_KALAM_FLASHREAD = yes` | ship: `#error` guard + verifier (no `via_command_kb`); dev: verifier requires it |
| POLICY-HHKB | HHKB is the default everywhere: keymaps, first layout, VIA layout option 0 | `keyboard.json`, keymaps, `via/falcon_kalam.json` | verifier |

### 2.1 RGB policy (POLICY-RGB)

**Normal = all 4 LEDs white, full brightness.** Anything else means something.
States are QMK rgblight layers over the base; a higher priority wins where
they overlap. Channel A and channel B are two LEDs each, so one safety state
and one connection state can show together. The channel LED ranges are
defined in `rgb_indicators.h`, to be mapped to physical positions after the
LED-order test.

| Priority | State | Colour | LEDs | Trigger |
|---|---|---|---|---|
| 1 | bootloader (about to jump; kept while in DFU) | red | all | `shutdown_user(jump=true)` pushes the frame out; WS2812s latch it |
| 2 | settings were reset | orange, 3 blinks at boot | all | `eeconfig_init_user` (EEPROM init) |
| 3 | keystroke printout on (privacy warning) | magenta | A | debug + Shift+Shift+K |
| 4 | debug output on | yellow | A | Shift+Shift+D / X |
| 5 | USB not configured by a host | blue | B | USB state ≠ configured/suspended |
| 6 | non-default layer locked on | green | B | layer ≥ 2 active, or default layer ≠ 0 |
| — | Fn held, Caps Lock | no RGB change | — | Fn is momentary; Caps Lock has its own LED |
| — | host asleep | all off | all | `RGBLIGHT_SLEEP`; indicators are suppressed too |
| — | dev only (with FLASHREAD): request served | cyan blink | all | `falcon_kalam_rgb_activity()` |

Both variants behave the same. Every rgblight effect is compiled out, and the
base is enforced to static white on every main-loop pass, so old EEPROM values
and VIA commands can't change it. The VIA definition has no lighting menu.
Fn+Z switches the white base light off/on (persisted); indicators still show.
Fn+X–B do nothing. No separate LED test tools are needed:
- the indicator colours (red, yellow, magenta, blue, green) also check the WS2812 colour order;
- triggering a channel (Shift+Shift+D → channel A) shows which physical LEDs it uses.

Blue ("powered but not configured") is kept for: charge-only cables,
power-only ports (charger, power bank, some hubs and monitors), a host that
doesn't enumerate, and a short flash at every plug-in. **Kept** (user decision,
2026-10-26), accepting that it glows blue on a port that stays powered while the
computer is off.

### 2.2 Naming (POLICY-NAMING)

| Context | Name | Where |
|---|---|---|
| **Seen by an ordinary user** (brand) | **Falcon** | USB manufacturer and product strings (`keyboard.json` `manufacturer`, `keyboard_name`), the VIA definition's `name` |
| **Development only** | **falcon_kalam** (`FALCON_KALAM_` for macros) | QMK keyboard id and folders (`keyboards/falcon_kalam`, `userspace/keyboards/falcon_kalam`, `userspace/users/falcon_kalam`), image and VIA file names, C identifiers and macros, tool names, environment variables (`FALCON_KALAM_GCC_BIN`), dev docs prose about this unit |
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
(retired names appear nowhere else).

History (the only place retired names may appear): until 2026-10-26 the dev
id was `falcon_hs_r2`, the product string "Falcon HS R2", and the variant
keymaps `keymaps/falcon/` (ship) and `keymaps/falcon_dev` (dev); before that
the release keymap was `hhkb_via`.

### Project

| ID | Rule | Enforced by |
|---|---|---|
| POLICY-QMK-LATEST | `qmk_firmware` sits exactly on the newest upstream QMK release tag; being newer than stock is intended | `scripts/qmk_policy.sh` (offline in every build, fatal; `--online` before each flash) |
| POLICY-ORIGINALS | `originals/` is immutable evidence: mode 444, hashes in `SHA256SUMS`. New device dumps may be **added**, never changed | `build.sh` hash check |
| POLICY-REPRODUCIBLE | offline build, pinned toolchain and sources, identical bytes on rebuild on any day (`SKIP_VERSION=yes`, `SOURCE_DATE_EPOCH` = pinned QMK commit time) | `build.sh --twice`; the verifier checks the embedded build date |
| POLICY-EVIDENCE | hardware parameters come only from verified evidence; anything unproven is marked so, never invented | analysis tool (exact-address checks); status column in `hardware.md` |
| POLICY-READONLY-DEVICE | without an approval, only read the device: enumeration, DFU **upload**, whitelisted VIA queries | `falcon_kalam_probe.py` whitelist |
| POLICY-FLASH-GATE | flash only an L2 image (ship, or dev for testing) whose approval names the **variant and its SHA-256**, via DFU **alt 2**, then test Esc→DFU. The daily image is always ship | `operations.md` §2 |
| POLICY-OPEN-ISSUES | an open issue stays open until the user settles it explicitly | `problems.md` |
| POLICY-NAMING | §2.2 | verifier, `check_docs.py` |
| POLICY-DOCS | §4 below | `scripts/check_docs.py` (run by `build.sh`) |

## 3. File organisation

| Path | Content |
|---|---|
| `keyboards/falcon_kalam/` | L1: `keyboard.json` (MCU, bootloader, USB IDs, pins, COL2ROW, Caps LED A10 active-low, wear-leveling 2048/1024, WS2812 A1, rgblight 4 LEDs + all effects + sleep, layouts `LAYOUT_60_hhkb` (default) and `LAYOUT_all`); `config.h` (pinned 64 KB flash size); `rules.mk` + `ld/…falcon_kalam.ld` (app region `0x08002000–0x0800F7FF`, exports `__falcon_kalam_app_*`); `keymaps/default/` (HHKB); `readme.md` |
| `userspace/` | L2: `qmk.json` (targets `falcon_kalam:ship`, `falcon_kalam:dev`); `users/falcon_kalam/` (`rules.mk` shared policy + FLASHREAD switch, `config.h` shared guards and RGB layer settings, `falcon_kalam_keymap.h` the keymap, `rgb_indicators.c/.h` POLICY-RGB, `flashread.c`); `userspace/keyboards/falcon_kalam/keymaps/ship/` (ship: selective Command, debug guards) and `…/falcon_dev/` (dev: all debug features) |
| `via/falcon_kalam.json` | VIA definition (both variants): the stock JSON with HHKB as layout option 0, without the lighting menu (POLICY-RGB) |
| `qmk_firmware/` | submodule, pinned (POLICY-QMK-LATEST) |
| `bootloader/STM32duino-bootloader/` | MIT bootloader source, pinned submodule; reference only, never built or flashed |
| `originals/` | stock BIN, readback, VIA JSON, `device_dump_2026-10-26/` (bootloader, F_SIZE/UID, option bytes), `SHA256SUMS` |
| `scripts/` | `build.sh` (only build entry), `verify_firmware.py` (`--profile base\|release\|dev`), `negative_tests.sh`, `qmk_policy.sh`, `check_docs.py` |
| `tools/` | `analyze_vendor_firmware.py`, `feature_parity.py`, `falcon_kalam_probe.py` (read-only VIA probe), `usb_watch.py` (passive USB log), `disassemble_vendor.sh` |
| `artifacts/` | built images (`.bin` with DFU suffix, `.elf`, `.map`, cflags, `info_config.h`, `.sha256`), build/verification reports, device logs |
| `build/` | scratch, git-ignored |

## 4. Documentation (POLICY-DOCS)

One document per category. Add a document only when strictly required, and
record it here.

| # | File | Holds | Update rule |
|---|---|---|---|
| 1 | `docs/hardware.md` | hardware facts and conventions (pins, matrix, layout, lights, bootloader, memory) | when a fact is established; give its evidence |
| 2 | `docs/stock-firmware.md` | stock image capabilities and feature list, RE method | when new RE results arrive |
| 3 | `docs/policy.md` | this file: design, policies, file organisation, doc rules, design notes | when a rule or file layout changes |
| 4 | `docs/firmware.md` | features of our firmware, current release identity, what is verified on hardware | at a milestone (from devnotes) |
| 5 | `docs/devnotes.md` | **the only status document**: device state, work in progress, to-do, test results awaiting a milestone | most often. When an item reaches a milestone, write the result into `firmware.md` and **delete** it here |
| 6 | `docs/problems.md` | open risks, bugs, limitations; closed items as a one-line ledger | when found or resolved |
| 7 | `docs/improvements.md` | optional improvements, not committed to | when an idea arises; move it to devnotes when it is started |
| 8 | `docs/operations.md` | procedures: build, flash, tests, recovery, QMK update | when a procedure changes |

Rules: status (what is flashed, what is pending) appears **only** in
`devnotes.md`; the current release hash appears **only** in `firmware.md`.
Other docs link to them instead of repeating them. `README.md` is an index.
`scripts/check_docs.py` checks that exactly these docs exist, that every
referenced repo path exists, and that the hash in `firmware.md` matches
`artifacts/`.

## 5. Design notes (non-obvious code)

**`via_command_kb()` / `falcon_kalam_read_allowed()`** (`users/falcon_kalam/flashread.c`, dev only).
QMK's VIA layer calls `via_command_kb()` first for every raw-HID packet; if it
returns true, VIA skips its own dispatch and the handler must send the reply.
The handler claims only command `0xF0`: it decodes a big-endian address and a
length (≤ 24, so the reply fits in 32 bytes), checks the range and copies
bytes with `volatile` reads. Allowed ranges: `0x08000000–0x08001FFF`,
`0x1FFFF7E0–0x1FFFF7EF`, `0x1FFFF800–0x1FFFF80F`. `addr + len >= addr` rejects
wrap-around; a refused range is answered with length 0. `noinline` keeps the
symbol visible under LTO.

**Shared keymap (`users/falcon_kalam/falcon_kalam_keymap.h`).** The header *defines* the
`keymaps` array and is included exactly once, from each variant's `keymap.c`.
It can't live in a `.c` under `users/`: QMK's keymap introspection compiles
the variant's `keymap.c` itself and derives the layer count from that array.

**Selective Command (ship `config.h`).** Unwanted `MAGIC_KEY_*` commands are
re-pointed at keys a US HHKB doesn't have (`INTERNATIONAL_1`–`9`,
`LANGUAGE_1`), so LShift+RShift+that-key reaches no case in `command.c`'s
switch and types normally. Each needs a distinct key (duplicate cases don't
compile). Names such as `F13` are unusable: QMK defines `F13` as a pin macro
(GPIOF pin 13), which breaks the `KC_##key` paste. Layer switching is turned
off with `MAGIC_KEY_SWITCH_LAYER_WITH_{FKEYS,NKEYS} false`.

**`rgb_indicators.c`.** `keyboard_post_init_user` installs the layer table.
`housekeeping_task_user` polls the state sources (USB state, debug flags,
layer state) every main-loop pass. It calls `rgblight_set_layer_state()` only
for layers whose state changed, because each call schedules a redraw. It also
enforces the white base in ship. The USB-state callback is avoided because it
can run in interrupt context, and the debug flags have no callback.
`shutdown_user` sets the red layer and calls `rgblight_set()` directly, since
there is no later main-loop pass before the reset. `eeconfig_init_user` runs
before the LEDs exist, so it only sets a flag (keeping QMK's default
`eeconfig_update_user(0)`); the blink starts in `keyboard_post_init_user`.
Note: rgblight blinks share one repeat timer, so overlapping blinks interfere
(only possible in dev, during a FLASHREAD dump at boot). Layer changes are applied in `rgblight_task()`, which
exists because `RGBLIGHT_LAYERS` forces `RGBLIGHT_USE_TIMER` even with no
effects compiled.

**EEPROM placement.** ChibiOS describes the F103 flash as 128 × 1 KB sectors
(the xB registry). QMK drops the sectors at or above the flash size, then takes
sectors from the top until it has 2 KB, which gives `0x0800F800`. L1 pins the
flash size, so a GD32's F_SIZE cannot move it; the linker script reserves the
same 2 KB.

**`analyze_vendor_firmware.py`.** The stock image has no symbols. Each check
names an address and the exact instruction/data bytes expected there
(`Image.thumb()` compares objdump-style halfwords); it never searches for a
"best match". Pin tables are decoded from ChibiOS `ioline_t` (port base | pad).
The wear-leveling log is decoded by inverting each 16-bit cell and splitting
the 2-bit entry type from the address and value.

**`verify_firmware.py`.**
- Container: the DFU suffix (CRC32) is valid, and the payload equals `objcopy -O binary` of the ELF.
- Placement: every `LOAD` segment lies in `[0x08002000, 0x0800F800)`, and RAM in 20 KB.
- Vector table: SP = `__main_stack_end__`, reset = `Reset_Handler`, every vector points into the image.
- Profiles: base = NKRO present, no VIA. Release (ship) and dev = no NKRO (only ChibiOS's empty `send_nkro` stub may exist); VIA, console, Command and LTO present; keymap from the right variant directory. Ship = none of the dev features. Dev = all of them: the FLASHREAD symbol, the `-D` flags, and the trace strings of `ACTION_DEBUG`, `WEAR_LEVELING_DEBUG_OUTPUT` and the scan rate. `VIA_INSECURE` leaves no binary trace, so it is enforced only by the ship guard and its negative test.
- Instruction checks (`thumb2_find`, `movw_find`) match the opcode and immediate but ignore register fields, because each variant allocates registers differently. They are validated against the stock image, and against constants that must not match.
- Stock equivalence: pin tables, flash descriptor, clock literal and PLL instruction, BKP flags, JTAG-disable instruction, Caps LED config; VIA's default view = `LAYOUT_60_hhkb`.

**`feature_parity.py`.** `config_descriptor()` finds a USB configuration
descriptor by its header and accepts it only if walking the length-prefixed
sub-descriptors lands exactly on `wTotalLength` with the declared interface
count. `peripherals()` lists the peripheral blocks each image references, so
hardware outputs are compared as well.

**`falcon_kalam_probe.py`.** Uses the `hid` module of Homebrew's QMK CLI Python.
`send()` refuses everything outside a read-only whitelist (`0x01, 0x02, 0x08,
0x0C, 0x0D, 0x11, 0x12, 0xF0`; for `0x02` only uptime, layout options, matrix
and firmware version). `matrix()` reads row data from byte 3 on VIA protocol
≥ 12 (which echoes a row offset in byte 2) and from byte 2 on older ones.
`legacy_to_actual()` re-maps logs recorded with the earlier off-by-one decoder.
Current QMK returns an all-zero matrix to VIA unless built with `VIA_INSECURE`,
which only the dev variant sets. On ship the matrix is tested through the console.
