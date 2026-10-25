// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Falcon HS R2 (GD32F103C8T6, STM32duino/Maple DFU bootloader).
// Base keyboard definition: hardware facts only, no policy. Every value is
// traced to the manufacturer firmware in docs/hardware.md. Project policy
// (NKRO off, VIA, keymap) lives in the userspace layer, see docs/architecture.md.

#pragma once

// The vendor image locates its wear-leveling area at runtime from the F_SIZE
// register and its data sits at 0x0800F800-0x0800FFFF in the flash readback,
// i.e. F_SIZE reads 64 KB on this unit. Pin the size so the location does not
// depend on GD32 F_SIZE semantics. Must match the linker script
// (ld/STM32F103x8_stm32duino_falcon_hs_r2.ld) and eeprom.wear_leveling in
// keyboard.json.
#define WEAR_LEVELING_EFL_FLASH_SIZE (64 * 1024)
