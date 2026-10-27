// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-HHKB: the HHKB Professional Classic setting modes and DIP switches
// (docs/policy.md §2.3, reference in docs/hardware.md §7). Each mode is a base
// layer plus its own Fn layer, in the order the Classic stores them.

#pragma once

#include QMK_KEYBOARD_H

enum falcon_kalam_layers {
    _HHK, // HHK mode (factory setting, as on the Classic)
    _HHK_FN,
    _MAC, // Mac mode
    _MAC_FN,
    _WIN, // Win mode
    _WIN_FN,
};

#define FALCON_KALAM_BASE_LAYERS ((layer_state_t)((1u << _HHK) | (1u << _MAC) | (1u << _WIN)))
#define FALCON_KALAM_FN_LAYERS ((layer_state_t)((1u << _HHK_FN) | (1u << _MAC_FN) | (1u << _WIN_FN)))

// Virtual DIP switches (bit n = SW(n+1) on), as on the Classic's underside.
#define FALCON_KALAM_SW1 (1u << 0) // with SW2: setting mode (OFF/OFF HHK, ON/OFF Win, OFF/ON Mac)
#define FALCON_KALAM_SW2 (1u << 1)
#define FALCON_KALAM_SW3 (1u << 2) // Delete key sends Backspace (not in Mac mode)
#define FALCON_KALAM_SW4 (1u << 3) // left ◇ becomes Fn
#define FALCON_KALAM_SW5 (1u << 4) // swap Alt and ◇
#define FALCON_KALAM_DIP_FACTORY 0 // all off, as the Classic ships: HHK mode

void falcon_kalam_hhkb_init(void);                                     // keyboard_post_init_user: power-on mode from SW1/SW2
bool falcon_kalam_hhkb_process(uint16_t keycode, keyrecord_t *record); // process_record_user; false = consumed
