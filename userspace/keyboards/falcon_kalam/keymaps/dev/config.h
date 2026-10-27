// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Dev variant (POLICY-VARIANTS). Shared configuration: users/falcon_kalam/config.h.
// Full QMK Command (all LShift+RShift keys) stays at its defaults here, except
// layer switching: with DYNAMIC_KEYMAP_LAYER_COUNT 6, Shift+Shift+6..9 and
// F6..F9 would select a default layer that has no keymap, and every key then
// reads KC_NO until the next plug-in (release 13; ship switches it off too).

#pragma once

#define MAGIC_KEY_SWITCH_LAYER_WITH_FKEYS false
#define MAGIC_KEY_SWITCH_LAYER_WITH_NKEYS false

// Dev variant marker: enables dev-only code in users/falcon_kalam (currently the
// Shift+Shift+F fault test for POLICY-RECOVERY).
#define FALCON_KALAM_DEV

// Lets VIA read the live switch matrix, so VIA's key tester and
// tools/falcon_kalam_probe.py matrix work. QMK warns at build time on purpose: any
// host program can then observe keypresses through VIA.
#define VIA_INSECURE

// Console traces (printed only while debug is on, Shift+Shift+D):
#define ACTION_DEBUG                // key actions, tapping and the waiting buffer
#define WEAR_LEVELING_DEBUG_OUTPUT  // EEPROM backing-store reads, writes and erases
