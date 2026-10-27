// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RGB: the RGB module shows state, not decoration (docs/policy.md).
//
// Normal state: every LED green at the brightness cap (69 of full scale 255).
// Indicator states are QMK rgblight layers drawn on top: a higher layer index
// wins where layers overlap.
// Safety/debug states use channel A, connection/layer states channel B, so one
// of each can show at the same time; one-off events use all LEDs. The base is
// enforced every main-loop pass (on, colour and mode), so EEPROM contents or
// VIA lighting commands cannot change it. Decorative effects exist in L1 and are disabled for every
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
// Only pure red, green and blue (one diode each: mixed colours are hard to
// tell on this module), at two levels: BRIGHT (the cap) and DIM (an eighth).
enum falcon_kalam_rgb_layer {
    FALCON_KALAM_RGB_LAYER_LOCKED, // B blue:      a layer other than the mode base / held Fn is on
    FALCON_KALAM_RGB_USB,          // B dim green: powered, but not configured by a USB host
    FALCON_KALAM_RGB_DEBUG,        // A blue:      debug output on
    FALCON_KALAM_RGB_KEYLOG,       // A red:       keystroke printout on (privacy warning)
#ifdef FALCON_KALAM_FLASHREAD_ENABLE
    FALCON_KALAM_RGB_ACTIVITY, // all blue blink: FLASHREAD request served (dev)
#endif
    FALCON_KALAM_RGB_MODE_HHK, // LED 1 red:   settings display, HHK mode
    FALCON_KALAM_RGB_MODE_MAC, // LED 1 blue:  Mac mode
    FALCON_KALAM_RGB_MODE_WIN, // LED 1 green: Win mode
    FALCON_KALAM_RGB_SHOW_GAP, // LED 1 off:   settings display gap (with SHOW_BG: all off)
    FALCON_KALAM_RGB_SHOW_BG,  // LEDs 2-4 off: settings display background
    FALCON_KALAM_RGB_SHOW_SW3, // switch LED blue: SW3 on
    FALCON_KALAM_RGB_SHOW_SW4, // switch LED blue: SW4 on
    FALCON_KALAM_RGB_SHOW_SW5, // switch LED blue: SW5 on
    FALCON_KALAM_RGB_DARK,     // all off: DIP-flip blinks (1 = turned off, 2 = on); background of the reset blinks
    FALCON_KALAM_RGB_RESET,    // all red, 3 blinks once configured: settings were reset
    FALCON_KALAM_RGB_BOOT,     // all red: jumping to the bootloader (WS2812 keeps it in DFU)
};

// Hues of the pure colours: QMK's HSV conversion gives exactly one diode for
// these (e.g. 0,255,v -> v,0,0). Values above the cap are clamped by rgblight.
#define FK_RED 0, 255
#define FK_GREEN 85, 255
#define FK_BLUE 170, 255
#define FK_BRIGHT RGBLIGHT_LIMIT_VAL
#define FK_DIM (RGBLIGHT_LIMIT_VAL / 8)
#define FK_LED_1 0, 1

static const rgblight_segment_t PROGMEM seg_locked[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_B, FK_BLUE, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_usb[]    = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_B, FK_GREEN, FK_DIM});
static const rgblight_segment_t PROGMEM seg_debug[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_A, FK_BLUE, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_keylog[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_CH_A, FK_RED, FK_BRIGHT});
#ifdef FALCON_KALAM_FLASHREAD_ENABLE
static const rgblight_segment_t PROGMEM seg_activity[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, FK_BLUE, FK_BRIGHT});
#endif
static const rgblight_segment_t PROGMEM seg_mode_hhk[] = RGBLIGHT_LAYER_SEGMENTS({FK_LED_1, FK_RED, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_mode_mac[] = RGBLIGHT_LAYER_SEGMENTS({FK_LED_1, FK_BLUE, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_mode_win[] = RGBLIGHT_LAYER_SEGMENTS({FK_LED_1, FK_GREEN, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_show_gap[] = RGBLIGHT_LAYER_SEGMENTS({FK_LED_1, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_show_bg[]  = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LEDS_SW, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_show_sw3[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LED_SW3, FK_BLUE, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_show_sw4[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LED_SW4, FK_BLUE, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_show_sw5[] = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_LED_SW5, FK_BLUE, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_dark[]     = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, HSV_BLACK});
static const rgblight_segment_t PROGMEM seg_reset[]    = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, FK_RED, FK_BRIGHT});
static const rgblight_segment_t PROGMEM seg_boot[]     = RGBLIGHT_LAYER_SEGMENTS({FALCON_KALAM_RGB_ALL, FK_RED, FK_BRIGHT});

// Global and `used` so scripts/verify_firmware.py can find it under LTO.
__attribute__((used)) const rgblight_segment_t *const PROGMEM falcon_kalam_rgb_layers[] = RGBLIGHT_LAYERS_LIST(seg_locked, seg_usb, seg_debug, seg_keylog,
#ifdef FALCON_KALAM_FLASHREAD_ENABLE
                                                                                                          seg_activity,
#endif
                                                                                                          seg_mode_hhk, seg_mode_mac, seg_mode_win, seg_show_gap, seg_show_bg, seg_show_sw3, seg_show_sw4, seg_show_sw5,
                                                                                                          seg_dark, seg_reset, seg_boot);

_Static_assert(FALCON_KALAM_RGB_BOOT < RGBLIGHT_MAX_LAYERS, "raise RGBLIGHT_MAX_LAYERS");
_Static_assert(RGBLIGHT_LED_COUNT == 4, "the channel segments assume the 4-LED module");
_Static_assert(FALCON_KALAM_RGB_KEYLOG < 8, "the polled indicator layers must fit the uint8_t masks in falcon_kalam_rgb_task");

static bool settings_were_reset;

// Boot: rgblight is stored off (RGBLIGHT_DEFAULT_ON false), so rgblight_init
// draws nothing, and the dark layer is on from post-init. The base comes on
// once the host has configured the keyboard: during enumeration the host
// suspends and resumes the bus, and the LEDs would flicker with it. If no host
// configures it within BOOT_WAIT_MS (charger, power bank), the base and the
// not-configured indicator come on anyway.
#define BOOT_WAIT_MS 2000
static bool     booting = true;
static uint16_t boot_timer;

// Reset blinks: 3 x (250 ms red + 250 ms off) over the dark layer, in place
// of the boot's first base frame. rgblight_suspend() cancels running blinks,
// which is another reason to wait for the configured state.
#define RESET_BLINK_MS 250
#define RESET_BLINKS 3
static bool     reset_dark;
static uint16_t reset_dark_timer;

// Ends the dark start: switch the base on for this session only, so the
// stored setting stays off for the next boot.
static void boot_light_up(void) {
    rgblight_set_layer_state(FALCON_KALAM_RGB_DARK, false);
    rgblight_enable_noeeprom();
}

// POLICY-HHKB settings display: the settings now, then (if different) the
// settings after the next plug-in, separated by a dark gap.
enum { SHOW_IDLE, SHOW_NOW, SHOW_GAP, SHOW_NEXT };
#define SHOW_MS 1500
#define SHOW_GAP_MS 300
static uint8_t  show_phase;
static uint16_t show_timer;
static uint8_t  show_now, show_next; // packed settings, see rgb_indicators.h

// EEPROM is being (re)initialised; the LEDs do not exist yet, so only note it
// and blink later (falcon_kalam_rgb_task).
void falcon_kalam_rgb_note_settings_reset(void) {
    settings_were_reset = true;
}

// The base is always on, static green at the brightness cap, once the boot is
// over: a raw-HID "effect 0" (rgblight off) is undone here, as are colour and
// mode changes. Only touches rgblight when something differs, because each
// call redraws the LEDs. Not while the boot is dark (dark start, reset blinks),
// and never while the host sleeps: the main loop does not run then.
static void enforce_green_base(void) {
    if (booting || reset_dark) {
        return;
    }
    if (!rgblight_is_enabled()) {
        rgblight_enable_noeeprom();
    }
    if (rgblight_get_mode() != RGBLIGHT_MODE_STATIC_LIGHT) {
        rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    }
    if (rgblight_get_hue() != 85 || rgblight_get_sat() != 255 || rgblight_get_val() != RGBLIGHT_LIMIT_VAL) {
        rgblight_sethsv_noeeprom(FK_GREEN, RGBLIGHT_LIMIT_VAL);
    }
}

void falcon_kalam_rgb_init(void) {
    rgblight_layers = falcon_kalam_rgb_layers;
    // Settings from older images store rgblight on: store it off once.
    rgblight_config_t cfg;
    eeconfig_read_rgblight(&cfg);
    if (cfg.enable) {
        cfg.enable = false;
        eeconfig_update_rgblight(&cfg);
        rgblight_disable_noeeprom();
    }
    rgblight_set_layer_state(FALCON_KALAM_RGB_DARK, true);
    boot_timer = timer_read();
    // The first LED garbles the first frame after the data pin is set up
    // (docs/hardware.md §6), and the next frame comes only when the host is
    // ready. Send the dark frame twice now.
    rgblight_set();
    rgblight_set();
}

// Draws one settings value (on = true): the mode colour on LED 1, LEDs 2-4
// off below their blue SW3-SW5 layers. on = false clears them all.
static void show_draw(uint8_t settings, bool on) {
    for (uint8_t mode = 0; mode < 3; mode++) {
        rgblight_set_layer_state(FALCON_KALAM_RGB_MODE_HHK + mode, on && (settings & 3) == mode);
    }
    rgblight_set_layer_state(FALCON_KALAM_RGB_SHOW_BG, on);
    for (uint8_t sw = 0; sw < 3; sw++) {
        rgblight_set_layer_state(FALCON_KALAM_RGB_SHOW_SW3 + sw, on && ((settings >> (2 + sw)) & 1));
    }
}

// The gap between the two phases: every LED off, so the normal state does not
// show through and read as part of the display.
static void show_gap(bool on) {
    rgblight_set_layer_state(FALCON_KALAM_RGB_SHOW_GAP, on);
    rgblight_set_layer_state(FALCON_KALAM_RGB_SHOW_BG, on);
}

static void show_task(void) {
    if (show_phase == SHOW_IDLE || timer_elapsed(show_timer) < (show_phase == SHOW_GAP ? SHOW_GAP_MS : SHOW_MS)) {
        return;
    }
    show_timer = timer_read();
    if (show_phase == SHOW_NOW) {
        show_draw(show_now, false);
        show_phase = SHOW_IDLE;
        if (show_next != show_now) {
            show_gap(true);
            show_phase = SHOW_GAP;
        }
    } else if (show_phase == SHOW_GAP) {
        show_gap(false);
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
static uint8_t shown; // bit n = polled indicator layer n currently on (n < 8)

#ifdef FALCON_KALAM_DEV
// Dev light test (Shift+Shift+L): every indicator state in turn, without a
// replug, each named on the console. Layer states are switched on directly;
// the one-off events run their normal code paths. The polled indicators are
// paused meanwhile and re-evaluated at the end.
#    include "print.h"
enum {
    TEST_IDLE,
    TEST_LOCKED,
    TEST_USB,
    TEST_DEBUG,
    TEST_KEYLOG,
    TEST_MODE_HHK,
    TEST_MODE_MAC,
    TEST_MODE_WIN,
    TEST_SHOW,
    TEST_DIP_OFF,
    TEST_DIP_ON,
    TEST_RESET,
    TEST_BOOT,
    TEST_END,
};
static const struct {
    const char *name;
    uint8_t     layer; // switched on for the step, 0xFF = none
    uint16_t    ms;
} PROGMEM test_steps[] = {
    [TEST_LOCKED] = {"channel B blue: layer locked", FALCON_KALAM_RGB_LAYER_LOCKED, 1500},
    [TEST_USB] = {"channel B dim green: USB not configured", FALCON_KALAM_RGB_USB, 1500},
    [TEST_DEBUG] = {"channel A blue: debug on", FALCON_KALAM_RGB_DEBUG, 1500},
    [TEST_KEYLOG] = {"channel A red: keystroke printout on", FALCON_KALAM_RGB_KEYLOG, 1500},
    [TEST_MODE_HHK] = {"LED 1 red: HHK mode", FALCON_KALAM_RGB_MODE_HHK, 1500},
    [TEST_MODE_MAC] = {"LED 1 blue: Mac mode", FALCON_KALAM_RGB_MODE_MAC, 1500},
    [TEST_MODE_WIN] = {"LED 1 green: Win mode", FALCON_KALAM_RGB_MODE_WIN, 1500},
    [TEST_SHOW] = {"settings display: HHK+SW3 now, then Mac+SW4+SW5 after a replug", 0xFF, 4000},
    [TEST_DIP_OFF] = {"switch flipped off: one off-blink", 0xFF, 1000},
    [TEST_DIP_ON] = {"switch flipped on: two off-blinks", 0xFF, 1000},
    [TEST_RESET] = {"settings reset: three red blinks on dark", 0xFF, 2000},
    [TEST_BOOT] = {"bootloader: all red", FALCON_KALAM_RGB_BOOT, 1500},
};
static uint8_t  test_step = TEST_IDLE;
static uint16_t test_timer;

static void test_begin(uint8_t step) {
    test_step  = step;
    test_timer = timer_read();
    if (step == TEST_END) {
        uprintf("falcon_kalam: light test done\n");
        test_step = TEST_IDLE;
        shown     = 0; // re-evaluate the polled indicators on the next pass
        return;
    }
    uprintf("falcon_kalam: light test %u/%u: %s\n", step, TEST_END - 1, test_steps[step].name);
    switch (step) {
        case TEST_SHOW:
            falcon_kalam_rgb_show_settings(0 | (1u << 2), 1 | (2u << 2) | (4u << 2));
            break;
        case TEST_DIP_OFF:
            falcon_kalam_rgb_dip_feedback(false);
            break;
        case TEST_DIP_ON:
            falcon_kalam_rgb_dip_feedback(true);
            break;
        case TEST_RESET:
            rgblight_set_layer_state(FALCON_KALAM_RGB_DARK, true);
            rgblight_blink_layer_repeat(FALCON_KALAM_RGB_RESET, RESET_BLINK_MS, RESET_BLINKS);
            break;
        default:
            rgblight_set_layer_state(test_steps[step].layer, true);
            break;
    }
}

static void test_end_step(void) {
    uint8_t layer = test_steps[test_step].layer;
    if (layer != 0xFF) {
        rgblight_set_layer_state(layer, false);
    } else if (test_step == TEST_RESET) {
        rgblight_set_layer_state(FALCON_KALAM_RGB_DARK, false);
    }
}

void falcon_kalam_rgb_test_start(void) {
    if (test_step != TEST_IDLE) {
        test_end_step();
    }
    test_begin(TEST_LOCKED);
}

static void test_task(void) {
    if (timer_elapsed(test_timer) >= test_steps[test_step].ms) {
        test_end_step();
        test_begin(test_step + 1);
    }
}
#endif

void falcon_kalam_rgb_task(void) {
    usb_configure_state_t usb = usb_device_state_get_configure_state();
    // POLICY-HHKB: a mode base (default layer; an empty state means layer 0)
    // and a held Fn layer are normal; any other layer switched on, or another
    // default layer, is not.
    layer_state_t base   = default_layer_state ? default_layer_state : (layer_state_t)1;
    bool          locked = (layer_state & ~FALCON_KALAM_FN_LAYERS) || (base & ~FALCON_KALAM_BASE_LAYERS);

    // End of the dark start: the host is ready (or BOOT_WAIT_MS passed).
    if (booting && (usb == USB_DEVICE_STATE_CONFIGURED || timer_elapsed(boot_timer) >= BOOT_WAIT_MS)) {
        booting = false;
        if (settings_were_reset) {
            settings_were_reset = false;
            reset_dark          = true;
            reset_dark_timer    = timer_read();
            rgblight_blink_layer_repeat(FALCON_KALAM_RGB_RESET, RESET_BLINK_MS, RESET_BLINKS);
        } else {
            boot_light_up();
        }
    }
    if (reset_dark && timer_elapsed(reset_dark_timer) >= 2 * RESET_BLINK_MS * RESET_BLINKS) {
        reset_dark = false;
        boot_light_up();
    }

#ifdef FALCON_KALAM_DEV
    if (test_step != TEST_IDLE) {
        test_task();
        enforce_green_base();
        return;
    }
#endif

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
    enforce_green_base();
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

// Puts the data line into the WS2812 idle state (low, for a reset period)
// before the first frame. QMK's board defaults leave the pin high (an input
// with pull-up, output register 1) and ws2812_init() only switches the mode,
// so a frame sent right after it starts after a millisecond-long high: the
// first LED reads that as a leading 1 bit and shows green at 128 while LEDs
// 2-4 stay dark (docs/hardware.md §6). Low before the mode switch turns the
// pull-up into a pull-down, low after it is the output level.
static void ws2812_line_idle_low(void) {
    gpio_write_pin_low(WS2812_DI_PIN);
    ws2812_init();
    gpio_write_pin_low(WS2812_DI_PIN);
    wait_us(2 * WS2812_TRST_US);
}

// keyboard_pre_init_user: the earliest point this firmware reaches. Until the
// data pin is driven, the first LED can pick up noise (docs/hardware.md §6).
// Settle the line and send a dark frame now, long before rgblight starts;
// the second frame is a safeguard.
void falcon_kalam_rgb_pre_init(void) {
    ws2812_line_idle_low();
    ws2812_set_color_all(0, 0, 0);
    ws2812_flush();
    ws2812_flush();
}

// Bootmagic runs before rgblight is initialised, so drive the WS2812 driver
// directly: all LEDs red, latched while the bootloader runs (R15). Bypassing
// rgblight also bypasses its cap and brightness curve, so apply both here.
// The line is settled low first (the same sequence as at pre-init) and the
// frame is sent twice as a safeguard.
void falcon_kalam_rgb_paint_bootloader(void) {
    rgb_t red = hsv_to_rgb((hsv_t){FK_RED, RGBLIGHT_LIMIT_VAL});
    ws2812_line_idle_low();
    ws2812_set_color_all(red.r, red.g, red.b);
    ws2812_flush();
    ws2812_flush();
}

// POLICY-HHKB virtual DIP switch flipped: all LEDs blink off.
void falcon_kalam_rgb_dip_feedback(bool on) {
    rgblight_blink_layer_repeat(FALCON_KALAM_RGB_DARK, 150, on ? 2 : 1);
}

// POLICY-HHKB settings display (Fn+Ctrl+Shift+0 and the mode keys); a new
// request restarts it.
void falcon_kalam_rgb_show_settings(uint8_t now, uint8_t next) {
    show_gap(false);
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
