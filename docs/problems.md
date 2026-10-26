# Problems, risks and bugs

IDs are permanent: R = risk, B = bug, L = known limitation. **HR** = needs a
user decision. Closed items move to the ledger at the end as one line.

## Open

| ID | Problem | Status / impact | Mitigation / next step |
|---|---|---|---|
| B1 | **Caps Lock LED never lights** on image `9b61141e…`: the A10 indicator was not configured | fixed in the current release; not flashed (devnotes) | flash the current release and test |
| R13 | **No power-on DFU window.** If an image fails to boot or scan keys, DFU is reachable only through the pads or SWD | mitigated by procedure | flash only verified images; test Esc→DFU after every flash (`operations.md` §2) |
| R2 | **Newer QMK/ChibiOS on the GD32** (intended, POLICY-QMK-LATEST); GD32F103 ≠ STM32F103 | largely retired by the first flash (USB, scan, EEPROM init, DFU path) | open: long-term EEPROM writes; re-test after each QMK update |
| R6 | **WS2812 bit-bang timing / colour order** on hardware | untested | RGB test in devnotes; failure = wrong colours or flicker, no damage |
| R8 | **macOS Caps Lock / input-source fix** | user reports it working, also after a replug | close after the console Caps trace (devnotes) |
| R12 | **Console + Command in the ship image** (stock parity): Shift+Shift+D then K prints keystrokes to the HID console, readable by any host program | **decided** (user, 2026-10-26): ship keeps H D X K V S B; the risky combos (wipe settings, lock, layer switch, …) type normally | residual risk accepted; the keystroke printout needs two deliberate combos and resets on replug |
| R14 | **Full-white current**: 4 WS2812 at full white draw roughly 4 × 50–60 mA (typical datasheet values, not measured) on top of the MCU; within the 500 mA USB budget, but continuous | estimate only | measure the module current (I4); `RGBLIGHT_LIMIT_VAL` in L2 if needed |
| R15 | **"Dark" is ambiguous** under POLICY-RGB: host asleep, DFU entered by Bootmagic, or white base switched off (Fn+Z) | by design | DFU entered via Shift+Shift+B / `QK_BOOT` should stay red (to be verified); I19 |
| R9 | **GCC 8.5** (stock used 10.1) | low | I6 |
| R11 | **Wrong image flashed** (`falcon_hs_r2_default.bin` sits next to the release) | procedural | approval names the SHA-256 (`operations.md` §2) |

## Known limitations

| ID | Limitation |
|---|---|
| L1 | In the **ship** image the VIA key tester and `falcon_probe.py matrix` see an all-zero matrix (QMK hides it unless built with `VIA_INSECURE`). Use the dev image, or the console (Shift+Shift+X) |
| L2 | Fn+Esc = Power (HHKB convention) opens the macOS shut-down dialog |
| L3 | `QK_BOOT` is not on any default layer: use Esc at plug-in, Shift+Shift+B, or assign it in VIA |
| L4 | Command N ("NKRO toggle") does nothing in the dev image (no NKRO); in ship it types a normal N |
| L5 | A DFU download to alt 1 erases `0x08002000`; only alt 2 is used |
| L6 | The first boot of a new image with a different EEPROM layout resets the configuration (`SKIP_VERSION` fixes the VIA magic; clear EEPROM with Bootmagic if a layout changes) |

## Ledger (closed)

| ID | Closed | How |
|---|---|---|
| R1 | 2026-10-26 | bootloader dumped twice through FALCON_READ, identical; = upstream `generic-none`; RDP off; stored in `originals/device_dump_2026-10-26/` |
| R3 | 2026-10-26 | HSE 8 MHz: inferred from code, confirmed indirectly by working USB at ÷1.5 |
| R4 | 2026-10-26 | first-boot EEPROM reset accepted by the user; the backup held no user configuration anyway |
| R5 | 2026-10-26 | physical layout settled by the user as HHKB classic; probe confirmed 53/60 positions |
| R7 | 2026-10-26 | upstream bootloader source has a `bool` flag bug that would keep the device in DFU after a download; the installed prebuilt binary does not (the app started by itself) |
| B3 | 2026-10-26 | builds were reproducible only within one day: QMK Command embeds `__DATE__` (enabled by POLICY-PARITY on 2026-10-26). Fixed: `SOURCE_DATE_EPOCH` = pinned QMK commit time; the verifier checks the embedded date. The images `9b61141e…` (flashed) and `dd580803…` carry their build-day date; nothing else differs |
| R10 | 2026-10-26 | FLASHREAD (exposes the chip UID) removed from the ship image by user decision; it exists only in the dev image, enforced by POLICY-VARIANTS (guard, verifier, negative test) |
| B2 | 2026-10-26 | `falcon_probe.py matrix` read VIA ≥ 12 replies one byte early; fixed; `decode-log` re-maps the affected log |
