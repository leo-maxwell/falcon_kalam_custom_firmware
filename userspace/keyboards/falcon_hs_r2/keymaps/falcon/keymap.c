// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ship variant (POLICY-VARIANTS). The keymap is shared with the dev variant.

#include QMK_KEYBOARD_H

// POLICY-VARIANTS: debug-only features must never reach the ship image.
#if defined(FALCON_FLASHREAD_ENABLE)
#    error "POLICY-VARIANTS: FLASHREAD is dev-only (set FALCON_FLASHREAD only in keymaps/falcon_dev)"
#endif
#if defined(VIA_INSECURE)
#    error "POLICY-VARIANTS: VIA_INSECURE is dev-only"
#endif
#if defined(KEYCODE_STRING_ENABLE) || defined(DEBUG_MATRIX_SCAN_RATE) || defined(ACTION_DEBUG) || defined(WEAR_LEVELING_DEBUG_OUTPUT)
#    error "POLICY-VARIANTS: debug traces (keycode strings, scan rate, action, wear-leveling) are dev-only"
#endif

// POLICY-RGB: the ship image has no decorative effects and no dev RGB tools.
#if defined(FALCON_RGB_DEV) || defined(RGBLIGHT_EFFECT_ALTERNATING) || defined(RGBLIGHT_EFFECT_BREATHING) ||         \
    defined(RGBLIGHT_EFFECT_CHRISTMAS) || defined(RGBLIGHT_EFFECT_KNIGHT) || defined(RGBLIGHT_EFFECT_RAINBOW_MOOD) || \
    defined(RGBLIGHT_EFFECT_RAINBOW_SWIRL) || defined(RGBLIGHT_EFFECT_RGB_TEST) || defined(RGBLIGHT_EFFECT_SNAKE) ||  \
    defined(RGBLIGHT_EFFECT_STATIC_GRADIENT) || defined(RGBLIGHT_EFFECT_TWINKLE)
#    error "POLICY-RGB: decorative RGB effects and dev RGB tools are dev-only"
#endif

#include "falcon_keymap.h"
