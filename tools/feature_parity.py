#!/usr/bin/env python3
"""Compare what the stock image and our release image expose, from the binaries.

Extracts, for each image: USB device/configuration descriptors (interfaces,
endpoints, polling interval, power), which HID reports exist (boot keyboard,
VIA raw, console, mouse/system/consumer/NKRO), matrix pin tables, the clock
configuration literal and the rgblight effect table. Prints a side-by-side
table; docs/firmware.md and docs/stock-firmware.md interpret it.

    python3 -I tools/feature_parity.py STOCK.bin RELEASE.bin [--report OUT]

A 16-byte DFU suffix ("UFD") is stripped automatically. Read-only, stdlib only.
"""

import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))  # python3 -I drops the script directory
from falcon_kalam_facts import HID_REPORTS, RCC_CFGR, RGBLIGHT_ALL_MODES, VENDOR_COL_PINS, VENDOR_ROW_PINS, ioline_bytes, strip_dfu_suffix

ROWS, COLS = ioline_bytes(VENDOR_ROW_PINS), ioline_bytes(VENDOR_COL_PINS)  # the pin tables as compiled
# Peripheral blocks (STM32F103/GD32F103 memory map). A block referenced by only
# one image (e.g. a timer for PWM backlight, another GPIO port for an LED) is a
# feature-gap candidate; this is how hardware outputs are compared, since
# descriptors cannot show them.
PERIPHERALS = [(0x40000000, 'TIM2'), (0x40000400, 'TIM3'), (0x40000800, 'TIM4'), (0x40002800, 'RTC'),
               (0x40002C00, 'WWDG'), (0x40003000, 'IWDG'), (0x40003800, 'SPI2'), (0x40004400, 'USART2'),
               (0x40004800, 'USART3'), (0x40005400, 'I2C1'), (0x40005800, 'I2C2'), (0x40005C00, 'USB'),
               (0x40006000, 'USB SRAM'), (0x40006400, 'CAN'), (0x40006C00, 'BKP'), (0x40007000, 'PWR'),
               (0x40010000, 'AFIO'), (0x40010400, 'EXTI'), (0x40010800, 'GPIOA'), (0x40010C00, 'GPIOB'),
               (0x40011000, 'GPIOC'), (0x40011400, 'GPIOD'), (0x40011800, 'GPIOE'), (0x40012400, 'ADC1'),
               (0x40012800, 'ADC2'), (0x40012C00, 'TIM1'), (0x40013000, 'SPI1'), (0x40013800, 'USART1'),
               (0x40020000, 'DMA1'), (0x40021000, 'RCC'), (0x40022000, 'FLASH'), (0x40023000, 'CRC'),
               (0xE000E000, 'NVIC/SysTick'), (0xE000ED00, 'SCB'), (0xE0001000, 'DWT'), (0xE000EDF0, 'CoreDebug')]


def load(path):
    return strip_dfu_suffix(Path(path).read_bytes())[0]


def config_descriptor(d):
    """Find the first plausible USB configuration descriptor and walk it.

    A configuration descriptor starts 09 02 <wTotalLength> <bNumInterfaces> 01 00
    <bmAttributes with bit 7 set> <bMaxPower>; the sub-descriptors that follow
    are length-prefixed, so walking them validates the match.
    """
    for m in re.finditer(rb'\x09\x02(..)(.)\x01\x00[\x80-\xff](.)', d, re.S):
        total = struct.unpack('<H', m.group(1))[0]
        blk = d[m.start():m.start() + total]
        if not 34 <= total <= 512 or len(blk) != total:
            continue
        info = {'interfaces': [], 'power_mA': blk[8] * 2, 'remote_wakeup': bool(blk[7] & 0x20)}
        i = 9
        while i < total and blk[i]:
            length, kind = blk[i], blk[i + 1]
            if kind == 4:
                info['interfaces'].append({'class': (blk[i + 5], blk[i + 6], blk[i + 7]), 'eps': []})
            elif kind == 5 and info['interfaces']:
                info['interfaces'][-1]['eps'].append((blk[i + 2], struct.unpack('<H', blk[i + 4:i + 6])[0], blk[i + 6]))
            i += length
        if i == total and len(info['interfaces']) == blk[4]:
            return info
    return None


def peripherals(d):
    """Peripheral blocks whose base (+ up to 1 KB) appears as a 32-bit literal."""
    used = set()
    for i in range(0, len(d) - 3, 4):
        v = struct.unpack_from('<I', d, i)[0]
        for base, name in PERIPHERALS:
            if base <= v < base + 0x400:
                used.add(name)
    return used


def features(d):
    f = {'size (bytes)': len(d)}
    dev = re.search(rb'\x12\x01\x00\x02\x00\x00\x00\x40(..)(..)(..)', d, re.S)
    f['USB VID:PID / bcdDevice'] = '%04X:%04X / %04X' % tuple(struct.unpack('<H', g)[0] for g in dev.groups()) if dev else '-'
    cfg = config_descriptor(d)
    if cfg:
        f['HID interfaces'] = len(cfg['interfaces'])
        f['endpoints (addr/size/interval ms)'] = ' '.join('%02X/%d/%d' % ep for itf in cfg['interfaces'] for ep in itf['eps'])
        f['bMaxPower / remote wakeup'] = '%d mA / %s' % (cfg['power_mA'], 'yes' if cfg['remote_wakeup'] else 'no')
    for name, sig in HID_REPORTS.items():
        f['HID: ' + name] = 'yes' if sig in d else 'no'
    f['matrix pins (vendor tables)'] = 'yes' if ROWS in d and COLS in d else 'no'
    f['clock RCC_CFGR 0x%08X' % RCC_CFGR] = 'yes' if struct.pack('<I', RCC_CFGR) in d else 'no'
    f['peripherals referenced'] = ' '.join(sorted(peripherals(d)))
    f['rgblight: all 42 effect modes'] = 'yes' if RGBLIGHT_ALL_MODES in d else 'no/other'
    return f


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('stock')
    ap.add_argument('release')
    ap.add_argument('--report', type=Path)
    a = ap.parse_args()
    s, r = features(load(a.stock)), features(load(a.release))
    lines = ['# Stock vs release, extracted from the binaries', '',
             '%-38s %-28s %-28s %s' % ('item', 'stock', 'release', 'same')]
    for k in list(s) + [k for k in r if k not in s]:
        lines.append('%-38s %-28s %-28s %s' % (k, s.get(k), r.get(k), '=' if s.get(k) == r.get(k) else '*'))
    text = '\n'.join(lines) + '\n'
    sys.stdout.write(text)
    if a.report:
        a.report.write_text(text)


if __name__ == '__main__':
    main()
