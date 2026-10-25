#!/usr/bin/env python3
"""Re-derive every hardware fact used by the Falcon HS R2 port from the
manufacturer firmware, its DFU readback and the vendor VIA JSON.

Each check states the address it inspects, the bytes it expects, and why.
Addresses were found by disassembling the vendor image (tools/disassemble_vendor.sh);
this script only re-verifies them, so it is deterministic and needs nothing
beyond the Python standard library.

    python3 -I tools/analyze_vendor_firmware.py [--report artifacts/vendor_analysis.txt]

Exit status is non-zero if any check fails.
"""

import argparse
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ORIG = ROOT / 'originals'
APP_BASE = 0x08002000
FLASH_END = 0x08010000

GPIO_PORTS = {0x40010800: 'A', 0x40010C00: 'B', 0x40011000: 'C', 0x40011400: 'D', 0x40011800: 'E'}

# QMK (>= 0.19 numbering) keycodes needed to read the vendor keymap.
HID_NAMES = dict(enumerate([None, None, None, None] + list('ABCDEFGHIJKLMNOPQRSTUVWXYZ') + list('1234567890'), 0))
HID_NAMES.update({
    0x00: 'KC_NO', 0x01: 'KC_TRNS', 0x28: 'ENT', 0x29: 'ESC', 0x2A: 'BSPC', 0x2B: 'TAB', 0x2C: 'SPC', 0x2D: 'MINS', 0x2E: 'EQL',
    0x2F: 'LBRC', 0x30: 'RBRC', 0x31: 'BSLS', 0x33: 'SCLN', 0x34: 'QUOT', 0x35: 'GRV', 0x36: 'COMM', 0x37: 'DOT', 0x38: 'SLSH',
    0x39: 'CAPS', 0xE0: 'LCTL', 0xE1: 'LSFT', 0xE2: 'LALT', 0xE3: 'LGUI', 0xE4: 'RCTL', 0xE5: 'RSFT', 0xE6: 'RALT', 0xE7: 'RGUI',
    0x7C00: 'QK_BOOT', 0x7C16: 'QK_GESC', 0x7820: 'UG_TOGG', 0x7821: 'UG_NEXT',
})
HID_NAMES.update({0x3A + i: 'F%d' % (i + 1) for i in range(12)})


def keycode_name(kc):
    if kc in HID_NAMES and HID_NAMES[kc]:
        return HID_NAMES[kc]
    if 0x5220 <= kc <= 0x523F:
        return 'MO(%d)' % (kc & 0x1F)
    if 0x4000 <= kc <= 0x4FFF:
        return 'LT(%d,%s)' % ((kc >> 8) & 0xF, keycode_name(kc & 0xFF))
    return '0x%04X' % kc


class Checker:
    def __init__(self):
        self.lines = []
        self.failures = 0

    def section(self, title):
        self.lines.append('')
        self.lines.append('## ' + title)

    def info(self, text):
        self.lines.append('    ' + text)

    def check(self, ok, text):
        self.lines.append('  [%s] %s' % ('PASS' if ok else 'FAIL', text))
        if not ok:
            self.failures += 1
        return ok


class Image:
    """Flat view of flash starting at APP_BASE."""

    def __init__(self, data):
        self.data = data

    def bytes(self, addr, n):
        off = addr - APP_BASE
        return self.data[off:off + n]

    def u8(self, addr):
        return self.bytes(addr, 1)[0]

    def u16(self, addr):
        return struct.unpack('<H', self.bytes(addr, 2))[0]

    def u32(self, addr):
        return struct.unpack('<I', self.bytes(addr, 4))[0]

    def thumb(self, addr, hexstr):
        """Compare raw instruction bytes as printed by objdump (halfwords, big-endian text)."""
        want = b''.join(struct.pack('<H', int(h, 16)) for h in hexstr.split())
        return self.bytes(addr, len(want)) == want

    def find(self, needle):
        hits, i = [], self.data.find(needle)
        while i >= 0:
            hits.append(APP_BASE + i)
            i = self.data.find(needle, i + 1)
        return hits


def ioline(v):
    port = GPIO_PORTS.get(v & ~0xF)
    return '%s%d' % (port, v & 0xF) if port else None


def pin_table(img, addr, count):
    return [ioline(img.u32(addr + 4 * i)) for i in range(count)]


def via_positions(via):
    """Matrix positions ("row,col" legends) referenced anywhere in a VIA KLE layout."""
    keys = set()
    for row in via['layouts']['keymap']:
        for k in row:
            if isinstance(k, str) and k:
                keys.add(tuple(int(x) for x in k.split('\n')[0].split(',')))
    return sorted(keys)


def analyse(c):
    raw_bin = (ORIG / 'manufacturer_firmware.bin').read_bytes()
    readback = (ORIG / 'falcon_flash_backup.bin').read_bytes()
    via = json.loads((ORIG / 'manufacturer_jsonfile.json').read_text())

    # ------------------------------------------------------------------
    c.section('1. Artifact integrity')
    sums = {}
    for line in (ORIG / 'SHA256SUMS').read_text().splitlines():
        h, name = line.split(None, 1)
        sums[name.strip()] = h
    for name in sorted(sums):
        actual = hashlib.sha256((ORIG / name).read_bytes()).hexdigest()
        c.check(actual == sums[name], '%s sha256 %s' % (name, actual))

    suffix, app = raw_bin[-16:], raw_bin[:-16]
    bcd_dev, pid, vid, bcd_dfu = struct.unpack('<HHHH', suffix[:8])
    sig, length, crc = suffix[8:11], suffix[11], struct.unpack('<I', suffix[12:])[0]
    c.check(sig == b'UFD' and length == 16, 'manufacturer BIN ends in a 16-byte DFU suffix (signature "UFD")')
    c.check((vid, pid) == (0x1EAF, 0x0003), 'DFU suffix targets VID:PID %04X:%04X (STM32duino/Maple bootloader)' % (vid, pid))
    c.check(crc == (~zlib.crc32(raw_bin[:-4]) & 0xFFFFFFFF), 'DFU suffix CRC32 0x%08X is valid' % crc)
    c.info('suffix: bcdDevice=0x%04X bcdDFU=0x%04X' % (bcd_dev, bcd_dfu))
    c.check(len(app) == 43940, 'application payload is %d bytes' % len(app))
    c.check(readback[:len(app)] == app, 'payload equals the first %d bytes of the DFU readback' % len(app))
    c.check(len(readback) == FLASH_END - APP_BASE,
            'readback covers 0x%08X-0x%08X (%d bytes) => 64 KB part, 8 KB bootloader' % (APP_BASE, FLASH_END - 1, len(readback)))

    img = Image(readback)

    # ------------------------------------------------------------------
    c.section('2. Vector table / runtime')
    sp, reset = img.u32(APP_BASE), img.u32(APP_BASE + 4)
    c.check(0x20000000 <= sp <= 0x20005000, 'initial SP 0x%08X is inside 20 KB SRAM' % sp)
    c.check(reset & 1 and APP_BASE <= reset < APP_BASE + len(app), 'reset vector 0x%08X is Thumb code inside the image' % reset)
    default = img.u32(APP_BASE + 0x10)
    svc, pendsv, systick = img.u32(APP_BASE + 0x2C), img.u32(APP_BASE + 0x38), img.u32(APP_BASE + 0x3C)
    c.check(svc != default and systick != default and pendsv == default,
            'SVCall 0x%08X and SysTick 0x%08X have handlers, PendSV is the default handler (ChibiOS RT ARMv7-M port, periodic tick)'
            % (svc, systick))

    # ------------------------------------------------------------------
    c.section('3. USB descriptors')
    dev = bytes.fromhex('1201000200000040d19d899d0100010200 01'.replace(' ', ''))
    hits = img.find(dev)
    c.check(len(hits) == 1, 'device descriptor (USB 2.0, EP0=64, 9DD1:9D89, bcdDevice 0x0001, iSerial 0) at %s' % [hex(h) for h in hits])
    for s in ('Falcon',):
        enc = bytes([2 + 2 * len(s), 3]) + s.encode('utf-16le')
        found = img.find(enc)
        c.check(len(found) == 2, 'string descriptor "%s" used for manufacturer and product at %s' % (s, [hex(h) for h in found]))
    c.check(bool(img.find(bytes.fromhex('0660ff0961a1010962'))), 'raw HID report (usage page 0xFF60, usage 0x61) present => VIA protocol')
    c.check(bool(img.find(bytes.fromhex('050109068501'.replace('8501', 'a101')))), 'boot keyboard report present')
    nkro = bytes.fromhex('050109 06a1 0185 06 0507 19e0 29e7'.replace(' ', ''))
    c.check(bool(img.find(nkro)), 'shared-EP NKRO report (report ID 6, bitmap) present => vendor firmware HAS NKRO compiled in')
    for rid, what in ((2, 'mouse'), (3, 'system control'), (4, 'consumer')):
        c.check(bool(img.find(bytes([0x85, rid]))), 'report ID %d (%s) present in shared report descriptor' % (rid, what))

    # ------------------------------------------------------------------
    c.section('4. Matrix pins and diode direction')
    rows = pin_table(img, 0x0800C464, 5)
    cols = pin_table(img, 0x0800C10C, 15)
    c.check(rows == ['B3', 'B4', 'B5', 'B6', 'A2'], 'row_pins @0x0800C464 (ChibiOS ioline_t = port|pad): %s' % ' '.join(rows))
    c.check(cols == ['A9', 'A3', 'A4', 'A5', 'A6', 'A7', 'B0', 'B1', 'B10', 'B11', 'B12', 'B13', 'B14', 'B15', 'A8'],
            'col_pins @0x0800C10C: %s' % ' '.join(cols))
    c.check(img.u32(0x08006228) == 0x0800C464 and img.u32(0x08006230) == 0x0800C10C,
            'matrix scan @0x08006100 iterates row_pins in the outer loop and col_pins in the inner loop')
    c.check(img.thumb(0x08006138, '2206') and img.thumb(0x0800613E, '6145'),
            'select: palSetLineMode(row, 6=OUTPUT_PUSHPULL) then GPIO->BRR (offset 0x14) => row driven LOW')
    c.check(img.thumb(0x0800617E, 'f8dc c008') and img.thumb(0x0800618A, 'f011 0f01') and img.thumb(0x0800618E, 'bf0c'),
            'read: GPIO->IDR (offset 0x08) bit tested, key = bit==0 => columns are active-low inputs')
    c.check(img.thumb(0x080056B4, '2203'), 'unselect @0x080056A4: palSetLineMode(row, 3=INPUT_PULLUP)')
    c.info('=> rows are strobed, columns sensed: QMK DIODE_DIRECTION COL2ROW')

    # ------------------------------------------------------------------
    c.section('5. Clock tree (stm32_clock_init @0x080090AC)')
    c.check(img.thumb(0x080090D0, '681a f442 3280'), 'RCC_CR |= HSEON (0x10000), waits HSERDY => external crystal used')
    c.check(img.thumb(0x080090E0, 'f442 12e8'), 'RCC_CFGR |= 0x001D0000 => PLLSRC=HSE, PLLMUL=x9')
    c.check(img.u32(0x08009118) == 0x001D6400,
            'RCC_CFGR = 0x001D6400 => HPRE/1, PPRE1/2, PPRE2/2, ADCPRE/4, USBPRE=/1.5')
    c.check(img.thumb(0x080090F4, '2112') and img.u32(0x0800911C) == 0x40022000, 'FLASH_ACR = 0x12 => 2 wait states + prefetch (72 MHz)')
    c.check(img.thumb(0x08009100, 'f042 0202'), 'SW = PLL')
    nops = sum(1 for a in range(0x08006146, 0x0800616A, 2) if img.u16(a) == 0xBF00)
    c.check(nops == 18, 'matrix input delay is %d NOPs = GPIO_INPUT_PIN_DELAY (CPU_CLOCK/1MHz/4) => CPU_CLOCK 72 MHz' % nops)
    c.check(img.thumb(0x080061B6, 'f5b3 6f07'), 'post-row wait is 0x870 = 2160 DWT cycles = 30 us (MATRIX_IO_DELAY) @ 72 MHz')
    c.info('=> SYSCLK 72 MHz = HSE x 9 => HSE = 8 MHz; USB = 72/1.5 = 48 MHz')

    # ------------------------------------------------------------------
    c.section('6. Board init, JTAG/SWD, bootloader flags')
    c.check(img.u32(0x08009624) == 0x40010000 and img.thumb(0x080094AA, 'f043 7300'),
            'AFIO_MAPR |= 0x02000000 (SWJ_CFG=010): JTAG-DP off, SW-DP on => PB3/PB4/PA15 usable as GPIO')
    c.check(img.thumb(0x080094B0, 'f244 224c') and img.u32(0x0800962C) == 0x40006C00,
            'boardInit writes BKP_DR10 = 0x424C (RTC_BOOTLOADER_FLAG)')
    c.check(img.thumb(0x08007876, 'f244 224d') and img.u32(0x08007884) == 0x40006C00,
            'mcu_reset writes BKP_DR10 = 0x424D (RTC_BOOTLOADER_JUST_UPLOADED) before NVIC reset')
    # Bootmagic (QMK bootmagic_scan): scan, wait, scan; test matrix row 0 bit 0;
    # eeconfig_disable() (format + magic 0xFFFF at addr 0); bootloader_jump().
    c.check(img.thumb(0x08009B40, 'f7fc fade') and img.thumb(0x08009B4C, 'f7fc fad8')
            and img.u32(0x08009BCC) == img.u32(0x08006240) == 0x200017A6
            and img.thumb(0x08009B52, '881b 07dc') and img.thumb(0x08009B5C, 'f64f 71ff 2000')
            and img.thumb(0x08009B66, 'f7fb f9f9'),
            'Bootmagic @0x08009B40: matrix_scan x2, row 0 (debounced matrix 0x200017A6) bit 0 -> eeconfig_disable -> bootloader_jump; key = 0,0 = Esc')
    expected_cr = {0x08009608: 0x888884B8}
    c.check(all(img.u32(a) == v for a, v in expected_cr.items()), 'GPIOA CRH reset value 0x888884B8')
    c.check(img.u32(0x0800960C) == 0x88388888 and img.u32(0x08009610) == 0x88888844,
            'GPIOC CRH 0x88388888, GPIOD CRL 0x88888844 (PD0/PD1 = OSC_IN/OUT)')
    c.info('these equal VAL_GPIOxCRx in QMK platforms/chibios/boards/STM32_F103_STM32DUINO/board/board.h')

    # ------------------------------------------------------------------
    c.section('7. Flash layout and EEPROM (wear-leveling on embedded flash)')
    desc = struct.unpack('<IIIIIII', img.bytes(0x0800C148, 28))
    c.check(desc == (3, 2, 128, 0, 1024, 0x08000000, 0x20000),
            'ChibiOS EFL descriptor @0x0800C148: attrs=%d page=%d sectors=%d sector_size=%d base=0x%08X size=0x%X (STM32F103xB registry)'
            % (desc[0], desc[1], desc[2], desc[4], desc[5], desc[6]))
    c.check(img.u32(0x08009664) == 0x1FFFF000 and img.thumb(0x080095C8, 'f8d3 27e0'),
            'backing_store_init reads F_SIZE @0x1FFFF7E0 to place storage at the end of flash')
    c.check(img.u32(0x08009668) == 0x03FFFC00, 'F_SIZE masked as (x & 0xFFFF) << 10 (QMK detect_flash_size)')
    end_app = APP_BASE + len(app)
    gap = img.bytes(end_app, 0x0800F800 - end_app)
    c.check(gap == b'\xff' * len(gap), '0x%08X-0x0800F7FF erased (unused application flash)' % end_app)
    wl0 = img.bytes(0x0800F800, 1024)
    wl1 = img.bytes(0x0800FC00, 1024)
    c.check(wl0 == b'\xff' * 1024, '0x0800F800-0x0800FBFF (WL base image, 1 KB) erased')
    c.check(wl1 != b'\xff' * 1024, '0x0800FC00-0x0800FFFF (WL write log) holds data')
    used = [i for i, b in enumerate(wl1) if b != 0xFF]
    c.info('write log non-FF bytes at offsets %s: %s' % (used, wl1[:16].hex(' ')))
    c.info('=> F_SIZE reads 64 KB on this unit; EEPROM = 0x0800F800-0x0800FFFF (2 KB backing, 1 KB logical)')
    # Decode the write log (QMK quantum/wear_leveling): flash stores ~value
    # (FLASH_ATTR_ERASED_IS_ONE), entries are 16-bit; type = top 2 bits of byte 0.
    entries = []
    for off in range(8, 1024, 2):
        raw = struct.unpack_from('<H', wl1, off)[0]
        if raw == 0xFFFF:
            continue
        b0, b1 = (~raw) & 0xFF, ((~raw) >> 8) & 0xFF
        entries.append((b0 >> 6, b0 & 0x3F, b1))
    c.check(entries == [(1, 0, 0xFF), (1, 1, 0xFF)] and wl1[:8] == b'\xff' * 8,
            'write log = fresh erase + OPTIMIZED_64 writes addr0=0xFF, addr1=0xFF => EECONFIG magic 0xFFFF')
    c.info('=> exactly nvm_eeconfig_disable(): eeprom_driver_format() then magic=0xFFFF, i.e. Bootmagic/Clear-EEPROM ran;')
    c.info('   the backup holds NO user configuration (any earlier VIA keymap was erased before the readback)')

    # ------------------------------------------------------------------
    c.section('8. RGB (WS2812 bit-bang, rgblight)')
    c.check(img.thumb(0x080056C8, '2206 2102') and img.u32(0x080056D4) == 0x40010800,
            'ws2812_init: palSetGroupMode(GPIOA, mask 0x0002, 6=OUTPUT_PUSHPULL) => data pin PA1')
    c.check(img.thumb(0x08004AEE, '2102') and img.thumb(0x08004AF8, '6111') and img.thumb(0x08004B0E, '6151'),
            'ws2812 bit writer toggles GPIOA BSRR/BRR with mask 0x0002 (PA1), MSB first')
    c.check(img.thumb(0x08004B78, 'f44f 63a8'), 'reset latch loop 1344 x 15 cycles = 20160 cycles = 280 us @ 72 MHz (WS2812_TRST_US default)')
    c.check(img.thumb(0x08004776, '2104') and img.thumb(0x08004768, '2b04'), 'rgblight_set -> ws2812_setleds(led, 4) => RGBLIGHT_LED_COUNT 4')
    c.check(img.thumb(0x080049F6, '7058 7019 709a'),
            'rgblight_setrgb stores r->[1], g->[0], b->[2] => rgb_led_t is {g,r,b} => WS2812_BYTE_ORDER GRB')

    modes = list(img.bytes(0x0800C2B6, 43))
    qmk_all = [0, 1] + [2] * 4 + [6] * 3 + [9] * 6 + [15] * 6 + [21] * 3 + [24] + [25] * 10 + [35, 36] + [37] * 6
    c.check(modes == qmk_all and img.u8(0x0800C2B6 + 43) == 0,
            'rgblight mode_base_table @0x0800C2B6 has all 43 modes: static, breathing x4, rainbow mood x3, swirl x6, '
            'snake x6, knight x3, christmas, gradient x10, RGB test, alternating, twinkle x6 => every effect enabled')

    # ------------------------------------------------------------------
    c.section('8b. Vendor build identity (QMK Command/console strings)')
    for addr, text in ((0x0800B746, b'BUILD:  (Mar 14 2025)'), (0x0800B75C, b'CHIBIOS: "be44b3", CONTRIB: "develop_2021_q3-455-g77cb0a"'),
                       (0x0800B796, b'OPTIONS: BOOTMAGIC MOUSEKEY EXTRAKEY CONSOLE COMMAND NKRO LTO BOOTLOADER_SIZE'),
                       (0x0800B7E4, b'GCC: 10.1.0'), (0x0800B70E, b'VID: 0x9DD1("Falcon") PID: 0x9D89("Falcon") VER: 0x0001')):
        c.check(img.bytes(addr, len(text)) == text, '@0x%08X "%s"' % (addr, text.decode()))
    c.check(bool(img.find(b'NKRO Toggle')) and bool(img.find(b'Jump to Bootloader')),
            'Command feature present (magic key combo; default LShift+RShift) incl. "NKRO Toggle" and "Jump to Bootloader"')
    c.info('=> vendor = QMK (no version string; keycodes >= 0.19) on ChibiOS be44b3, built 2025-03-14 with GCC 10.1.0, NKRO compiled in')

    # ------------------------------------------------------------------
    c.section('9. Vendor default keymap (2 layers @0x0800C18A)')
    for layer in range(2):
        base = 0x0800C18A + layer * 150
        c.info('layer %d:' % layer)
        for r in range(5):
            kcs = struct.unpack('<15H', img.bytes(base + r * 30, 30))
            c.info('  r%d ' % r + ' '.join('%-9s' % keycode_name(k) for k in kcs))
    c.check(img.u16(0x0800C18A) == 0x7C16, 'keycode values use QMK >= 0.19 numbering (QK_GESC = 0x7C16, QK_BOOT = 0x7C00, UG_TOGG = 0x7820)')

    # ------------------------------------------------------------------
    c.section('10. VIA JSON')
    c.check(via['vendorId'].lower() == '0x9dd1' and via['productId'].lower() == '0x9d89', 'VID/PID match the device descriptor')
    c.check(via['matrix'] == {'rows': 5, 'cols': 15}, 'matrix 5 x 15')
    keys = set(via_positions(via))
    c.info('labels: %s; menus: %s' % (via['layouts']['labels'], via.get('menus')))
    c.info('%d matrix positions referenced: %s' % (len(keys), ' '.join('%d,%d' % k for k in sorted(keys))))
    return {'rows': rows, 'cols': cols, 'via_positions': sorted(keys)}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--report', type=Path, help='also write the report to this file')
    args = ap.parse_args()
    c = Checker()
    c.lines.append('# Falcon HS R2 vendor firmware analysis')
    analyse(c)
    c.lines.append('')
    c.lines.append('RESULT: %s (%d failure%s)' % ('PASS' if c.failures == 0 else 'FAIL', c.failures, '' if c.failures == 1 else 's'))
    text = '\n'.join(c.lines) + '\n'
    sys.stdout.write(text)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(text)
    return 1 if c.failures else 0


if __name__ == '__main__':
    sys.exit(main())
