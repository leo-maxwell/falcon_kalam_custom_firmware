// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// falcon_kalam configuration shared by every variant (POLICY-VARIANTS), on top of
// the normal base keyboard (keyboards/falcon_kalam). See docs/policy.md.

#pragma once

// POLICY-NKRO: rules.mk sets NKRO_ENABLE = no. This guard stops the build if
// anything (a command-line NKRO_ENABLE=yes, a stray rules.mk) re-enables it.
// Remove together with the rules.mk line when lifting the policy.
#if defined(NKRO_ENABLE)
#    error "POLICY-NKRO: NKRO must stay disabled in the falcon_kalam configuration (macOS Caps Lock / input-source issue)"
#endif

// Release number of the ship and dev images, readable over VIA
// (`falcon_kalam_probe.py info`). Bump it whenever the images change; must
// equal the release in docs/firmware.md (scripts/check_docs.py).
#define VIA_FIRMWARE_VERSION 3

// POLICY-HHKB: HHK, Mac and Win mode, each a base layer and an Fn layer
// (falcon_kalam_keymap.h). L1's EEPROM is sized for six layers plus VIA
// macros (docs/policy.md §5).
#define DYNAMIC_KEYMAP_LAYER_COUNT 6

// POLICY-RGB: the RGB module is a status indicator (rgb_indicators.c). The base
// is static white at the brightness cap; indicator states are rgblight layers.
// Supersedes the stock rgblight defaults (rainbow swirl, brightness 127).
// Brightness cap: a quarter of full scale (255). Full scale and half (127)
// were too bright to look at; rgblight clamps every colour, the base and the
// indicators, to this.
#define RGBLIGHT_LIMIT_VAL 63
#define RGBLIGHT_LAYERS
#define RGBLIGHT_LAYERS_OVERRIDE_RGB_OFF // indicators show even with the base light off
#define RGBLIGHT_LAYER_BLINK             // one-off events (settings reset, dev activity)
#define RGBLIGHT_MAX_LAYERS 16
#define RGBLIGHT_DEFAULT_MODE RGBLIGHT_MODE_STATIC_LIGHT
#define RGBLIGHT_DEFAULT_HUE 0
#define RGBLIGHT_DEFAULT_SAT 0
#define RGBLIGHT_DEFAULT_VAL RGBLIGHT_LIMIT_VAL

// POLICY-RGB: L1 keeps every decorative effect (normal QMK keyboard, stock
// parity); the configuration layer disables them for all variants.
#undef RGBLIGHT_EFFECT_ALTERNATING
#undef RGBLIGHT_EFFECT_BREATHING
#undef RGBLIGHT_EFFECT_CHRISTMAS
#undef RGBLIGHT_EFFECT_KNIGHT
#undef RGBLIGHT_EFFECT_RAINBOW_MOOD
#undef RGBLIGHT_EFFECT_RAINBOW_SWIRL
#undef RGBLIGHT_EFFECT_RGB_TEST
#undef RGBLIGHT_EFFECT_SNAKE
#undef RGBLIGHT_EFFECT_STATIC_GRADIENT
#undef RGBLIGHT_EFFECT_TWINKLE
