#!/usr/bin/env python3
"""Compare what the stock image and our release image expose, from the binaries.

Extracts, for each image: USB device/configuration descriptors (interfaces,
endpoints, polling interval, power), which HID reports exist (boot keyboard,
VIA raw, console, mouse/system/consumer/NKRO), matrix pin tables, the clock
configuration literal and the rgblight effect table. Prints a side-by-side
table; docs/parity.md interprets it.

    python3 -I tools/feature_parity.py STOCK.bin RELEASE.bin [--report OUT]

A 16-byte DFU suffix ("UFD") is stripped automatically. Read-only, stdlib only.
"""

import argparse
import re
import struct
import sys
from pathlib import Path

HID_REPORTS = {  # unique byte signatures from QMK's report descriptors
    'boot keyboard (6KRO)': b'\x05\x01\x09\x06\xa1\x01\x05\x07',
    'VIA raw HID 0xFF60': b'\x06\x60\xff\x09\x61',
    'console 0xFF31': b'\x06\x31\xff\x09\x74',
    'mouse (ID 2)': b'\x05\x01\x09\x02\xa1\x01\x85\x02',
    'system control (ID 3)': b'\x05\x01\x09\x80\xa1\x01\x85\x03',
    'consumer (ID 4)': b'\x05\x0c\x09\x01\xa1\x01\x85\x04',
    'NKRO bitmap (ID 6)': b'\x05\x01\x09\x06\xa1\x01\x85\x06',
}
ROWS = b''.join(struct.pack('<I', v) for v in (0x40010C03, 0x40010C04, 0x40010C05, 0x40010C06, 0x40010802))
COLS = b''.join(struct.pack('<I', v) for v in (0x40010809, 0x40010803, 0x40010804, 0x40010805, 0x40010806, 0x40010807,
                                                 0x40010C00, 0x40010C01, 0x40010C0A, 0x40010C0B, 0x40010C0C, 0x40010C0D,
                                                 0x40010C0E, 0x40010C0F, 0x40010808))
ALL_MODES = bytes([0, 1] + [2] * 4 + [6] * 3 + [9] * 6 + [15] * 6 + [21] * 3 + [24] + [25] * 10 + [35, 36] + [37] * 6)


def load(path):
    d = Path(path).read_bytes()
    return d[:-16] if d[-16:][8:11] == b'UFD' else d


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
    f['clock RCC_CFGR 0x001D6400'] = 'yes' if struct.pack('<I', 0x001D6400) in d else 'no'
    f['rgblight: all 43 effects'] = 'yes' if ALL_MODES + b'\x00' in d or ALL_MODES in d else 'no/other'
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
    for k in s:
        lines.append('%-38s %-28s %-28s %s' % (k, s[k], r.get(k), '=' if s[k] == r.get(k) else '*'))
    text = '\n'.join(lines) + '\n'
    sys.stdout.write(text)
    if a.report:
        a.report.write_text(text)


if __name__ == '__main__':
    main()
