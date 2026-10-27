#!/opt/homebrew/opt/qmk/libexec/bin/python
"""Read-only USB probe for falcon_kalam over the VIA raw-HID interface.

Works with the manufacturer firmware (VIA) and with this project's ship and
dev images. It only sends query commands. A whitelist enforced in send() makes
it impossible to issue any VIA command that writes EEPROM, resets, or jumps to
the bootloader.

    tools/falcon_kalam_probe.py info                      protocol, layers, layout options
    tools/falcon_kalam_probe.py dump-keymap OUT.json      all dynamic-keymap layers
    tools/falcon_kalam_probe.py matrix [--log OUT.csv]    live matrix positions (layout settlement)
    tools/falcon_kalam_probe.py dump-bootloader OUTDIR    0x08000000-0x08001FFF + option bytes + F_SIZE/UID
                                                    (dev image only: POLICY-FLASHREAD)
    tools/falcon_kalam_probe.py decode-log CSV            re-map a matrix log of the pre-fix decoder (offline, B2)

Uses the `hid` module bundled with the Homebrew QMK CLI (hence the shebang).
"""

import argparse
import hashlib
import json
import sys
import time
from pathlib import Path

import hid

sys.path.insert(0, str(Path(__file__).resolve().parent))  # python3 -I drops the script directory
from falcon_kalam_facts import APP_PID, APP_VID, matrix_size

VID, PID = APP_VID, APP_PID
USAGE_PAGE, USAGE = 0xFF60, 0x61
ROWS, COLS = matrix_size()
REPORT = 32

# VIA command ids (quantum/via.h); everything not listed here is refused.
GET_PROTOCOL_VERSION = 0x01
GET_KEYBOARD_VALUE = 0x02
CUSTOM_GET_VALUE = 0x08  # protocol >= 12: channel/value; older: lighting_get_value
MACRO_GET_COUNT = 0x0C
MACRO_GET_BUFFER_SIZE = 0x0D
DYNAMIC_KEYMAP_GET_LAYER_COUNT = 0x11
DYNAMIC_KEYMAP_GET_BUFFER = 0x12
FALCON_KALAM_READ = 0xF0
READ_ONLY_COMMANDS = {GET_PROTOCOL_VERSION, GET_KEYBOARD_VALUE, CUSTOM_GET_VALUE, MACRO_GET_COUNT, MACRO_GET_BUFFER_SIZE,
                      DYNAMIC_KEYMAP_GET_LAYER_COUNT, DYNAMIC_KEYMAP_GET_BUFFER, FALCON_KALAM_READ}
RGBLIGHT_CHANNEL = 0x02  # id_qmk_rgblight_channel (protocol >= 12)
RGB_VALUES = (('brightness', 1, 0x80), ('effect', 2, 0x81), ('effect_speed', 3, 0x82), ('color(h,s)', 4, 0x83))
# Sub-ids of GET_KEYBOARD_VALUE (all reads).
KV_UPTIME, KV_LAYOUT_OPTIONS, KV_SWITCH_MATRIX_STATE, KV_FIRMWARE_VERSION = 0x01, 0x02, 0x03, 0x04
UNHANDLED = 0xFF


class Probe:
    def __init__(self):
        paths = [d['path'] for d in hid.enumerate(VID, PID) if d['usage_page'] == USAGE_PAGE and d['usage'] == USAGE]
        if not paths:
            sys.exit('No %04X:%04X raw-HID (0xFF60) interface found. Is the keyboard out of DFU mode?' % (VID, PID))
        self.dev = hid.Device(path=paths[0])

    def send(self, *payload):
        if payload[0] not in READ_ONLY_COMMANDS:
            raise RuntimeError('refusing non-read-only command 0x%02X' % payload[0])
        if payload[0] == GET_KEYBOARD_VALUE and payload[1] not in (KV_UPTIME, KV_LAYOUT_OPTIONS, KV_SWITCH_MATRIX_STATE, KV_FIRMWARE_VERSION):
            raise RuntimeError('refusing unknown keyboard-value id 0x%02X' % payload[1])
        msg = bytes(payload) + bytes(REPORT - len(payload))
        self.dev.write(b'\x00' + msg)
        for _ in range(10):
            reply = self.dev.read(REPORT, timeout=500)
            if reply and reply[0] in (payload[0], UNHANDLED):
                return reply
        raise TimeoutError('no reply to command 0x%02X' % payload[0])

    def protocol(self):
        r = self.send(GET_PROTOCOL_VERSION)
        return (r[1] << 8) | r[2]

    def layers(self):
        return self.send(DYNAMIC_KEYMAP_GET_LAYER_COUNT)[1]

    def layout_options(self):
        r = self.send(GET_KEYBOARD_VALUE, KV_LAYOUT_OPTIONS)
        return int.from_bytes(r[2:6], 'big') if r[0] != UNHANDLED else None

    def firmware_version(self):
        """VIA_FIRMWARE_VERSION: our release number (I12); 0 on stock."""
        r = self.send(GET_KEYBOARD_VALUE, KV_FIRMWARE_VERSION)
        return int.from_bytes(r[2:6], 'big') if r[0] != UNHANDLED else None

    def macros(self):
        count = self.send(MACRO_GET_COUNT)
        size = self.send(MACRO_GET_BUFFER_SIZE)
        if UNHANDLED in (count[0], size[0]):
            return None
        return count[1], (size[1] << 8) | size[2]

    def rgblight(self, protocol):
        """Current rgblight state via VIA's lighting values (read-only)."""
        out = {}
        for name, new_id, legacy_id in RGB_VALUES:
            if protocol >= 0x000C:
                r = self.send(CUSTOM_GET_VALUE, RGBLIGHT_CHANNEL, new_id)
                data = r[3:5]
            else:
                r = self.send(CUSTOM_GET_VALUE, legacy_id)
                data = r[2:4]
            if r[0] == UNHANDLED:
                return None
            out[name] = list(data) if name.startswith('color') else data[0]
        return out

    def matrix(self):
        """Raw switch matrix as ROWS 16-bit masks (bit c = column c pressed).

        Reply layout: [0x02, 0x03, ...]. VIA protocol >= 12 echoes the requested
        row offset in byte 2, so row data starts at byte 3; older firmware starts
        at byte 2. Each row is 2 bytes big-endian (MATRIX_COLS 9..16).
        """
        if not hasattr(self, '_matrix_start'):
            self._matrix_start = 3 if self.protocol() >= 0x000C else 2
        r = self.send(GET_KEYBOARD_VALUE, KV_SWITCH_MATRIX_STATE, 0)
        if r[0] == UNHANDLED:
            return None
        st = self._matrix_start
        return [int.from_bytes(r[st + 2 * row:st + 2 + 2 * row], 'big') for row in range(ROWS)]

    def keymap_buffer(self, nbytes):
        out = bytearray()
        while len(out) < nbytes:
            n = min(28, nbytes - len(out))
            r = self.send(DYNAMIC_KEYMAP_GET_BUFFER, len(out) >> 8, len(out) & 0xFF, n)
            out += bytes(r[4:4 + n])
        return bytes(out)

    def read_mem(self, addr, n):
        r = self.send(FALCON_KALAM_READ, (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF, n)
        if r[0] == UNHANDLED:
            raise RuntimeError('firmware does not implement FALCON_KALAM_READ (ship or manufacturer firmware; use the dev image)')
        if r[5] != n:
            raise RuntimeError('address range 0x%08X+%d refused by firmware' % (addr, n))
        return bytes(r[6:6 + n])


def cmd_info(p, a):
    proto = p.protocol()
    print('product:           %s / %s' % (p.dev.manufacturer, p.dev.product))
    print('VIA protocol:      0x%04X' % proto)
    fw = p.firmware_version()
    print('firmware release:  %s' % ('unsupported' if fw is None else fw))
    print('dynamic layers:    %d' % p.layers())
    m = p.macros()
    print('macros:            %s' % ('unsupported' if m is None else '%d, buffer %d bytes' % m))
    print('rgblight now:      %s' % (p.rgblight(proto) or 'unsupported'))
    lo = p.layout_options()
    print('layout options:    %s' % ('unsupported' if lo is None else '0x%08X' % lo))
    m = p.matrix()
    print('matrix state cmd:  %s' % ('unsupported' if m is None else 'supported (rows now: %s)' % ' '.join('%04X' % v for v in m)))


def cmd_dump_keymap(p, a):
    layers = p.layers()
    raw = p.keymap_buffer(layers * ROWS * COLS * 2)
    kc = [int.from_bytes(raw[i:i + 2], 'big') for i in range(0, len(raw), 2)]
    doc = {'vid': '0x%04X' % VID, 'pid': '0x%04X' % PID, 'via_protocol': '0x%04X' % p.protocol(),
           'layout_options': p.layout_options(), 'layers': [
               [['0x%04X' % kc[(l * ROWS + r) * COLS + c] for c in range(COLS)] for r in range(ROWS)] for l in range(layers)]}
    Path(a.out).write_text(json.dumps(doc, indent=2) + '\n')
    print('wrote %s (%d layers, sha256 %s)' % (a.out, layers, hashlib.sha256(raw).hexdigest()))


def cmd_matrix(p, a):
    first = p.matrix()
    if first is None:
        sys.exit('firmware does not answer the switch-matrix query')
    print('Press each key once, slowly. Ctrl-C to stop.')
    print('(If nothing ever appears, the firmware gates matrix reads -- see docs/problems.md L1.)')
    seen, prev = {}, first
    log = open(a.log, 'w') if a.log else None
    if log:
        log.write('time,row,col,event\n')
    try:
        while True:
            cur = p.matrix()
            for r in range(ROWS):
                for c in range(COLS):
                    bit = 1 << c
                    if (cur[r] & bit) != (prev[r] & bit):
                        ev = 'down' if cur[r] & bit else 'up'
                        if ev == 'down':
                            seen[(r, c)] = seen.get((r, c), 0) + 1
                            print('  %d,%d  (pressed %dx, %d distinct positions)' % (r, c, seen[(r, c)], len(seen)))
                        if log:
                            log.write('%.3f,%d,%d,%s\n' % (time.time(), r, c, ev))
                            log.flush()
            prev = cur
            time.sleep(0.01)
    except KeyboardInterrupt:
        print('\n%d distinct positions: %s' % (len(seen), ' '.join('%d,%d' % k for k in sorted(seen))))


def legacy_to_actual(r, c):
    """Map a position logged by the pre-fix decoder (start byte 2 on a protocol-12
    firmware) to the real matrix position. That decoder read every row one byte
    early: its row r = (real row r-1 low byte << 8) | real row r high byte, so
    its columns 0-7 are real (r, c+8) and its columns 8-14 are real (r-1, c-8).
    Real column 7 and real row 4 columns 0-7 were never visible to it."""
    return (r, c + 8) if c < 8 else (r - 1, c - 8)


def cmd_decode_log(_, a):
    seen = []
    for line in Path(a.csv).read_text().splitlines()[1:]:
        _, r, c, ev = line.split(',')
        if ev == 'down':
            seen.append(legacy_to_actual(int(r), int(c)))
    print('%d presses -> real positions in press order:' % len(seen))
    print(' '.join('%d,%d' % p for p in seen))
    print('distinct: %s' % ' '.join('%d,%d' % p for p in sorted(set(seen))))


def cmd_dump_bootloader(p, a):
    out = Path(a.outdir)
    out.mkdir(parents=True, exist_ok=True)
    regions = [('bootloader_0x08000000.bin', 0x08000000, 0x2000),
               ('fsize_uid_0x1FFFF7E0.bin', 0x1FFFF7E0, 20),  # F_SIZE, SRAM density, 96-bit UID (release >= 13; earlier dev images allow 16)
               ('option_bytes_0x1FFFF800.bin', 0x1FFFF800, 16)]
    for name, base, size in regions:
        data = b''.join(p.read_mem(base + off, min(24, size - off)) for off in range(0, size, 24))
        (out / name).write_bytes(data)
        print('%-32s %5d bytes  sha256 %s' % (name, len(data), hashlib.sha256(data).hexdigest()))
    # Read the bootloader twice; a mismatch means an unreliable transfer.
    again = b''.join(p.read_mem(0x08000000 + off, min(24, 0x2000 - off)) for off in range(0, 0x2000, 24))
    if again != (out / regions[0][0]).read_bytes():
        sys.exit('second bootloader read differs -- do not trust this dump')
    print('second read identical')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('info', help='read-only summary; run it once on the stock firmware for docs/stock-firmware.md')
    s = sub.add_parser('dump-keymap', help='all dynamic-keymap layers to OUT.json')
    s.add_argument('out')
    s = sub.add_parser('matrix', help='live matrix positions (dev or stock only; ship hides the matrix, problems L1)')
    s.add_argument('--log')
    s = sub.add_parser('decode-log', help='re-map a matrix log recorded with the pre-fix decoder (offline)')
    s.add_argument('csv')
    s = sub.add_parser('dump-bootloader', help='bootloader, F_SIZE/UID and option bytes via FLASHREAD (dev image only)')
    s.add_argument('outdir')
    a = ap.parse_args()
    if a.cmd == 'decode-log':
        return cmd_decode_log(None, a)
    p = Probe()
    {'info': cmd_info, 'dump-keymap': cmd_dump_keymap, 'matrix': cmd_matrix, 'dump-bootloader': cmd_dump_bootloader}[a.cmd](p, a)


if __name__ == '__main__':
    main()
