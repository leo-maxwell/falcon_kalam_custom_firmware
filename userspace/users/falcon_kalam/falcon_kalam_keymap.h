// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The falcon_kalam keymap shared by every variant (POLICY-VARIANTS). This unit is
// physically an HHKB classic (docs/hardware.md), so the keymap uses
// LAYOUT_60_hhkb. L1 keeps LAYOUT_all because the PCB also supports split
// backspace / left shift / spacebar; positions outside LAYOUT_60_hhkb stay
// KC_NO and can still be assigned in VIA.
//
// POLICY-HHKB: the three HHKB Professional Classic setting modes, copied from
// its User's Guide (P3PC-6661-01EN) and its keymap dump (docs/hardware.md §7).
// Layers in the order the Classic stores them (HHK, Mac, Win); the power-on
// mode comes from the virtual DIP switches (factory: HHK, as on the Classic).
// Keys a mode does not support are transparent: the HHKB sends the plain key
// there too. Mode keys and DIP switches: hhkb_modes.c. Codes as the real
// keyboard sends them: Power = Keyboard Power (0x66; the HHKB has no
// system-control report), Stop = 0x78, Clear = Keypad Num Lock and Clear
// (0x53), volume/mute/eject on the consumer page.
//
// This header DEFINES the keymaps array. Include it exactly once, from a
// variant's keymap.c: QMK's keymap introspection compiles keymap.c itself and
// derives the layer count from this array.

#pragma once

#include "hhkb_modes.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* HHK mode. Delete = Delete (DIP SW3 off), ◇ = Muhenkan / Henkan (International 5 / 4). */
    [_HHK] = LAYOUT_60_hhkb(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSLS, KC_GRV,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,          KC_DEL,
        KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,                   KC_ENT,
        KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,          MO(_HHK_FN),
                 KC_LALT, KC_INT5,                            KC_SPC,                             KC_INT4, KC_RALT
    ),
    [_HHK_FN] = LAYOUT_60_hhkb(
        _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_INS,  KC_DEL,
        _______, _______, _______, _______, _______, _______, _______, _______, KC_PSCR, KC_SCRL, KC_PAUS, KC_UP,   _______,          KC_BSPC,
        _______, _______, _______, _______, _______, _______, _______, _______, KC_HOME, KC_PGUP, KC_LEFT, KC_RGHT,                   _______,
        _______,          _______, _______, _______, _______, _______, _______, _______, KC_END,  KC_PGDN, KC_DOWN, _______,          _______,
                 _______, _______,                            _______,                            KC_STOP, _______
    ),
    /* Mac mode. Delete = Backspace (the HHKB ignores DIP SW3 here), ◇ = Command, Alt = Option. */
    [_MAC] = LAYOUT_60_hhkb(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSLS, KC_GRV,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,          KC_BSPC,
        KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,                   KC_ENT,
        KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,          MO(_MAC_FN),
                 KC_LALT, KC_LGUI,                            KC_SPC,                             KC_RGUI, KC_RALT
    ),
    [_MAC_FN] = LAYOUT_60_hhkb(
        KC_KB_POWER, KC_F1, KC_F2, KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_INS,  KC_DEL,
        KC_CAPS, _______, _______, _______, _______, _______, _______, _______, KC_PSCR, KC_SCRL, KC_PAUS, KC_UP,   _______,          KC_NUM,
        _______, KC_VOLD, KC_VOLU, KC_MUTE, KC_EJCT, _______, KC_PAST, KC_PSLS, KC_HOME, KC_PGUP, KC_LEFT, KC_RGHT,                   KC_PENT,
        _______,          _______, _______, _______, _______, _______, KC_PPLS, KC_PMNS, KC_END,  KC_PGDN, KC_DOWN, _______,          _______,
                 _______, _______,                            _______,                            _______, _______
    ),
    /* Win mode. Delete = Delete (DIP SW3 off, factory setting), ◇ = Windows key. */
    [_WIN] = LAYOUT_60_hhkb(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSLS, KC_GRV,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,          KC_DEL,
        KC_LCTL, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,                   KC_ENT,
        KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,          MO(_WIN_FN),
                 KC_LALT, KC_LGUI,                            KC_SPC,                             KC_RGUI, KC_RALT
    ),
    [_WIN_FN] = LAYOUT_60_hhkb(
        _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_INS,  KC_DEL,
        KC_CAPS, _______, _______, _______, _______, _______, _______, _______, KC_PSCR, KC_SCRL, KC_PAUS, KC_UP,   _______,          KC_BSPC,
        _______, _______, _______, _______, _______, _______, KC_PAST, KC_PSLS, KC_HOME, KC_PGUP, KC_LEFT, KC_RGHT,                   _______,
        _______,          _______, _______, _______, _______, _______, KC_PPLS, KC_PMNS, KC_END,  KC_PGDN, KC_DOWN, _______,          _______,
                 _______, _______,                            _______,                            _______, _______
    ),
};
// clang-format on
