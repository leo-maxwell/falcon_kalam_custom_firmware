// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RGB: the 4-LED RGB module is a status indicator (docs/policy.md).

#pragma once

#include <stdint.h>

// Indicator channels as "first LED, LED count" pairs. Which physical LEDs a
// channel uses shows on the device itself: Shift+Shift+D lights channel A
// yellow. Adjust here if another split reads better.
#define FALCON_KALAM_RGB_CH_A 0, 2 // safety / debug states
#define FALCON_KALAM_RGB_CH_B 2, 2 // connection / layer states
#define FALCON_KALAM_RGB_ALL 0, RGBLIGHT_LED_COUNT

#ifdef FALCON_KALAM_FLASHREAD_ENABLE
// Dev only (FLASHREAD): brief all-LED blink when a request is served.
void falcon_kalam_rgb_activity(void);
#endif
