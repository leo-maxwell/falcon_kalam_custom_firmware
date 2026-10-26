// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-HHKB: setting modes and DIP switches of the HHKB Professional Classic
// (docs/policy.md §2.3, docs/hardware.md §7).
//
// As on the Classic: SW1/SW2 select the mode at power-on; Fn+Control+M
// switches to Mac mode and Fn+Control+W to Win mode (to HHK mode when SW1/SW2
// select HHK), until the next power-on; SW3-SW5 change keys on top of the
// keymap, as the Classic's DIP switches act on its stored keymap. All are off
// from the factory (HHK mode). This unit has no switches, so they are
// virtual: saved in EEPROM and flipped with Fn+Control+Shift+1..5. As on the
// Classic, a flip takes effect at the next plug-in.
//
// Additions (the Classic has no display): Fn+Control+Shift+0 and the mode keys
// show the settings on the RGB module.

#include "hhkb_modes.h"
#include "rgb_indicators.h"
#include "user_config.h"

static uint8_t dip; // the virtual DIP switches in effect: as saved at power-on

// SW1/SW2 -> base layer. ON/ON is not a setting the manual defines; it acts as HHK mode.
static uint8_t dip_mode_of(uint8_t switches) {
    switch (switches & (FALCON_KALAM_SW1 | FALCON_KALAM_SW2)) {
        case FALCON_KALAM_SW1:
            return _WIN;
        case FALCON_KALAM_SW2:
            return _MAC;
        default:
            return _HHK;
    }
}

static uint8_t dip_mode(void) {
    return dip_mode_of(dip);
}

static uint8_t current_mode(void) {
    return get_highest_layer(default_layer_state);
}

static void set_mode(uint8_t base_layer) {
    default_layer_set((layer_state_t)1 << base_layer);
}

// Packs a mode and SW3-SW5 for the settings display (rgb_indicators.h).
static uint8_t settings(uint8_t base_layer, uint8_t switches) {
    return (base_layer / 2) | (((switches >> 2) & 7) << 2);
}

// Fn+Control+Shift+0: the settings in effect, then those after the next plug-in.
static void show_settings(void) {
    falcon_kalam_user_config_t cfg = {.raw = eeconfig_read_user()};
    falcon_kalam_rgb_show_settings(settings(current_mode(), dip), settings(dip_mode_of(cfg.dip), cfg.dip));
}

// Mode key (Fn+Control+M/W): switch until the next plug-in, show the settings.
static void mode_key(uint8_t base_layer) {
    set_mode(base_layer);
    show_settings();
}

void falcon_kalam_hhkb_init(void) {
    falcon_kalam_user_config_t cfg = {.raw = eeconfig_read_user()};
    dip                            = cfg.dip;
    set_mode(dip_mode());
}

// Flips the saved switch only; `dip` keeps the power-on setting until the
// next plug-in, as the Classic reads its switches only at power-on.
static void flip_dip(uint8_t index) {
    falcon_kalam_user_config_t cfg = {.raw = eeconfig_read_user()};
    cfg.dip ^= 1u << index;
    eeconfig_update_user(cfg.raw);
    falcon_kalam_rgb_dip_feedback(cfg.dip & (1u << index));
}

// What a held key was turned into at press, so the release matches it even if
// the mode changes in between. 0 = not substituted.
#define HELD_AS_FN 0x8000u // | Fn layer: the key acts as Fn (SW4)
static uint16_t held[MATRIX_ROWS][MATRIX_COLS];

// SW3-SW5 applied to a base-layer keycode. The ◇ keys send GUI in Mac/Win
// mode and Muhenkan (left) / Henkan (right) in HHK mode.
static uint16_t dip_substitute(uint16_t keycode) {
    uint8_t mode       = current_mode();
    bool    left_dia   = keycode == KC_LGUI || keycode == KC_INT5;
    bool    right_dia  = keycode == KC_RGUI || keycode == KC_INT4;
    if ((dip & FALCON_KALAM_SW4) && left_dia) {
        return HELD_AS_FN | (mode + 1); // each mode's Fn layer follows its base layer
    }
    if (dip & FALCON_KALAM_SW5) {
        if (keycode == KC_LALT) return mode == _HHK ? KC_INT5 : KC_LGUI;
        if (left_dia) return KC_LALT;
        if (keycode == KC_RALT) return mode == _HHK ? KC_INT4 : KC_RGUI;
        if (right_dia) return KC_RALT;
    }
    if ((dip & FALCON_KALAM_SW3) && keycode == KC_DEL && mode != _MAC) {
        return KC_BSPC;
    }
    return 0;
}

bool falcon_kalam_hhkb_process(uint16_t keycode, keyrecord_t *record) {
    keypos_t key = record->event.key;
    if (key.row >= MATRIX_ROWS || key.col >= MATRIX_COLS) {
        return true; // not a matrix key (combo, encoder)
    }

    if (!record->event.pressed) {
        uint16_t sub = held[key.row][key.col];
        if (!sub) {
            return true;
        }
        held[key.row][key.col] = 0;
        if (sub & HELD_AS_FN) {
            layer_off(sub & 0xFF);
        } else {
            unregister_code16(sub);
        }
        return false;
    }

    // Fn+Control(+Shift)+key, judged by the key's base-layer legend: on the
    // Fn layers M is keypad minus and 0-5 are F10 and F1-F5.
    if ((layer_state & FALCON_KALAM_FN_LAYERS) && (get_mods() & MOD_MASK_CTRL)) {
        uint16_t legend = keymap_key_to_keycode(current_mode(), key);
        if (get_mods() & MOD_MASK_SHIFT) {
            if (legend >= KC_1 && legend <= KC_5) {
                flip_dip(legend - KC_1);
                return false;
            }
            if (legend == KC_0) {
                show_settings();
                return false;
            }
        } else if (legend == KC_M) {
            mode_key(_MAC);
            return false;
        } else if (legend == KC_W) {
            mode_key(dip_mode() == _HHK ? _HHK : _WIN);
            return false;
        }
    }

    // DIP switches act on base-layer keys only (Fn+` stays Delete, for instance).
    if (!(((layer_state_t)1 << layer_switch_get_layer(key)) & FALCON_KALAM_BASE_LAYERS)) {
        return true;
    }
    uint16_t sub = dip_substitute(keycode);
    if (!sub) {
        return true;
    }
    held[key.row][key.col] = sub;
    if (sub & HELD_AS_FN) {
        layer_on(sub & 0xFF);
    } else {
        register_code16(sub);
    }
    return false;
}
