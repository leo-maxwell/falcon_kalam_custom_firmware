# Architecture

## 1. Layers

```
qmk_firmware/              L0 upstream QMK, latest release, unmodified          (POLICY-QMK-LATEST)
keyboards/falcon_hs_r2/    L1 BASE: normal QMK keyboard = hardware facts only   (POLICY-LAYERING)
userspace/…/keymaps/falcon L2 POLICY: every project decision, tagged POLICY-*   → release image
```

* **L0** is a git submodule. It is never edited. The build stages L1 into it
  and hides the copy through the submodule's `info/exclude`.
* **L1** is what an upstream submission would look like. It passes `qmk lint`,
  has NKRO on (as shipped), no VIA, and an HHKB `default` keymap. It builds
  `falcon_hs_r2_default.bin`, a reference that is **never flashed**.
* **L2** uses QMK External Userspace (`QMK_USERSPACE=userspace/`). It builds the
  release `falcon_hs_r2_falcon.bin`. Policies are listed in `docs/policy.md`;
  `grep -rn POLICY- userspace` finds every one.

Rule: a hardware fact goes in L1, a choice goes in L2. When upstream fixes
something (e.g. NKRO on macOS), only L2 changes.

## 2. File map

### Firmware source

| File | Layer | Purpose |
|---|---|---|
| `keyboards/falcon_hs_r2/keyboard.json` | L1 | MCU `STM32F103` + `stm32duino` bootloader; USB IDs; matrix pins, `COL2ROW`; features (bootmagic, extrakey, mousekey, nkro, rgblight); wear-leveling 2048/1024; WS2812 on A1; rgblight 4 LEDs, all effects, sleep; layouts `LAYOUT_60_hhkb` (first = default) and `LAYOUT_all` (63 VIA positions) |
| `keyboards/falcon_hs_r2/config.h` | L1 | `WEAR_LEVELING_EFL_FLASH_SIZE = 64 KB` (EEPROM placement independent of GD32 F_SIZE) |
| `keyboards/falcon_hs_r2/rules.mk` | L1 | selects the keyboard-local linker script |
| `keyboards/falcon_hs_r2/ld/STM32F103x8_stm32duino_falcon_hs_r2.ld` | L1 | memory map: app `0x08002000–0x0800F7FF`. Excludes the bootloader and the 2 KB EEPROM, so an oversized image fails to link. Exports `__falcon_app_start/limit__` for the verifier |
| `keyboards/falcon_hs_r2/keymaps/default/keymap.c` | L1 | native HHKB base + HHKB Fn layer (`LAYOUT_60_hhkb`) |
| `keyboards/falcon_hs_r2/readme.md` | L1 | upstream-style readme |
| `userspace/qmk.json` | L2 | userspace manifest; build target `falcon_hs_r2:falcon` |
| `userspace/…/falcon/rules.mk` | L2 | POLICY-NKRO (`NKRO_ENABLE = no`), POLICY-VIA, POLICY-PARITY (console, Command, LTO) |
| `userspace/…/falcon/config.h` | L2 | POLICY-NKRO guard (`#error` if `NKRO_ENABLE` is defined); POLICY-PARITY stock RGB defaults (rainbow swirl, brightness 127) |
| `userspace/…/falcon/keymap.c` | L2 | HHKB keymap on `LAYOUT_60_hhkb` (this unit is an HHKB classic), 4 layers as stock; POLICY-FLASHREAD handler |
| `via/falcon_hs_r2.json` | L2 | VIA definition: vendor JSON with HHKB as layout option 0 |

### Tooling

| File | Purpose |
|---|---|
| `scripts/build.sh` | the only build entry point; offline; logs to `artifacts/build.log` (§3) |
| `scripts/verify_firmware.py` | post-build gate with `--profile base\|release` (§4) |
| `scripts/negative_tests.sh` | proves the NKRO guard and the flash limit both reject bad builds |
| `scripts/qmk_policy.sh` | POLICY-QMK-LATEST: `--offline`, `--online`, `--update` |
| `tools/analyze_vendor_firmware.py` | re-derives every hardware fact from `originals/` (§4) |
| `tools/feature_parity.py` | stock vs release table from the binaries (§4) → `artifacts/feature_parity.txt` |
| `tools/falcon_probe.py` | read-only USB probe over VIA raw HID (§5); `decode-log` re-maps logs from the pre-fix decoder |
| `tools/usb_watch.py` | passive USB arrival/departure log with timestamps (measures the bootloader's power-on DFU window) |
| `tools/disassemble_vendor.sh` | objdump listing of the vendor app (`build/`, git-ignored) |

### Data and evidence

| Path | Content |
|---|---|
| `originals/` | vendor BIN, DFU readback, VIA JSON, `SHA256SUMS`: read-only, never modified |
| `bootloader/STM32duino-bootloader/` | MIT bootloader source, pinned submodule: reference only, never built or flashed |
| `artifacts/` | release and base images (`.bin` with DFU suffix, `.elf`, `.map`, cflags, `info_config.h`, `.sha256`), logs, reports, device inspection |
| `docs/` | `policy`, `architecture`, `parity`, `hardware`, `bootloader`, `layout` (L-1, settled), `rgb-crosscheck`, `build`, `recovery`, `preflash-checklist`, `risks` |

## 3. Build flow (`scripts/build.sh`)

1. Pin the toolchain on `PATH`. Export `SKIP_VERSION=yes` (fixed version and date strings, needed for reproducibility), `LC_ALL=C`, `TZ=UTC` and `QMK_USERSPACE`.
2. Check the originals' hashes, then re-run the vendor analysis (fatal on any FAIL).
3. Check that `qmk_firmware` HEAD equals the gitlink, the policy holds (`qmk_policy.sh --offline`), and the submodules and tree are clean.
4. `rsync` L1 into `qmk_firmware/keyboards/`, then run `qmk lint` with upstream rules.
5. Build base and release from clean. Assert that the release keymap path is inside `userspace/`.
6. Optionally (`--twice`) rebuild the release and require an identical SHA-256.
7. Verify both images, then write the parity table.

## 4. Non-obvious functions

**`via_command_kb()` / `falcon_read_allowed()`** (`userspace/…/keymap.c`, POLICY-FLASHREAD).
QMK's VIA layer calls `via_command_kb()` first for every raw-HID packet. If it
returns true, VIA skips its own dispatch and the handler must send the reply.
The handler only claims command `0xF0`: it decodes a big-endian address and a
length (≤ 24, so the reply fits in 32 bytes), checks the range, and copies
bytes with a `volatile` read. Allowed ranges: bootloader `0x08000000–0x08001FFF`,
F_SIZE+UID `0x1FFFF7E0–0x1FFFF7EF`, option bytes `0x1FFFF800–0x1FFFF80F`.
`addr + len >= addr` rejects 32-bit wrap-around. A refused range is answered
with length 0. `noinline` keeps the symbol visible under LTO for the verifier.

**EEPROM placement (QMK `backing_store_init`, configured by L1).** ChibiOS
describes the F103 flash as 128 × 1 KB sectors (the xB registry, larger than
the chip). QMK drops sectors at or above the detected flash size, then takes
sectors from the top until it has 2 KB, which gives `0x0800F800`. Pinning
`WEAR_LEVELING_EFL_FLASH_SIZE` replaces the F_SIZE register read, so placement
cannot move if a GD32 reports F_SIZE differently. The linker script reserves
the same 2 KB.

**`tools/analyze_vendor_firmware.py`.** The vendor image has no symbols. Each
check names an address found by disassembly and the exact instruction or data
bytes expected there (`Image.thumb()` compares objdump-style halfwords). The
tool never searches heuristically for a "best match", so a wrong assumption
fails loudly. Pin tables are decoded from ChibiOS `ioline_t` (port base | pad).
The wear-leveling log is decoded by inverting each 16-bit cell (the flash
erases to 1) and splitting the 2-bit entry type from the address and value.

**`scripts/verify_firmware.py`.**
- Container: strip and validate the DFU suffix (CRC32 per DFU 1.1), then require the payload to equal `objcopy -O binary` of the ELF.
- Placement: every ELF `LOAD` segment with file data must lie in `[0x08002000, 0x0800F800)`, and RAM segments in 20 KB.
- Vector table: SP equals `__main_stack_end__`, the reset vector is `Reset_Handler`, and every vector points into the image.
- Profiles: `release` requires no NKRO report/flag/code (only the empty `send_nkro` stub ChibiOS always defines), VIA, console, Command, LTO, the userspace keymap and the FLASHREAD symbol. `base` requires NKRO present and VIA absent.
- Vendor equivalence: identical pin tables, flash descriptor, `RCC_CFGR` literal, PLL instruction, BKP flags and the AFIO JTAG-disable instruction. VIA's default view (all options 0) must equal `LAYOUT_60_hhkb`.

**`tools/feature_parity.py` `config_descriptor()`.** It finds the USB
configuration descriptor by its header pattern, then walks the length-prefixed
sub-descriptors. A match only counts if the walk lands exactly on
`wTotalLength` with the declared number of interfaces. That rejects false
matches in code or data.

## 5. Device access (`tools/falcon_probe.py`)

It runs on the Python bundled with Homebrew's QMK CLI (it provides `hid`). It
opens the `0xFF60/0x61` interface and sends 32-byte VIA packets. `send()`
refuses any command outside a read-only whitelist (`0x01, 0x02, 0x08, 0x0C,
0x0D, 0x11, 0x12, 0xF0`), and any `0x02` sub-id other than uptime, layout
options, matrix state or firmware version. Commands: `info` (protocol, layers,
macros, current RGB state; handles both pre-12 and ≥ 12 lighting protocols),
`dump-keymap`, `matrix` (polls the switch-matrix state about every 10 ms and prints
`row,col` on each press), `dump-bootloader` (FALCON_READ, read twice and
compared).
