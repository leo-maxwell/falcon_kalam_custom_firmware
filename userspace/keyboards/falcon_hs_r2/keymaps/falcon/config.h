// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Falcon project policy layer: configuration on top of the normal base
// keyboard (keyboards/falcon_hs_r2). See docs/architecture.md.

#pragma once

// POLICY-NKRO: rules.mk sets NKRO_ENABLE = no. This guard stops the build if
// anything (a command-line NKRO_ENABLE=yes, a stray rules.mk) re-enables it.
// Remove together with the rules.mk line when lifting the policy.
#if defined(NKRO_ENABLE)
#    error "POLICY-NKRO: NKRO must stay disabled in the falcon keymap (macOS Caps Lock / input-source issue)"
#endif

// POLICY-PARITY: stock rgblight defaults, read from the stock firmware right
// after its EEPROM reset (tools/falcon_probe.py info, 2026-10-26): mode 9
// (rainbow swirl), brightness 127, hue 0, sat 255, speed 0. Hue/sat/speed equal
// QMK's defaults. No brightness cap: the stock code has no RGBLIGHT_LIMIT_VAL clamp.
#define RGBLIGHT_DEFAULT_MODE RGBLIGHT_MODE_RAINBOW_SWIRL
#define RGBLIGHT_DEFAULT_VAL 127
