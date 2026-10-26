// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The Falcon keymap shared by every variant (POLICY-VARIANTS). This unit is
// physically an HHKB classic (docs/hardware.md), so the keymap uses
// LAYOUT_60_hhkb. L1 keeps LAYOUT_all because the PCB also supports split
// backspace / left shift / spacebar; positions outside LAYOUT_60_hhkb stay
// KC_NO and can still be assigned in VIA. Layers 2 and 3 are transparent for
// VIA (DYNAMIC_KEYMAP_LAYER_COUNT 4, as stock).
//
// This header DEFINES the keymaps array. Include it exactly once, from a
// variant's keymap.c: QMK's keymap introspection compiles keymap.c itself and
// derives the layer count from this array.

#pragma once

// Fn+Z..B (unused on a real HHKB). Ship (POLICY-RGB): Fn+Z switches the white
// base light on/off (indicators still show); X..B do nothing. Dev: effect
// controls for LED tests.
#ifdef FALCON_RGB_DEV
#    define FALCON_FN_Z UG_TOGG
#    define FALCON_FN_X UG_NEXT
#    define FALCON_FN_C UG_HUEU
#    define FALCON_FN_V UG_SATU
#    define FALCON_FN_B UG_VALU
#else
#    define FALCON_FN_Z UG_TOGG
#    define FALCON_FN_X _______
#    define FALCON_FN_C _______
#    define FALCON_FN_V _______
#    define FALCON_FN_B _______
#endif

enum layers {
    _BASE,
    _FN,
    _L2,
    _L3,
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT_60_hhkb(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSLS, KC_GRV,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,          KC_BSPC,
        KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,                   KC_ENT,
        KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,          MO(_FN),
                 KC_LALT, KC_LGUI,                            KC_SPC,                             KC_RGUI, KC_RALT
    ),
    [_FN] = LAYOUT_60_hhkb(
        KC_PWR,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_INS,  KC_DEL,
        KC_CAPS, _______, _______, _______, _______, _______, _______, _______, KC_PSCR, KC_SCRL, KC_PAUS, KC_UP,   _______,          _______,
        _______, KC_VOLD, KC_VOLU, KC_MUTE, _______, _______, KC_PAST, KC_PSLS, KC_HOME, KC_PGUP, KC_LEFT, KC_RGHT,                   KC_PENT,
        _______,          FALCON_FN_Z, FALCON_FN_X, FALCON_FN_C, FALCON_FN_V, FALCON_FN_B, KC_PPLS, KC_PMNS, KC_END,  KC_PGDN, KC_DOWN, _______,          _______,
                 _______, _______,                            _______,                            _______, _______
    ),
    [_L2] = LAYOUT_60_hhkb(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                   _______,
        _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
                 _______, _______,                            _______,                            _______, _______
    ),
    [_L3] = LAYOUT_60_hhkb(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                   _______,
        _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
                 _______, _______,                            _______,                            _______, _______
    ),
};
// clang-format on
