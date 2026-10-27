// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-CAPSLED (docs/policy.md §2.3): a 200 ms on-delay for the Caps Lock
// LED. QMK calls led_update_user() whenever the host's LED report changes;
// returning false keeps QMK from writing the pin, and led_update_ports() is
// called from here instead: at once for off, after the delay for on.

#include "caps_led.h"

#define CAPS_LED_ON_DELAY_MS 200

static bool     caps_host;  // the host's Caps Lock state
static bool     caps_shown; // what the LED shows
static uint16_t caps_since; // when caps_host last changed

bool falcon_kalam_caps_led_update(led_t led_state) {
    if (led_state.caps_lock != caps_host) {
        caps_host  = led_state.caps_lock;
        caps_since = timer_read();
    }
    if (!caps_host && caps_shown) {
        caps_shown = false;
        led_update_ports(led_state);
    }
    return false;
}

// noinline: keeps the symbol under LTO so scripts/verify_firmware.py can find it.
__attribute__((noinline)) void falcon_kalam_caps_led_task(void) {
    if (caps_host && !caps_shown && timer_elapsed(caps_since) >= CAPS_LED_ON_DELAY_MS) {
        caps_shown = true;
        led_update_ports(host_keyboard_led_state());
    }
}
