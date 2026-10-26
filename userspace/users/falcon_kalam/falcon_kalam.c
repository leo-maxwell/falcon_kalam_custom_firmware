// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The only place that defines QMK's *_user / weak hooks for falcon_kalam. Each
// hook may exist once per firmware, so modules (rgb_indicators.c, ...) expose
// plain functions and this file dispatches to them.

#include QMK_KEYBOARD_H
#include "rgb_indicators.h"
#include "hhkb_modes.h"
#ifdef FALCON_KALAM_DEV
#    include "print.h"
#endif

// Layout of the saved settings (eeconfig user word). Bump it whenever the
// keymap layers or anything else stored in EEPROM changes meaning: a keyboard
// with an older stamp then resets its settings once, so the new defaults take
// effect even if DFU was entered without the Esc reset. VIA's own check cannot
// do this, because it keys on the build date, which is pinned (reproducible
// builds). 1 = six HHKB mode layers (POLICY-HHKB).
#define FALCON_KALAM_EEPROM_SCHEMA 0x464B0001UL // "FK" + version

// Runs after the EEPROM driver starts and before QMK reads the settings: a
// wrong stamp marks them invalid, and QMK re-initialises all of them (VIA
// keymaps and macros included) right after.
void keyboard_pre_init_user(void) {
    if (eeconfig_is_enabled() && eeconfig_read_user() != FALCON_KALAM_EEPROM_SCHEMA) {
        eeconfig_disable();
    }
}

void keyboard_post_init_user(void) {
    falcon_kalam_rgb_init();
}

void housekeeping_task_user(void) {
    falcon_kalam_rgb_task();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    return falcon_kalam_hhkb_mode_process(record);
}

bool shutdown_user(bool jump_to_bootloader) {
    falcon_kalam_rgb_shutdown(jump_to_bootloader);
    return true;
}

// Called by QMK while (re)initialising EEPROM, before the LEDs exist.
void eeconfig_init_user(void) {
    eeconfig_update_user(FALCON_KALAM_EEPROM_SCHEMA);
    falcon_kalam_rgb_note_settings_reset();
}

// Bootmagic (Esc held at plug-in): QMK's default only clears EEPROM; also
// paint the LEDs red so DFU entered this way is not dark (POLICY-RGB, R15).
void bootmagic_reset_eeprom(void) {
    eeconfig_disable();
    falcon_kalam_rgb_paint_bootloader();
}

#ifdef FALCON_KALAM_DEV
// Dev Command extra. Shift+Shift+F: deliberately fault the CPU to prove that
// POLICY-RECOVERY lands in DFU (hardware test for R13). Dev image only.
bool command_extra(uint8_t code) {
    if (code != KC_F) {
        return false;
    }
    uprintf("falcon_kalam: fault test, expect DFU\n");
    wait_ms(50); // let the console report go out
    __asm__ volatile("udf #0");
    return true;
}
#endif
