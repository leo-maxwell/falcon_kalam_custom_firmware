// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The only place that defines QMK's *_user hooks and the shared weak hooks
// (bootmagic_reset_eeprom, command_extra) for falcon_kalam. Each hook may exist
// once per firmware, so modules (rgb_indicators.c, ...) expose plain functions
// and this file dispatches to them. Two single-owner hooks stay with their
// module: via_command_kb (flashread.c, dev) and the fault vectors (recovery.c).

#include QMK_KEYBOARD_H
#include "rgb_indicators.h"
#include "hhkb_modes.h"
#include "caps_led.h"
#include "user_config.h"
#ifdef FALCON_KALAM_DEV
#    include "print.h"
#endif

// Runs after the EEPROM driver starts and before QMK reads the settings: a
// schema stamp other than FALCON_KALAM_EEPROM_SCHEMA (user_config.h) marks
// them invalid, and QMK re-initialises all of them (VIA keymaps and macros
// included) right after.
void keyboard_pre_init_user(void) {
    falcon_kalam_rgb_pre_init();
    falcon_kalam_user_config_t cfg = {.raw = eeconfig_read_user()};
    if (eeconfig_is_enabled() && cfg.schema != FALCON_KALAM_EEPROM_SCHEMA) {
        eeconfig_disable();
    }
}

void keyboard_post_init_user(void) {
    falcon_kalam_hhkb_init();
    falcon_kalam_rgb_init();
}

#ifdef FALCON_KALAM_DEV
static bool     fault_pending;
static uint16_t fault_timer;
#endif

void housekeeping_task_user(void) {
    falcon_kalam_rgb_task();
    falcon_kalam_caps_led_task();
#ifdef FALCON_KALAM_DEV
    // Dev fault test (command_extra): fault from the main loop once the
    // console report has had time to go out (the console is sent from here).
    if (fault_pending && timer_elapsed(fault_timer) >= 200) {
        __asm__ volatile("udf #0");
    }
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    return falcon_kalam_hhkb_process(keycode, record);
}

bool led_update_user(led_t led_state) {
    return falcon_kalam_caps_led_update(led_state);
}

bool shutdown_user(bool jump_to_bootloader) {
    falcon_kalam_rgb_shutdown(jump_to_bootloader);
    return true;
}

// Called by QMK while (re)initialising EEPROM, before the LEDs exist.
void eeconfig_init_user(void) {
    falcon_kalam_user_config_t cfg = {.schema = FALCON_KALAM_EEPROM_SCHEMA, .dip = FALCON_KALAM_DIP_FACTORY};
    eeconfig_update_user(cfg.raw);
    falcon_kalam_rgb_note_settings_reset();
}

// Bootmagic (Esc held at plug-in): QMK's default only clears EEPROM; also
// paint the LEDs red so DFU entered this way is not dark (POLICY-RGB, R15).
void bootmagic_reset_eeprom(void) {
    eeconfig_disable();
    falcon_kalam_rgb_paint_bootloader();
}

#ifdef FALCON_KALAM_DEV
// Dev Command extras. Shift+Shift+F: deliberately fault the CPU to prove that
// POLICY-RECOVERY lands in DFU (hardware test for R13). Shift+Shift+L: the
// light test, every indicator state in turn (POLICY-RGB). Dev image only.
bool command_extra(uint8_t code) {
    if (code == KC_L) {
        falcon_kalam_rgb_test_start();
        return true;
    }
    if (code != KC_F) {
        return false;
    }
    uprintf("falcon_kalam: fault test, expect DFU\n");
    fault_pending = true; // faults from housekeeping_task_user after 200 ms
    fault_timer   = timer_read();
    return true;
}
#endif
