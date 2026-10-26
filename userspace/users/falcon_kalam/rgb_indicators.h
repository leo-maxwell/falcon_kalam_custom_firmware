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

#include <stdbool.h>

// Module interface; QMK hooks are dispatched from falcon_kalam.c.
void falcon_kalam_rgb_init(void);                          // keyboard_post_init_user
void falcon_kalam_rgb_task(void);                          // housekeeping_task_user
void falcon_kalam_rgb_shutdown(bool jump_to_bootloader);   // shutdown_user
void falcon_kalam_rgb_note_settings_reset(void);           // eeconfig_init_user
void falcon_kalam_rgb_paint_bootloader(void);              // bootmagic_reset_eeprom
void falcon_kalam_rgb_dip_feedback(bool on);               // hhkb_modes.c: virtual DIP switch toggled

#ifdef FALCON_KALAM_FLASHREAD_ENABLE
// Dev only (FLASHREAD): brief all-LED blink when a request is served.
void falcon_kalam_rgb_activity(void);
#endif
