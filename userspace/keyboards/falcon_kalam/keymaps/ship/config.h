// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ship variant (POLICY-VARIANTS). Shared configuration: users/falcon_kalam/config.h.

#pragma once

// Selective Command (POLICY-PARITY, decided 2026-10-26). Kept with
// LShift+RShift: H help, D debug, X matrix print, K key reports, V version,
// S status, B bootloader. The rest are pointed at keys no keymap layer sends
// (International 1-3 and 6-9, Language 1-3; HHK mode's ◇ keys send
// International 4/5, POLICY-HHKB), so LShift+RShift+<that key> types normally.
// The verifier checks that no layer contains them. Each needs a distinct key
// (command.c switches on them), and plain names such as F13 cannot be used:
// QMK defines F13 as a pin macro (GPIOF pin 13).
#define MAGIC_KEY_SWITCH_LAYER_WITH_FKEYS false         // Shift+Shift+F1..F10: default layer
#define MAGIC_KEY_SWITCH_LAYER_WITH_NKEYS false         // Shift+Shift+1..0: default layer
#define MAGIC_KEY_HELP_ALT INTERNATIONAL_1              // was / (help, alternate)
#define MAGIC_KEY_DEBUG_MOUSE INTERNATIONAL_2           // was M (mouse debug)
#define MAGIC_KEY_CONSOLE INTERNATIONAL_3               // was C (console sub-mode)
#define MAGIC_KEY_LAYER0_ALT INTERNATIONAL_6            // was ` (default layer 0)
#define MAGIC_KEY_BOOTLOADER_ALT INTERNATIONAL_7        // was Esc (bootloader, alternate)
#define MAGIC_KEY_LOCK INTERNATIONAL_8                  // was Caps Lock (keyboard lock)
#define MAGIC_KEY_EEPROM INTERNATIONAL_9                // was E (print EEPROM)
#define MAGIC_KEY_EEPROM_CLEAR LANGUAGE_2               // was Backspace (wipe settings)
#define MAGIC_KEY_NKRO LANGUAGE_3                       // was N (NKRO toggle; no NKRO here)
#define MAGIC_KEY_SLEEP_LED LANGUAGE_1                  // was Z (sleep LED)
