# Falcon project policy layer (on top of the normal base keyboard).
# Each line is a deliberate deviation from the base; see docs/architecture.md.

# POLICY-NKRO: NKRO off. With NKRO the vendor firmware breaks the macOS Caps
# Lock / input-source toggle (NK_OFF fixes it until replug). Remove this line and
# the guard in config.h once the upstream behaviour is fixed and re-tested.
NKRO_ENABLE = no

# POLICY-VIA: VIA configurator support (QMK keeps VIA out of keyboard folders).
VIA_ENABLE = yes
