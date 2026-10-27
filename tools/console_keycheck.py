#!/usr/bin/env python3
"""Check key presses in a dev-image console log against POLICY-HHKB. Offline.

The dev image traces every key event (ACTION_DEBUG) and every EEPROM read
(WEAR_LEVELING_DEBUG_OUTPUT). A key lookup reads the VIA keymap from EEPROM,
so the last 16-bit read before the event's action names the layer, row, column
and keycode the firmware actually used. This tool lists, per press, the
position, layer and keycode, and compares them with the table that
scripts/verify_firmware.py builds from the HHKB Classic manual.

    tools/console_keycheck.py LOG [--start HH:MM:SS] [--end HH:MM:SS]
"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))
import verify_firmware as vf  # noqa: E402

ROWS, COLS = 5, 15
# DYNAMIC_KEYMAP_EEPROM_ADDR in QMK 0.34.7: eeconfig 37 B (eeprom_core_t in
# quantum/nvm/eeprom/nvm_eeprom_eeconfig_internal.h; no KB/user/module data) +
# VIA magic 3 B + layout options 1 B (quantum/via.h). Re-derive after a QMK
# update (docs/operations.md §5); a wrong value shows as 'lookup at r,c' mismatches.
KEYMAP_BASE = 41

EVENT = re.compile(r'(\d\d:\d\d:\d\d) .*EVENT: ([0-9A-F]{2})([0-9A-F]{2})([du])')
READ = re.compile(r'Wear leveling: Read  \[0x([0-9A-F]{4})\]:  ([0-9A-F]{2})')
ACTION = re.compile(r'ACTION: ')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('log', type=Path)
    ap.add_argument('--start', default='00:00:00')
    ap.add_argument('--end', default='99:99:99')
    a = ap.parse_args()

    kb = json.loads((ROOT / 'keyboards' / 'falcon_kalam' / 'keyboard.json').read_text())
    positions = [tuple(k['matrix']) for k in kb['layouts']['LAYOUT_60_hhkb']['layout']]
    kc = vf.qmk_keycodes()
    names = {}
    for n, v in kc.items():
        if v is not None and n.startswith('KC_') and (v not in names or len(n) < len(names[v])):
            names[v] = n
    want = vf.hhkb_expected(kc, positions, ROWS, COLS)
    legend = {p: vf.HHKB_KEYS[i] for i, p in enumerate(positions)}

    presses, cur, reads = [], None, {}
    for line in a.log.read_text(errors='ignore').splitlines():
        m = EVENT.search(line)
        if m:
            t, row, col, ud = m.group(1), int(m.group(2), 16), int(m.group(3), 16), m.group(4)
            cur = {'t': t, 'pos': (row, col), 'down': ud == 'd', 'last': None} if a.start <= t <= a.end else None
            reads = {}
            continue
        if cur is None:
            continue
        m = READ.search(line)
        if m:
            addr, val = int(m.group(1), 16), int(m.group(2), 16)
            reads[addr] = val
            if addr - 1 >= KEYMAP_BASE and addr - 1 in reads and (addr - 1 - KEYMAP_BASE) % 2 == 0:
                idx = (addr - 1 - KEYMAP_BASE) // 2
                cur['last'] = (idx // (ROWS * COLS), (idx // COLS) % ROWS, idx % COLS, (reads[addr - 1] << 8) | val)
            continue
        if ACTION.search(line) and cur['down']:
            presses.append(cur)
            cur = None

    seen, bad = set(), []
    for p in presses:
        if not p['last']:
            continue
        layer, row, col, code = p['last']
        if (row, col) != p['pos']:
            bad.append('%s %s: lookup at %d,%d' % (p['t'], p['pos'], row, col))
            continue
        exp = want[layer][row][col]
        mark = 'ok ' if code == exp else 'BAD'
        if code != exp:
            bad.append('%s layer %d %s (%d,%d): 0x%04X %s, want 0x%04X %s' % (p['t'], layer, legend.get((row, col), '?'), row, col,
                                                                          code, names.get(code, ''), exp, names.get(exp, '')))
        seen.add((layer, row, col))
        print('%s %s layer %d  %-7s (%d,%2d)  0x%04X %s' % (mark, p['t'], layer, legend.get((row, col), '?'), row, col, code, names.get(code, '')))
    for layer in sorted({l for l, _, _ in seen}):
        hit = {(r, c) for l, r, c in seen if l == layer}
        missing = [legend[p] for p in positions if p not in hit]
        print('layer %d: %d of %d HHKB positions pressed%s' % (layer, len(hit & set(positions)), len(positions),
                                                              ('; missing: ' + ' '.join(missing)) if missing else ''))
    print('mismatches: %d' % len(bad))
    for b in bad:
        print('  ' + b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
