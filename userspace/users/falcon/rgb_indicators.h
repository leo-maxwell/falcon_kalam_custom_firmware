// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RGB: the 4-LED RGB module is a status indicator (docs/policy.md).

#pragma once

#include <stdint.h>

// Indicator channels as "first LED, LED count" pairs. The physical order of the
// LEDs on the module is not known yet (devnotes: LED-order test, dev Shift+Shift+L),
// so each channel is simply two LEDs; adjust here once the order is known.
#define FALCON_RGB_CH_A 0, 2 // safety / debug states
#define FALCON_RGB_CH_B 2, 2 // connection / layer states
#define FALCON_RGB_ALL 0, RGBLIGHT_LED_COUNT

// Dev only: brief all-LED blink when a FLASHREAD request is served.
void falcon_rgb_activity(void);
