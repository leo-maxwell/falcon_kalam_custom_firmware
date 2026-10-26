# falcon_kalam configuration shared by every variant (POLICY-VARIANTS):
#   keymaps/ship      ship image (daily use)
#   keymaps/dev       dev image (all debug features)
# Each variant's rules.mk sets USER_NAME := falcon_kalam so QMK includes this file
# (after the variant's rules.mk, before feature processing). See docs/policy.md.

# POLICY-NKRO: NKRO off. With NKRO the stock firmware breaks the macOS Caps
# Lock / input-source toggle (NK_OFF fixes it until replug). Remove this line and
# the guard in config.h once the upstream behaviour is fixed and re-tested.
NKRO_ENABLE = no

# POLICY-VIA: VIA configurator support (QMK keeps VIA out of keyboard folders).
VIA_ENABLE = yes

# POLICY-PARITY: stock firmware options (its OPTIONS string lists BOOTMAGIC
# MOUSEKEY EXTRAKEY CONSOLE COMMAND NKRO LTO). Bootmagic/mousekey/extrakey come
# from the base; these three are added here, NKRO is excluded above. The ship
# variant restricts Command to a safe subset (its config.h).
CONSOLE_ENABLE = yes   # HID console (usage page 0xFF31), read with `qmk console`
COMMAND_ENABLE = yes   # LShift+RShift+<key> magic commands
LTO_ENABLE = yes       # link-time optimisation

# POLICY-RGB: status-indicator module (both variants).
SRC += rgb_indicators.c

# POLICY-FLASHREAD: read-only bootloader/option-byte/UID dump over raw HID.
# Only a variant that sets FALCON_KALAM_FLASHREAD = yes gets it (dev); the ship
# variant's keymap.c refuses to build with it.
ifeq ($(strip $(FALCON_KALAM_FLASHREAD)), yes)
    OPT_DEFS += -DFALCON_KALAM_FLASHREAD_ENABLE
    SRC += flashread.c
endif
