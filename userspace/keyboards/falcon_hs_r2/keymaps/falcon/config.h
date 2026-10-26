// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ship variant (POLICY-VARIANTS). Shared configuration: users/falcon/config.h.

#pragma once

// Selective Command (POLICY-PARITY, decided 2026-10-26). Kept with
// LShift+RShift: H help, D debug, X matrix print, K key reports, V version,
// S status, B bootloader. The rest are pointed at keys a US HHKB does not have
// (International 1-9, Language 1), so LShift+RShift+<that key> types normally.
// Each needs a distinct key (command.c switches on them), and plain names such
// as F13 cannot be used: QMK defines F13 as a pin macro (GPIOF pin 13).
#define MAGIC_KEY_SWITCH_LAYER_WITH_FKEYS false         // Shift+Shift+F1..F10: default layer
#define MAGIC_KEY_SWITCH_LAYER_WITH_NKEYS false         // Shift+Shift+1..0: default layer
#define MAGIC_KEY_HELP_ALT INTERNATIONAL_1              // was / (help, alternate)
#define MAGIC_KEY_DEBUG_MOUSE INTERNATIONAL_2           // was M (mouse debug)
#define MAGIC_KEY_CONSOLE INTERNATIONAL_3               // was C (console sub-mode)
#define MAGIC_KEY_LAYER0_ALT INTERNATIONAL_4            // was ` (default layer 0)
#define MAGIC_KEY_BOOTLOADER_ALT INTERNATIONAL_5        // was Esc (bootloader, alternate)
#define MAGIC_KEY_LOCK INTERNATIONAL_6                  // was Caps Lock (keyboard lock)
#define MAGIC_KEY_EEPROM INTERNATIONAL_7                // was E (print EEPROM)
#define MAGIC_KEY_EEPROM_CLEAR INTERNATIONAL_8          // was Backspace (wipe settings)
#define MAGIC_KEY_NKRO INTERNATIONAL_9                  // was N (NKRO toggle; no NKRO here)
#define MAGIC_KEY_SLEEP_LED LANGUAGE_1                  // was Z (sleep LED)
