# Falcon project policy layer (on top of the normal base keyboard).
# Each block is a deliberate deviation from the base; see docs/architecture.md.

# POLICY-NKRO: NKRO off. With NKRO the stock firmware breaks the macOS Caps
# Lock / input-source toggle (NK_OFF fixes it until replug). Remove this line and
# the guard in config.h once the upstream behaviour is fixed and re-tested.
NKRO_ENABLE = no

# POLICY-VIA: VIA configurator support (QMK keeps VIA out of keyboard folders).
VIA_ENABLE = yes

# POLICY-PARITY: stock firmware options (its OPTIONS string lists BOOTMAGIC
# MOUSEKEY EXTRAKEY CONSOLE COMMAND NKRO LTO). Bootmagic/mousekey/extrakey come
# from the base; these three are added here, NKRO is excluded above.
CONSOLE_ENABLE = yes   # HID console (usage page 0xFF31), read with `qmk console`
COMMAND_ENABLE = yes   # LShift+RShift+<key> magic commands, e.g. B = bootloader
LTO_ENABLE = yes       # link-time optimisation; saves ~5.8 KB
