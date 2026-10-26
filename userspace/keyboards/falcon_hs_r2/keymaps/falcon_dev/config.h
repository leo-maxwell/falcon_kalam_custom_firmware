// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Dev variant (POLICY-VARIANTS). Shared configuration: users/falcon/config.h.
// Full QMK Command (all LShift+RShift keys) stays at its defaults here.

#pragma once

// POLICY-RGB dev extensions: decorative effects stay selectable underneath the
// indicators (for LED tests such as RGB test mode), white is not enforced,
// Shift+Shift+L runs the LED-order identify, FLASHREAD blinks cyan.
#define FALCON_RGB_DEV

// Lets VIA read the live switch matrix, so VIA's key tester and
// tools/falcon_probe.py matrix work. QMK warns at build time on purpose: any
// host program can then observe keypresses through VIA.
#define VIA_INSECURE

// Console traces (printed only while debug is on, Shift+Shift+D):
#define ACTION_DEBUG                // key actions, tapping and the waiting buffer
#define WEAR_LEVELING_DEBUG_OUTPUT  // EEPROM backing-store reads, writes and erases
