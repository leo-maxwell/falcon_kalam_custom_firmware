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

#include "falcon_keymap.h"
