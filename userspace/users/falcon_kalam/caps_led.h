// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-CAPSLED: the Caps Lock LED follows the host's Caps Lock state, but
// lights only once that state has lasted CAPS_LED_ON_DELAY_MS; it goes off at
// once. macOS asserts and reverts Caps Lock within about 115 ms on some short
// presses when Caps Lock also switches input sources, which flashed the LED.

#pragma once

#include QMK_KEYBOARD_H

bool falcon_kalam_caps_led_update(led_t led_state); // led_update_user; always false: the pin is driven here
void falcon_kalam_caps_led_task(void);             // housekeeping_task_user
