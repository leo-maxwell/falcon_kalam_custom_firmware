// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The only place that defines QMK's *_user / weak hooks for falcon_kalam. Each
// hook may exist once per firmware, so modules (rgb_indicators.c, ...) expose
// plain functions and this file dispatches to them.

#include QMK_KEYBOARD_H
#include "rgb_indicators.h"
#ifdef FALCON_KALAM_DEV
#    include "print.h"
#endif

void keyboard_post_init_user(void) {
    falcon_kalam_rgb_init();
}

void housekeeping_task_user(void) {
    falcon_kalam_rgb_task();
}

bool shutdown_user(bool jump_to_bootloader) {
    falcon_kalam_rgb_shutdown(jump_to_bootloader);
    return true;
}

// Called by QMK while (re)initialising EEPROM, before the LEDs exist.
void eeconfig_init_user(void) {
#if (EECONFIG_USER_DATA_SIZE) == 0
    eeconfig_update_user(0); // what QMK's default implementation does
#endif
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
