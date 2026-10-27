// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RGB: the 4-LED RGB module is a status indicator (docs/policy.md).

#pragma once

#include <stdbool.h>
#include <stdint.h>

// Indicator channels as "first LED, LED count" pairs. LED indices run left to
// right (docs/hardware.md §6): channel A is the left pair, B the right pair.
#define FALCON_KALAM_RGB_CH_A 0, 2 // safety / debug states
#define FALCON_KALAM_RGB_CH_B 2, 2 // connection / layer states
#define FALCON_KALAM_RGB_ALL 0, RGBLIGHT_LED_COUNT

// POLICY-HHKB settings display (Fn+Ctrl+Shift+0, mode keys): LED 1 (leftmost)
// shows the mode colour, the other three SW3, SW4, SW5 left to right (blue =
// on, off = off).
#define FALCON_KALAM_RGB_LEDS_SW 1, 3 // the three switch LEDs
#define FALCON_KALAM_RGB_LED_SW3 1, 1
#define FALCON_KALAM_RGB_LED_SW4 2, 1
#define FALCON_KALAM_RGB_LED_SW5 3, 1

// Module interface; QMK hooks are dispatched from falcon_kalam.c.
void falcon_kalam_rgb_pre_init(void);                    // keyboard_pre_init_user
void falcon_kalam_rgb_init(void);                        // keyboard_post_init_user
void falcon_kalam_rgb_task(void);                        // housekeeping_task_user
void falcon_kalam_rgb_shutdown(bool jump_to_bootloader); // shutdown_user
void falcon_kalam_rgb_note_settings_reset(void);         // eeconfig_init_user
void falcon_kalam_rgb_paint_bootloader(void);            // bootmagic_reset_eeprom
void falcon_kalam_rgb_dip_feedback(bool on);             // hhkb_modes.c: virtual DIP switch flipped

// hhkb_modes.c. A settings value packs the mode (bits 0-1: 0 HHK, 1 Mac,
// 2 Win) and SW3-SW5 (bits 2-4).
void falcon_kalam_rgb_show_settings(uint8_t now, uint8_t next); // settings now, then after a replug if different

#ifdef FALCON_KALAM_FLASHREAD_ENABLE
// Dev only (FLASHREAD): brief all-LED blink when a request is served.
void falcon_kalam_rgb_activity(void);
#endif
