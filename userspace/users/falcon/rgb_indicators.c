// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RGB: the RGB module shows state, not decoration (docs/policy.md).
//
// Normal state: every LED white at full brightness. Indicator states are QMK
// rgblight layers drawn on top: a higher layer index wins where layers overlap.
// Safety/debug states use channel A, connection/layer states channel B, so one
// of each can show at the same time; one-off events use all LEDs. The base is
// enforced every main-loop pass, so EEPROM contents or VIA lighting commands
// cannot change it. Decorative effects exist in L1 and are disabled for every
// variant by users/falcon/config.h.

#include QMK_KEYBOARD_H
#include "usb_device_state.h"
#include "rgb_indicators.h"

#if defined(RGBLIGHT_EFFECT_ALTERNATING) || defined(RGBLIGHT_EFFECT_BREATHING) || defined(RGBLIGHT_EFFECT_CHRISTMAS) ||   \
    defined(RGBLIGHT_EFFECT_KNIGHT) || defined(RGBLIGHT_EFFECT_RAINBOW_MOOD) || defined(RGBLIGHT_EFFECT_RAINBOW_SWIRL) || \
    defined(RGBLIGHT_EFFECT_RGB_TEST) || defined(RGBLIGHT_EFFECT_SNAKE) || defined(RGBLIGHT_EFFECT_STATIC_GRADIENT) ||     \
    defined(RGBLIGHT_EFFECT_TWINKLE)
#    error "POLICY-RGB: decorative effects must be disabled in the configuration layer (users/falcon/config.h)"
#endif

// Layer indices = priority (low to high). Keep in sync with docs/policy.md §2.1.
enum falcon_rgb_layer {
    FALCON_RGB_LAYER_LOCKED, // B green:   a non-default layer is locked on
    FALCON_RGB_USB,          // B blue:    powered, but not configured by a USB host
    FALCON_RGB_DEBUG,        // A yellow:  debug output on
    FALCON_RGB_KEYLOG,       // A magenta: keystroke printout on (privacy warning)
#ifdef FALCON_FLASHREAD_ENABLE
    FALCON_RGB_ACTIVITY, // all cyan blink: FLASHREAD request served (dev)
#endif
    FALCON_RGB_RESET, // all orange, 3 blinks at boot: settings were reset
    FALCON_RGB_BOOT,  // all red: jumping to the bootloader (WS2812 keeps it in DFU)
};

static const rgblight_segment_t PROGMEM seg_locked[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_B, HSV_GREEN});
static const rgblight_segment_t PROGMEM seg_usb[]    = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_B, HSV_BLUE});
static const rgblight_segment_t PROGMEM seg_debug[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_A, HSV_YELLOW});
static const rgblight_segment_t PROGMEM seg_keylog[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_CH_A, HSV_MAGENTA});
#ifdef FALCON_FLASHREAD_ENABLE
static const rgblight_segment_t PROGMEM seg_activity[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_ALL, HSV_CYAN});
#endif
static const rgblight_segment_t PROGMEM seg_reset[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_ALL, HSV_ORANGE});
static const rgblight_segment_t PROGMEM seg_boot[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_RGB_ALL, HSV_RED});

// Global and `used` so scripts/verify_firmware.py can find it under LTO.
__attribute__((used)) const rgblight_segment_t *const PROGMEM falcon_rgb_layers[] = RGBLIGHT_LAYERS_LIST(seg_locked, seg_usb, seg_debug, seg_keylog,
#ifdef FALCON_FLASHREAD_ENABLE
                                                                                                          seg_activity,
#endif
                                                                                                          seg_reset, seg_boot);

_Static_assert(FALCON_RGB_BOOT < RGBLIGHT_MAX_LAYERS, "raise RGBLIGHT_MAX_LAYERS");
_Static_assert(RGBLIGHT_LED_COUNT == 4, "the channel segments assume the 4-LED module");

static bool settings_were_reset;

// Called by QMK while (re)initialising EEPROM, before the LEDs are set up:
// only note it here and blink once rgblight is running.
void eeconfig_init_user(void) {
#if (EECONFIG_USER_DATA_SIZE) == 0
    eeconfig_update_user(0); // what QMK's default implementation does
#endif
    settings_were_reset = true;
}

// The base is always static white at full brightness. Only touches rgblight
// when something differs, because each call redraws the LEDs.
static void enforce_white_base(void) {
    if (rgblight_get_mode() != RGBLIGHT_MODE_STATIC_LIGHT) {
        rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    }
    if (rgblight_get_hue() != 0 || rgblight_get_sat() != 0 || rgblight_get_val() != RGBLIGHT_LIMIT_VAL) {
        rgblight_sethsv_noeeprom(0, 0, RGBLIGHT_LIMIT_VAL);
    }
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

    usb_configure_state_t usb    = usb_device_state_get_configure_state();
    layer_state_t         locked = (layer_state & ~((layer_state_t)1 << 1)) | default_layer_state; // ignore momentary Fn (layer 1)

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

#ifdef FALCON_FLASHREAD_ENABLE
void falcon_rgb_activity(void) {
    rgblight_blink_layer(FALCON_RGB_ACTIVITY, 40);
}
#endif
