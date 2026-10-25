// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// VIA keymap. Same HHKB base and Fn layers as the default keymap, laid out on
// LAYOUT_all so every matrix position named in the VIA definition has a sane
// default whichever layout option (split backspace / left shift / spacebar)
// the board is built with:
//   - 2u backspace  : the 2u key sits on 0,14 and sends `; remap it in VIA.
//   - split L-shift : the extra 1u key at 3,1 is a second Left Shift.
//   - split space   : 4,5 / 4,7 / 4,9 all send Space.
// Layers 2 and 3 are left transparent for VIA (DYNAMIC_KEYMAP_LAYER_COUNT 4).

#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _FN,
    _L2,
    _L3,
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT_all(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSLS, KC_GRV,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,          KC_BSPC,
        KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,                   KC_ENT,
        KC_LSFT, KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,          MO(_FN),
                 KC_LALT, KC_LGUI,          KC_SPC,           KC_SPC,           KC_SPC,           KC_RGUI, KC_RALT
    ),
    [_FN] = LAYOUT_all(
        KC_PWR,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_INS,  KC_DEL,
        KC_CAPS, _______, _______, _______, _______, _______, _______, _______, KC_PSCR, KC_SCRL, KC_PAUS, KC_UP,   _______,          _______,
        _______, KC_VOLD, KC_VOLU, KC_MUTE, _______, _______, KC_PAST, KC_PSLS, KC_HOME, KC_PGUP, KC_LEFT, KC_RGHT,                   KC_PENT,
        _______, _______, UG_TOGG, UG_NEXT, UG_HUEU, UG_SATU, UG_VALU, KC_PPLS, KC_PMNS, KC_END,  KC_PGDN, KC_DOWN, _______,          _______,
                 _______, _______,          _______,          _______,          _______,          _______, _______
    ),
    [_L2] = LAYOUT_all(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                   _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
                 _______, _______,          _______,          _______,          _______,          _______, _______
    ),
    [_L3] = LAYOUT_all(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                   _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
                 _______, _______,          _______,          _______,          _______,          _______, _______
    ),
};
// clang-format on
