// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RGB: the RGB module shows state, not decoration (docs/policy.md).
//
// Normal state: every LED white at full brightness. Indicator states are QMK
// rgblight layers drawn on top: a higher layer index wins where layers overlap.
// Safety/debug states use channel A, connection/layer states channel B, so one
// of each can show at the same time; one-off events use all LEDs.
//
// Ship: effects are compiled out and white is enforced every loop, so EEPROM
// contents or VIA lighting commands cannot change the base colour. Dev
// (FALCON_RGB_DEV): the user may pick any effect underneath the indicators,
// and Shift+Shift+L lights one LED at a time for the LED-order test.

#include QMK_KEYBOARD_H
#include "usb_device_state.h"
#include "rgb_indicators.h"
#ifdef FALCON_RGB_DEV
#    include "print.h"
#endif

// Layer indices = priority (low to high). Keep in sync with docs/policy.md.
enum falcon_rgb_layer {
    FALCON_RGB_LAYER_LOCKED, // B green:   a non-default layer is locked on
    FALCON_RGB_USB,          // B blue:    USB not configured by a host
    FALCON_RGB_DEBUG,        // A yellow:  debug output on
    FALCON_RGB_KEYLOG,       // A magenta: keystroke printout on (privacy warning)
#ifdef FALCON_RGB_DEV
    FALCON_RGB_ACTIVITY, // all cyan blink: FLASHREAD request served
    FALCON_RGB_ID0,      // LED-order test: only LED n lit (n = 0..3)
    FALCON_RGB_ID1,
    FALCON_RGB_ID2,
    FALCON_RGB_ID3,
#endif
    FALCON_RGB_RESET, // all orange, 3 blinks at boot: settings were reset
    FALCON_RGB_BOOT,  // all red: jumping to the bootloader (WS2812 keeps it in DFU)
};

static const rgblight_segment_t PROGMEM seg_locked[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_B, HSV_GREEN});
static const rgblight_segment_t PROGMEM seg_usb[]    = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_B, HSV_BLUE});
static const rgblight_segment_t PROGMEM seg_debug[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_A, HSV_YELLOW});
static const rgblight_segment_t PROGMEM seg_keylog[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_A, HSV_MAGENTA});
#ifdef FALCON_RGB_DEV
static const rgblight_segment_t PROGMEM seg_activity[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_ALL, HSV_CYAN});
static const rgblight_segment_t PROGMEM seg_id0[]      = RGBLIGHT_LAYER_SEGMENTS({0, 1, HSV_WHITE}, {1, 3, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_id1[]      = RGBLIGHT_LAYER_SEGMENTS({0, 1, HSV_BLACK}, {1, 1, HSV_WHITE}, {2, 2, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_id2[]      = RGBLIGHT_LAYER_SEGMENTS({0, 2, HSV_BLACK}, {2, 1, HSV_WHITE}, {3, 1, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_id3[]      = RGBLIGHT_LAYER_SEGMENTS({0, 3, HSV_BLACK}, {3, 1, HSV_WHITE});
#endif
static const rgblight_segment_t PROGMEM seg_reset[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_ALL, HSV_ORANGE});
static const rgblight_segment_t PROGMEM seg_boot[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_ALL, HSV_RED});

// Global and `used` so scripts/verify_firmware.py can find it under LTO.
__attribute__((used)) const rgblight_segment_t *const PROGMEM falcon_rgb_layers[] = RGBLIGHT_LAYERS_LIST(seg_locked, seg_usb, seg_debug, seg_keylog,
#ifdef FALCON_RGB_DEV
                                                                                                          seg_activity, seg_id0, seg_id1, seg_id2, seg_id3,
#endif
                                                                                                          seg_reset, seg_boot);

_Static_assert(FALCON_RGB_BOOT < RGBLIGHT_MAX_LAYERS, "raise RGBLIGHT_MAX_LAYERS");
_Static_assert(RGBLIGHT_LED_COUNT == 4, "channel and identify segments assume the 4-LED module");

static bool settings_were_reset;

// Called by QMK while (re)initialising EEPROM, before the LEDs are set up:
// only note it here and blink once rgblight is running.
void eeconfig_init_user(void) {
#if (EECONFIG_USER_DATA_SIZE) == 0
    eeconfig_update_user(0); // what QMK's default implementation does
#endif
    settings_were_reset = true;
}

// Ship: the base is always static white at full brightness. Only touches
// rgblight when something differs, because each call redraws the LEDs.
static void enforce_white_base(void) {
#ifndef FALCON_RGB_DEV
    if (rgblight_get_mode() != RGBLIGHT_MODE_STATIC_LIGHT) {
        rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    }
    if (rgblight_get_hue() != 0 || rgblight_get_sat() != 0 || rgblight_get_val() != RGBLIGHT_LIMIT_VAL) {
        rgblight_sethsv_noeeprom(0, 0, RGBLIGHT_LIMIT_VAL);
    }
#endif
}

void keyboard_post_init_user(void) {
    rgblight_layers = falcon_rgb_layers;
    enforce_white_base();
    if (settings_were_reset) {
        rgblight_blink_layer_repeat(FALCON_RGB_RESET, 250, 3);
    }
}

// State-driven indicators, polled from the main loop (the USB state callback
// may run in interrupt context, and the debug flags have no callback at all).
// rgblight_set_layer_state() schedules a redraw, so it is only called on change.
void housekeeping_task_user(void) {
    static uint8_t shown; // bit n = indicator layer n currently on (n < 8)

    usb_configure_state_t usb = usb_device_state_get_configure_state();
    layer_state_t locked = (layer_state & ~((layer_state_t)1 << 1)) | default_layer_state; // ignore momentary Fn (layer 1)

    uint8_t want = 0;
    if (get_highest_layer(locked) >= 2 || get_highest_layer(default_layer_state) != 0) want |= 1u << FALCON_RGB_LAYER_LOCKED;
    if (usb != USB_DEVICE_STATE_CONFIGURED && usb != USB_DEVICE_STATE_SUSPEND) want |= 1u << FALCON_RGB_USB;
    if (debug_config.enable) want |= 1u << FALCON_RGB_DEBUG;
    if (debug_config.enable && debug_config.keyboard) want |= 1u << FALCON_RGB_KEYLOG;

    uint8_t changed = want ^ shown;
    for (uint8_t layer = FALCON_RGB_LAYER_LOCKED; layer <= FALCON_RGB_KEYLOG; layer++) {
        if (changed & (1u << layer)) {
            rgblight_set_layer_state(layer, want & (1u << layer));
        }
    }
    shown = want;

    enforce_white_base();
}

// Runs right before QMK resets the MCU. On a bootloader jump, paint all LEDs
// red and push the frame out now: the WS2812s keep showing it while the
// bootloader (which does not drive A1) is active.
bool shutdown_user(bool jump_to_bootloader) {
    if (jump_to_bootloader) {
        rgblight_set_layer_state(FALCON_RGB_BOOT, true);
        rgblight_set();
    }
    return true;
}

#ifdef FALCON_RGB_DEV
void falcon_rgb_activity(void) {
    rgblight_blink_layer(FALCON_RGB_ACTIVITY, 40);
}

// Dev Command extra: Shift+Shift+L steps "LED 0 only, 1, 2, 3, off" so the
// physical position of each index can be noted (LED-order test).
bool command_extra(uint8_t code) {
    static uint8_t step; // 0 = off, n = LED n-1 lit
    if (code != KC_L) {
        return false;
    }
    if (step) {
        rgblight_set_layer_state(FALCON_RGB_ID0 + step - 1, false);
    }
    step = (step + 1) % (RGBLIGHT_LED_COUNT + 1);
    if (step) {
        rgblight_set_layer_state(FALCON_RGB_ID0 + step - 1, true);
        uprintf("RGB identify: LED %u lit\n", step - 1);
    } else {
        uprintf("RGB identify: off\n");
    }
    return true;
}
#else
void falcon_rgb_activity(void) {}
#endif
