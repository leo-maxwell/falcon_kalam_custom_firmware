// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The eeconfig user word: falcon_kalam's own saved settings (docs/policy.md §5).

#pragma once

#include <stdint.h>

// Layout of the saved settings. Bump it whenever the keymap layers or anything
// else stored in EEPROM changes meaning: a keyboard with an older value then
// resets its settings once, so the new defaults take effect even if DFU was
// entered without the Esc reset. VIA's own check cannot do this, because it
// keys on the build date, which is pinned (reproducible builds).
//   1 = six HHKB mode layers, Mac first (stored as the full word 0x464B0001)
//   2 = layers in the HHKB Classic's order (HHK, Mac, Win) + virtual DIP switches
#define FALCON_KALAM_EEPROM_SCHEMA 2

typedef union {
    uint32_t raw;
    struct {
        uint16_t schema; // FALCON_KALAM_EEPROM_SCHEMA
        uint8_t  dip;    // virtual DIP switches: bit n = SW(n+1) on (hhkb_modes.c)
        uint8_t  reserved;
    };
} falcon_kalam_user_config_t;

_Static_assert(sizeof(falcon_kalam_user_config_t) == sizeof(uint32_t), "fits the eeconfig user word");
