#!/usr/bin/env python3
"""Facts shared by the scripts and tools: addresses, sizes, USB ids, byte
signatures. Every value is established in docs/hardware.md or
docs/stock-firmware.md; this module is their single copy in code.

    python3 -I tools/falcon_kalam_facts.py --shell    print them as shell assignments (eval'd by the scripts)
"""

import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Flash map (docs/hardware.md §2).
FLASH_BASE = 0x08000000
APP_START = 0x08002000       # end of the 8 KB STM32duino bootloader
EEPROM_START = 0x0800F000    # 4 KB wear-leveling backing store (stock: 2 KB at 0x0800F800)
FLASH_END = 0x08010000       # 64 KB part
APP_REGION_BYTES = FLASH_END - APP_START  # one DFU upload or readback
RAM_START, RAM_END = 0x20000000, 0x20005000  # 20 KB

# USB identities (docs/hardware.md §3, docs/stock-firmware.md §2).
DFU_VID, DFU_PID = 0x1EAF, 0x0003  # STM32duino/Maple bootloader
APP_VID, APP_PID = 0x9DD1, 0x9D89  # the keyboard, stock and ours

# Stock image (docs/stock-firmware.md §1).
STOCK_APP_BYTES = 43940

# STM32F1 GPIO port bases; ChibiOS ioline_t = port base | pad.
GPIO_PORT_BASE = {'A': 0x40010800, 'B': 0x40010C00, 'C': 0x40011000, 'D': 0x40011400, 'E': 0x40011800}

# Matrix pins and where the stock image keeps its pin tables (docs/hardware.md §1, §5).
VENDOR_ROW_PINS_ADDR, VENDOR_COL_PINS_ADDR = 0x0800C464, 0x0800C10C
VENDOR_ROW_PINS = ['B3', 'B4', 'B5', 'B6', 'A2']
VENDOR_COL_PINS = ['A9', 'A3', 'A4', 'A5', 'A6', 'A7', 'B0', 'B1', 'B10', 'B11', 'B12', 'B13', 'B14', 'B15', 'A8']

# Clock and flash descriptor literals present in every image (docs/hardware.md §4, §2).
RCC_CFGR = 0x001D6400
EFL_DESCRIPTOR = (3, 2, 128, 0, 1024, 0x08000000, 0x20000)  # ChibiOS EFL: 128 x 1 KB sectors

# rgblight's mode_base_table with every effect enabled (43 entries: mode 0 + 42 effect modes).
RGBLIGHT_ALL_MODES = bytes([0, 1] + [2] * 4 + [6] * 3 + [9] * 6 + [15] * 6 + [21] * 3 + [24] + [25] * 10 + [35, 36] + [37] * 6)

# Unique byte signatures of QMK's HID report descriptors.
HID_REPORTS = {
    'boot keyboard (6KRO)': bytes.fromhex('05010906a1010507'),
    'VIA raw HID 0xFF60': bytes.fromhex('0660ff0961'),
    'console 0xFF31': bytes.fromhex('0631ff0974'),
    'mouse (ID 2)': bytes.fromhex('05010902a1018502'),
    'system control (ID 3)': bytes.fromhex('05010980a1018503'),
    'consumer (ID 4)': bytes.fromhex('050c0901a1018504'),
    'NKRO bitmap (ID 6)': bytes.fromhex('05010906a1018506'),
}


def ioline_bytes(pins):
    """The little-endian ioline_t table QMK compiles for a pin list."""
    return b''.join(struct.pack('<I', GPIO_PORT_BASE[p[0]] | int(p[1:])) for p in pins)


def ioline_name(value):
    port = next((n for n, base in GPIO_PORT_BASE.items() if base == value & ~0xF), None)
    return '%s%d' % (port, value & 0xF) if port else None


def strip_dfu_suffix(data):
    """(payload, 16-byte suffix or None): a DFU suffix ends in the signature "UFD"."""
    return (data[:-16], data[-16:]) if data[-16:][8:11] == b'UFD' else (data, None)


def matrix_size():
    """(rows, cols) from keyboards/falcon_kalam/keyboard.json."""
    kb = json.loads((ROOT / 'keyboards' / 'falcon_kalam' / 'keyboard.json').read_text())
    return len(kb['matrix_pins']['rows']), len(kb['matrix_pins']['cols'])


def main():
    import sys
    if sys.argv[1:] != ['--shell']:
        sys.exit('usage: falcon_kalam_facts.py --shell')
    print('FALCON_KALAM_APP_START=0x%08X' % APP_START)
    print('FALCON_KALAM_APP_START_SHORT=0x%x' % APP_START)  # as dfu-util prints it
    print('FALCON_KALAM_EEPROM_START=0x%08X' % EEPROM_START)
    print('FALCON_KALAM_APP_REGION_BYTES=%d' % APP_REGION_BYTES)
    print('FALCON_KALAM_DFU_ID=%04x:%04x' % (DFU_VID, DFU_PID))
    print('FALCON_KALAM_APP_ID=%04x:%04x' % (APP_VID, APP_PID))
    print('FALCON_KALAM_STOCK_APP_BYTES=%d' % STOCK_APP_BYTES)


if __name__ == '__main__':
    main()
