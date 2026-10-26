// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-HHKB: the HHKB Professional Classic setting modes (docs/policy.md,
// reference in docs/hardware.md §7). Each mode is a base layer plus its own Fn
// layer. The mode is QMK's persistent default layer, so it survives replugging
// and an EEPROM reset returns to layer 0, Mac mode.

#pragma once

#include QMK_KEYBOARD_H

enum falcon_kalam_layers {
    _MAC,    // Mac mode (our default; the HHKB factory default is HHK mode)
    _MAC_FN,
    _WIN,    // Win mode
    _WIN_FN,
    _HHK,    // HHK mode
    _HHK_FN,
};

#define FALCON_KALAM_BASE_LAYERS ((layer_state_t)((1u << _MAC) | (1u << _WIN) | (1u << _HHK)))
#define FALCON_KALAM_FN_LAYERS ((layer_state_t)((1u << _MAC_FN) | (1u << _WIN_FN) | (1u << _HHK_FN)))

// process_record_user: Fn+Ctrl+M / Fn+Ctrl+W / Fn+Ctrl+H switch to Mac / Win /
// HHK mode. Returns false when the key press was consumed.
bool falcon_kalam_hhkb_mode_process(keyrecord_t *record);
