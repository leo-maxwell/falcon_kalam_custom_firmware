// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-HHKB: switching between the HHKB setting modes (docs/policy.md).
//
// The HHKB Classic switches with Fn+Control+W (Win mode) and Fn+Control+M
// (Mac mode), pressed in that order; its DIP switches pick the power-on mode.
// Without DIP switches the mode is stored instead (persistent default layer),
// and Fn+Control+H is added to reach HHK mode.

#include "hhkb_modes.h"

bool falcon_kalam_hhkb_mode_process(keyrecord_t *record) {
    if (!record->event.pressed || !(layer_state & FALCON_KALAM_FN_LAYERS) || !(get_mods() & MOD_MASK_CTRL)) {
        return true;
    }
    // Judge the key by its base-layer legend: on the Fn layers M and H are
    // keypad keys in Mac/Win mode.
    uint8_t mode;
    switch (keymap_key_to_keycode(get_highest_layer(default_layer_state), record->event.key)) {
        case KC_M:
            mode = _MAC;
            break;
        case KC_W:
            mode = _WIN;
            break;
        case KC_H:
            mode = _HHK;
            break;
        default:
            return true;
    }
    // The held Fn layer belongs to the old mode until Fn is released; its MO()
    // release still turns it off (QMK caches the source layer of each press).
    set_single_persistent_default_layer(mode);
    return false;
}
