// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-FLASHREAD (dev variant only, POLICY-VARIANTS): compiled when the
// variant's rules.mk sets FALCON_FLASHREAD = yes; the ship variant refuses it.

#include QMK_KEYBOARD_H
#include "raw_hid.h"
#include "via.h"

// POLICY-FLASHREAD: read-only raw-HID command so the bootloader can be backed up
// once this firmware runs (DFU alt 2 cannot read below 0x08002000). Only the
// ranges below are readable; nothing is ever written. Host side:
// tools/falcon_probe.py dump-bootloader.
//
//   request : F0 a3 a2 a1 a0 len           (address big-endian, len <= 24)
//   reply   : F0 a3 a2 a1 a0 len  data...  on success
//             F0 a3 a2 a1 a0 00            if the range is not allowed
#define FALCON_CMD_READ 0xF0
#define FALCON_READ_MAX 24

static bool falcon_read_allowed(uint32_t addr, uint8_t len) {
    static const struct {
        uint32_t start, end; // end exclusive
    } ranges[] = {
        {0x08000000, 0x08002000}, // STM32duino/Maple bootloader (8 KB)
        {0x1FFFF7E0, 0x1FFFF7F0}, // F_SIZE + 96-bit unique ID
        {0x1FFFF800, 0x1FFFF810}, // option bytes
    };
    for (uint8_t i = 0; i < sizeof(ranges) / sizeof(ranges[0]); i++) {
        if (addr >= ranges[i].start && addr + len <= ranges[i].end && addr + len >= addr) {
            return true;
        }
    }
    return false;
}

// noinline: keeps the handler a distinct symbol under LTO so scripts/verify_firmware.py can find it.
__attribute__((noinline)) bool via_command_kb(uint8_t *data, uint8_t length) {
    if (data[0] != FALCON_CMD_READ || length < 6 + FALCON_READ_MAX) {
        return false;
    }
    uint32_t addr = ((uint32_t)data[1] << 24) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 8) | data[4];
    uint8_t  len  = data[5];
    if (len == 0 || len > FALCON_READ_MAX || !falcon_read_allowed(addr, len)) {
        data[5] = 0;
    } else {
        const volatile uint8_t *src = (const volatile uint8_t *)addr;
        for (uint8_t i = 0; i < len; i++) {
            data[6 + i] = src[i];
        }
    }
    raw_hid_send(data, length);
    return true;
}
