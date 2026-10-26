// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RGB: the RGB module shows state, not decoration (docs/policy.md).
//
// Normal state: every LED white at the brightness cap (half of full scale).
// Indicator states are QMK rgblight layers drawn on top: a higher layer index
// wins where layers overlap.
// Safety/debug states use channel A, connection/layer states channel B, so one
// of each can show at the same time; one-off events use all LEDs. The base is
// enforced every main-loop pass, so EEPROM contents or VIA lighting commands
// cannot change it. Decorative effects exist in L1 and are disabled for every
// variant by users/falcon_kalam/config.h.

#include QMK_KEYBOARD_H
#include "usb_device_state.h"
#include "ws2812.h"
#include "rgb_indicators.h"
#include "hhkb_modes.h"

#if defined(RGBLIGHT_EFFECT_ALTERNATING) || defined(RGBLIGHT_EFFECT_BREATHING) || defined(RGBLIGHT_EFFECT_CHRISTMAS) ||   \
    defined(RGBLIGHT_EFFECT_KNIGHT) || defined(RGBLIGHT_EFFECT_RAINBOW_MOOD) || defined(RGBLIGHT_EFFECT_RAINBOW_SWIRL) || \
    defined(RGBLIGHT_EFFECT_RGB_TEST) || defined(RGBLIGHT_EFFECT_SNAKE) || defined(RGBLIGHT_EFFECT_STATIC_GRADIENT) ||     \
    defined(RGBLIGHT_EFFECT_TWINKLE)
#    error "POLICY-RGB: decorative effects must be disabled in the configuration layer (users/falcon_kalam/config.h)"
#endif

// Layer indices = priority (low to high). Keep in sync with docs/policy.md §2.1.
enum falcon_kalam_rgb_layer {
    FALCON_KALAM_RGB_LAYER_LOCKED, // B green:   a layer other than the mode base / held Fn is on
    FALCON_KALAM_RGB_USB,          // B blue:    powered, but not configured by a USB host
    FALCON_KALAM_RGB_DEBUG,        // A yellow:  debug output on
    FALCON_KALAM_RGB_KEYLOG,       // A magenta: keystroke printout on (privacy warning)
#ifdef FALCON_KALAM_FLASHREAD_ENABLE
    FALCON_KALAM_RGB_ACTIVITY, // all cyan blink: FLASHREAD request served (dev)
#endif
    FALCON_KALAM_RGB_MODE_HHK, // all purple: HHK mode (mode-key flash; settings display, first LED)
    FALCON_KALAM_RGB_MODE_MAC, // all pink:   Mac mode
    FALCON_KALAM_RGB_MODE_WIN, // all gold:   Win mode
    FALCON_KALAM_RGB_SHOW_BG,  // switch LEDs dark: settings display background
    FALCON_KALAM_RGB_SHOW_SW3, // switch LED green: SW3 on
    FALCON_KALAM_RGB_SHOW_SW4, // switch LED green: SW4 on
    FALCON_KALAM_RGB_SHOW_SW5, // switch LED green: SW5 on
    FALCON_KALAM_RGB_DIP,      // all dark, 1 blink = a virtual DIP switch turned off, 2 = on
    FALCON_KALAM_RGB_RESET, // all orange, 3 blinks at boot: settings were reset
    FALCON_KALAM_RGB_BOOT,  // all red: jumping to the bootloader (WS2812 keeps it in DFU)
};

static const rgblight_segment_t PROGMEM seg_locked[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_B, HSV_GREEN});
static const rgblight_segment_t PROGMEM seg_usb[]    = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_B, HSV_BLUE});
static const rgblight_segment_t PROGMEM seg_debug[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_A, HSV_YELLOW});
static const rgblight_segment_t PROGMEM seg_keylog[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_A, HSV_MAGENTA});
#ifdef FALCON_KALAM_FLASHREAD_ENABLE
static const rgblight_segment_t PROGMEM seg_activity[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_CYAN});
#endif
static const rgblight_segment_t PROGMEM seg_mode_hhk[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_PURPLE});
static const rgblight_segment_t PROGMEM seg_mode_mac[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_PINK});
static const rgblight_segment_t PROGMEM seg_mode_win[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_GOLD});
static const rgblight_segment_t PROGMEM seg_show_bg[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LEDS_SW, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_show_sw3[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LED_SW3, HSV_GREEN});
static const rgblight_segment_t PROGMEM seg_show_sw4[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LED_SW4, HSV_GREEN});
static const rgblight_segment_t PROGMEM seg_show_sw5[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LED_SW5, HSV_GREEN});
static const rgblight_segment_t PROGMEM seg_dip[]      = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_reset[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_ORANGE});
static const rgblight_segment_t PROGMEM seg_boot[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_RED});

// Global and `used` so scripts/verify_firmware.py can find it under LTO.
__attribute__((used)) const rgblight_segment_t *const PROGMEM falcon_kalam_rgb_layers[] = RGBLIGHT_LAYERS_LIST(seg_locked, seg_usb, seg_debug, seg_keylog,
#ifdef FALCON_KALAM_FLASHREAD_ENABLE
                                                                                                          seg_activity,
#endif
                                                                                                          seg_mode_hhk, seg_mode_mac, seg_mode_win, seg_show_bg, seg_show_sw3, seg_show_sw4, seg_show_sw5,
                                                                                                          seg_dip, seg_reset, seg_boot);

_Static_assert(FALCON_KALAM_RGB_BOOT < RGBLIGHT_MAX_LAYERS, "raise RGBLIGHT_MAX_LAYERS");
_Static_assert(RGBLIGHT_LED_COUNT == 4, "the channel segments assume the 4-LED module");

static bool settings_were_reset;

// POLICY-HHKB settings display: the settings now, then (if different) the
// settings after the next plug-in, separated by a dark gap.
enum { SHOW_IDLE, SHOW_NOW, SHOW_GAP, SHOW_NEXT };
#define SHOW_MS 1500
#define SHOW_GAP_MS 300
static uint8_t  show_phase;
static uint16_t show_timer;
static uint8_t  show_now, show_next; // packed settings, see rgb_indicators.h

// EEPROM is being (re)initialised; the LEDs do not exist yet, so only note it
// and blink once rgblight is running.
void falcon_kalam_rgb_note_settings_reset(void) {
    settings_were_reset = true;
}

// The base is always static white at the brightness cap. Only touches rgblight
// when something differs, because each call redraws the LEDs.
static void enforce_white_base(void) {
    if (rgblight_get_mode() != RGBLIGHT_MODE_STATIC_LIGHT) {
        rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    }
    if (rgblight_get_hue() != 0 || rgblight_get_sat() != 0 || rgblight_get_val() != RGBLIGHT_LIMIT_VAL) {
        rgblight_sethsv_noeeprom(0, 0, RGBLIGHT_LIMIT_VAL);
    }
}

void falcon_kalam_rgb_init(void) {
    rgblight_layers = falcon_kalam_rgb_layers;
    enforce_white_base();
    if (settings_were_reset) {
        rgblight_blink_layer_repeat(FALCON_KALAM_RGB_RESET, 250, 3);
    }
}

// Draws one settings value (on = true): the mode colour on all LEDs, the
// switch LEDs dark below their green SW3-SW5 layers. on = false clears them all.
static void show_draw(uint8_t settings, bool on) {
    for (uint8_t mode = 0; mode < 3; mode++) {
        rgblight_set_layer_state(FALCON_KALAM_RGB_MODE_HHK + mode, on && (settings & 3) == mode);
    }
    rgblight_set_layer_state(FALCON_KALAM_RGB_SHOW_BG, on);
    for (uint8_t sw = 0; sw < 3; sw++) {
        rgblight_set_layer_state(FALCON_KALAM_RGB_SHOW_SW3 + sw, on && ((settings >> (2 + sw)) & 1));
    }
}

static void show_task(void) {
    if (show_phase == SHOW_IDLE || timer_elapsed(show_timer) < (show_phase == SHOW_GAP ? SHOW_GAP_MS : SHOW_MS)) {
        return;
    }
    show_timer = timer_read();
    if (show_phase == SHOW_NOW) {
        show_draw(show_now, false);
        show_phase = show_next != show_now ? SHOW_GAP : SHOW_IDLE;
    } else if (show_phase == SHOW_GAP) {
        show_draw(show_next, true);
        show_phase = SHOW_NEXT;
    } else {
        show_draw(show_next, false);
        show_phase = SHOW_IDLE;
    }
}

// State-driven indicators, polled from the main loop (the USB state callback
// may run in interrupt context, and the debug flags have no callback at all).
// rgblight_set_layer_state() schedules a redraw, so it is only called on change.
void falcon_kalam_rgb_task(void) {
    static uint8_t shown; // bit n = indicator layer n currently on (n < 8)

    usb_configure_state_t usb    = usb_device_state_get_configure_state();
    // POLICY-HHKB: a mode base (default layer; an empty state means layer 0)
    // and a held Fn layer are normal; any other layer switched on, or another
    // default layer, is not.
    layer_state_t base   = default_layer_state ? default_layer_state : (layer_state_t)1;
    bool          locked = (layer_state & ~FALCON_KALAM_FN_LAYERS) || (base & ~FALCON_KALAM_BASE_LAYERS);

    uint8_t want = 0;
    if (locked) want |= 1u << FALCON_KALAM_RGB_LAYER_LOCKED;
    if (usb != USB_DEVICE_STATE_CONFIGURED && usb != USB_DEVICE_STATE_SUSPEND) want |= 1u << FALCON_KALAM_RGB_USB;
    if (debug_config.enable) want |= 1u << FALCON_KALAM_RGB_DEBUG;
    if (debug_config.enable && debug_config.keyboard) want |= 1u << FALCON_KALAM_RGB_KEYLOG;

    uint8_t changed = want ^ shown;
    for (uint8_t layer = FALCON_KALAM_RGB_LAYER_LOCKED; layer <= FALCON_KALAM_RGB_KEYLOG; layer++) {
        if (changed & (1u << layer)) {
            rgblight_set_layer_state(layer, want & (1u << layer));
        }
    }
    shown = want;

    show_task();
    enforce_white_base();
}

// Runs right before QMK resets the MCU. On a bootloader jump, paint all LEDs
// red and push the frame out now: the WS2812s keep showing it while the
// bootloader (which does not drive A1) is active.
void falcon_kalam_rgb_shutdown(bool jump_to_bootloader) {
    if (jump_to_bootloader) {
        rgblight_set_layer_state(FALCON_KALAM_RGB_BOOT, true);
        rgblight_set();
    }
}

// Bootmagic runs before rgblight is initialised, so drive the WS2812 driver
// directly: all LEDs red, latched while the bootloader runs (R15). Bypassing
// rgblight also bypasses its brightness cap, so apply it here.
void falcon_kalam_rgb_paint_bootloader(void) {
    ws2812_init();
    ws2812_set_color_all(RGBLIGHT_LIMIT_VAL, 0, 0);
    ws2812_flush();
}

// POLICY-HHKB virtual DIP switch flipped: the white base blinks dark.
void falcon_kalam_rgb_dip_feedback(bool on) {
    rgblight_blink_layer_repeat(FALCON_KALAM_RGB_DIP, 150, on ? 2 : 1);
}

// POLICY-HHKB mode key: one flash in the new mode's colour. Ends a running
// settings display, which shares the mode layers.
void falcon_kalam_rgb_mode_flash(uint8_t mode) {
    show_draw(0, false);
    show_phase = SHOW_IDLE;
    rgblight_blink_layer(FALCON_KALAM_RGB_MODE_HHK + mode, 300);
}

void falcon_kalam_rgb_show_settings(uint8_t now, uint8_t next) {
    for (uint8_t mode = 0; mode < 3; mode++) {
        rgblight_unblink_layer(FALCON_KALAM_RGB_MODE_HHK + mode); // a pending flash would cut the display short
    }
    show_draw(0, false);
    show_now   = now;
    show_next  = next;
    show_phase = SHOW_NOW;
    show_timer = timer_read();
    show_draw(now, true);
}

#ifdef FALCON_KALAM_FLASHREAD_ENABLE
void falcon_kalam_rgb_activity(void) {
    rgblight_blink_layer(FALCON_KALAM_RGB_ACTIVITY, 40);
}
#endif
