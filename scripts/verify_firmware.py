#!/usr/bin/env python3
"""Offline verification of a built Falcon HS R2 image. Flashes nothing.

Checks that the image fits between the bootloader and the EEPROM pages, that
the binary is exactly the ELF's load image, that NKRO is absent and VIA is
present, and that the compiled configuration matches what was recovered from
the manufacturer firmware (tools/analyze_vendor_firmware.py).

Requires arm-none-eabi-readelf / -nm / -objcopy on PATH (scripts/build.sh sets it).
"""

import argparse
import json
import re
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import analyze_vendor_firmware as vendor  # noqa: E402

FLASH_BASE = 0x08000000
APP_START = 0x08002000      # end of the 8 KB STM32duino bootloader
EEPROM_START = 0x0800F800   # 2 KB wear-leveling backing store
FLASH_END = 0x08010000      # 64 KB part
RAM_START, RAM_END = 0x20000000, 0x20005000  # 20 KB

PORT_BASE = {'A': 0x40010800, 'B': 0x40010C00, 'C': 0x40011000, 'D': 0x40011400}


def run(*cmd):
    return subprocess.run(cmd, check=True, capture_output=True, text=True).stdout


def thumb2_find(payload, mask1, val1, mask2, val2):
    """True if a 32-bit Thumb-2 instruction (two halfwords) matches the masks anywhere.

    Used for stock-equivalence checks so they do not depend on which register the
    compiler picked (the encodings differ only in register fields)."""
    for off in range(0, len(payload) - 3, 2):
        hw1, hw2 = struct.unpack_from('<HH', payload, off)
        if hw1 & mask1 == val1 and hw2 & mask2 == val2:
            return True
    return False


def movw_find(payload, imm16):
    """MOVW Rd, #imm16 (encoding T3) with any destination register."""
    imm4, i, imm3, imm8 = imm16 >> 12, (imm16 >> 11) & 1, (imm16 >> 8) & 7, imm16 & 0xFF
    return thumb2_find(payload, 0xFFFF, 0xF240 | (i << 10) | imm4, 0x70FF, (imm3 << 12) | imm8)


def ioline_bytes(pins):
    return b''.join(struct.pack('<I', PORT_BASE[p[0]] | int(p[1:])) for p in pins)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bin', type=Path, required=True)
    ap.add_argument('--elf', type=Path, required=True)
    ap.add_argument('--profile', choices=('base', 'release', 'dev'), required=True,
                    help='base: L1 normal firmware (NKRO, no VIA); release: ship variant (L2, no debug-only features); '
                         'dev: dev variant (L2 + every debug feature) -- POLICY-VARIANTS')
    ap.add_argument('--via-json', type=Path, help='VIA definition shipped with the L2 variants')
    ap.add_argument('--keyboard-json', type=Path, required=True)
    ap.add_argument('--cflags', type=Path, help='cflags.txt from the build (default: next to --bin)')
    ap.add_argument('--report', type=Path)
    args = ap.parse_args()

    c = vendor.Checker()
    c.lines.append('# Falcon HS R2 firmware verification: %s (profile: %s)' % (args.bin.name, args.profile))
    release = args.profile in ('release', 'dev')   # built from the L2 configuration
    ship, dev = args.profile == 'release', args.profile == 'dev'
    keymap_dir = {'release': 'falcon', 'dev': 'falcon_dev'}.get(args.profile)
    kb = json.loads(args.keyboard_json.read_text())
    raw = args.bin.read_bytes()

    # ------------------------------------------------------------------
    c.section('1. Binary container')
    if raw[-16:][8:11] == b'UFD':
        suffix, payload = raw[-16:], raw[:-16]
        _, pid, vid, _ = struct.unpack('<HHHH', suffix[:8])
        crc = struct.unpack('<I', suffix[12:])[0]
        c.check((vid, pid) == (0x1EAF, 0x0003), 'DFU suffix targets %04X:%04X (STM32duino/Maple)' % (vid, pid))
        c.check(crc == (~zlib.crc32(raw[:-4]) & 0xFFFFFFFF), 'DFU suffix CRC32 0x%08X valid' % crc)
    else:
        payload = raw
        c.info('no DFU suffix')
    c.info('file %d bytes, payload %d bytes' % (len(raw), len(payload)))

    with tempfile.TemporaryDirectory() as td:
        ref = Path(td) / 'ref.bin'
        run('arm-none-eabi-objcopy', '-O', 'binary', str(args.elf), str(ref))
        c.check(ref.read_bytes() == payload, 'payload is byte-identical to objcopy -O binary of the ELF')

    # ------------------------------------------------------------------
    c.section('2. Flash and RAM placement')
    syms = {}
    for line in run('arm-none-eabi-nm', str(args.elf)).splitlines():
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16)
    c.check(syms.get('__falcon_app_start__') == APP_START, 'linker script app start 0x%08X' % syms.get('__falcon_app_start__', 0))
    c.check(syms.get('__falcon_app_limit__') == EEPROM_START, 'linker script app limit 0x%08X == EEPROM start' % syms.get('__falcon_app_limit__', 0))

    segs = []
    for line in run('arm-none-eabi-readelf', '-lW', str(args.elf)).splitlines():
        m = re.match(r'\s*LOAD\s+(0x\w+)\s+(0x\w+)\s+(0x\w+)\s+(0x\w+)\s+(0x\w+)', line)
        if m:
            _, vaddr, paddr, filesz, memsz = (int(x, 16) for x in m.groups())
            segs.append((vaddr, paddr, filesz, memsz))
    flash_hi = APP_START
    for vaddr, paddr, filesz, memsz in segs:
        if filesz:
            ok = APP_START <= paddr and paddr + filesz <= EEPROM_START
            c.check(ok, 'LOAD lma 0x%08X-0x%08X inside application flash' % (paddr, paddr + filesz))
            flash_hi = max(flash_hi, paddr + filesz)
        if RAM_START <= vaddr < RAM_END:
            c.check(vaddr + memsz <= RAM_END, 'RAM segment 0x%08X-0x%08X inside 20 KB SRAM' % (vaddr, vaddr + memsz))
    c.check(APP_START + len(payload) <= EEPROM_START, 'image ends at 0x%08X (< 0x%08X)' % (APP_START + len(payload), EEPROM_START))
    c.check(APP_START + len(payload) == flash_hi, 'binary length agrees with ELF load segments')
    cap = EEPROM_START - APP_START
    c.info('application flash used %d / %d bytes (%.1f%%), %d bytes free' % (len(payload), cap, 100.0 * len(payload) / cap, cap - len(payload)))
    c.info('bootloader 0x%08X-0x%08X and EEPROM 0x%08X-0x%08X are outside every LOAD segment' % (FLASH_BASE, APP_START - 1, EEPROM_START, FLASH_END - 1))
    ram_hi = max((v + m for v, _, _, m in segs if RAM_START <= v < RAM_END), default=RAM_START)
    c.info('static RAM (data+bss+stacks+heap reservation) up to 0x%08X' % ram_hi)

    # ------------------------------------------------------------------
    c.section('3. Vector table')
    sp, reset = struct.unpack('<II', payload[:8])
    c.check(RAM_START < sp <= RAM_END, 'initial SP 0x%08X in SRAM' % sp)
    c.check(sp == syms.get('__main_stack_end__'), 'initial SP equals __main_stack_end__')
    c.check(reset & 1 and APP_START <= reset < APP_START + len(payload), 'reset vector 0x%08X is Thumb code in the image' % reset)
    c.check(reset & ~1 == syms.get('Reset_Handler', -1) & ~1, 'reset vector is Reset_Handler')
    n_vec = 16 + 60  # Cortex-M3 system exceptions + STM32F10x medium-density IRQs
    bad = [i for i, v in enumerate(struct.unpack('<%dI' % (n_vec - 2), payload[8:n_vec * 4]), 2)
           if v and not (APP_START <= v < APP_START + len(payload))]
    c.check(not bad, 'all populated entries of the %d-entry vector table point into the image%s' % (n_vec, ' (bad: %s)' % bad if bad else ''))

    # ------------------------------------------------------------------
    c.section('4. USB / HID')
    img = vendor.Image(payload)
    vid, pid = int(kb['usb']['vid'], 16), int(kb['usb']['pid'], 16)
    devdesc = struct.pack('<BBHBBBBHHH', 18, 1, 0x0200, 0, 0, 0, 64, vid, pid, 0x0001)
    c.check(len(img.find(devdesc)) == 1, 'device descriptor USB 2.0 / EP0 64 / %04X:%04X / bcdDevice 0x0001 (same as vendor)' % (vid, pid))
    via_hid = bool(img.find(bytes.fromhex('0660ff0961')))
    c.check(via_hid == release, 'raw HID usage page 0xFF60 / usage 0x61 (VIA) %s' % ('present' if release else 'absent'))
    nkro_bitmap = re.compile(rb'\x05\x07\x19\x00\x29[\x00-\xff]\x15\x00\x25\x01')
    sizes = {}
    for line in run('arm-none-eabi-nm', '-S', str(args.elf)).splitlines():
        parts = line.split()
        if len(parts) == 4:
            sizes[parts[3]] = (int(parts[0], 16), int(parts[1], 16))
    nk_syms = sorted(s for s in syms if 'nkro' in s.lower())
    cflags_path = args.cflags or args.bin.with_name(args.bin.stem + '.cflags.txt')
    flags = cflags_path.read_text().split() if cflags_path.exists() else []
    c.check(bool(flags), 'compiler flags read from %s' % cflags_path.name)
    if release:
        # POLICY-NKRO. The ChibiOS host driver always defines send_nkro(); without
        # NKRO_ENABLE its body is compiled out. Accept it only as a bare `bx lr`.
        c.check(not nkro_bitmap.search(payload), 'POLICY-NKRO: no keyboard bitmap (NKRO) report in any HID descriptor')
        stub = sizes.get('send_nkro')
        stub_ok = stub is not None and stub[1] == 2 and img.u16(stub[0] & ~1) == 0x4770
        c.check(nk_syms == [] or (nk_syms == ['send_nkro'] and stub_ok),
                'POLICY-NKRO: no NKRO code (at most the empty send_nkro stub, 2 bytes, bx lr): %s' % nk_syms)
        c.check('-DNKRO_ENABLE' not in flags, 'POLICY-NKRO: -DNKRO_ENABLE absent from compiler flags')
        c.check(any('userspace/keyboards/falcon_hs_r2/keymaps/%s/keymap.c' % keymap_dir in f for f in flags),
                'keymap comes from the userspace variant keymaps/%s' % keymap_dir)
        want = ('-DVIA_ENABLE', '-DRAW_ENABLE', '-DDYNAMIC_KEYMAP_ENABLE',
                '-DCONSOLE_ENABLE', '-DCOMMAND_ENABLE', '-DLTO_ENABLE')  # POLICY-PARITY
        c.check(bool(img.find(bytes.fromhex('0631ff0974'))), 'POLICY-PARITY: HID console interface (0xFF31) present, as in stock')
        # POLICY-VARIANTS: debug-only features are present in dev and absent in ship.
        flashread = any(n.split('.')[0] == 'via_command_kb' for n in syms)
        debug_flags = ('-DFALCON_FLASHREAD_ENABLE', '-DKEYCODE_STRING_ENABLE', '-DDEBUG_MATRIX_SCAN_RATE')
        traces = {'ACTION_DEBUG': b'action_exec: start', 'WEAR_LEVELING_DEBUG_OUTPUT': b'Backing store: ',
                  'DEBUG_MATRIX_SCAN_RATE': b'matrix scan frequency'}
        # POLICY-RGB: status-indicator module in both variants; effects and dev RGB tools only in dev.
        all_modes = bytes([0, 1] + [2] * 4 + [6] * 3 + [9] * 6 + [15] * 6 + [21] * 3 + [24] + [25] * 10 + [35, 36] + [37] * 6)
        c.check(any(n.split('.')[0] == 'falcon_rgb_layers' for n in syms), 'POLICY-RGB: indicator layer table (falcon_rgb_layers) linked')
        c.check((all_modes in payload) == dev, 'POLICY-RGB: decorative effect table (all 43 rgblight modes) %s' % ('present (dev)' if dev else 'absent (ship)'))
        c.check((b'RGB identify' in payload) == dev, 'POLICY-RGB: LED identify tool (Shift+Shift+L) %s' % ('present (dev)' if dev else 'absent (ship)'))
        if args.via_json:
            menus = json.loads(args.via_json.read_text()).get('menus', [])
            c.check(('qmk_rgblight' in menus) == dev, 'POLICY-RGB: %s lighting menu %s' % (args.via_json.name, 'present (dev)' if dev else 'absent (ship)'))
        if dev:
            c.check(flashread, 'POLICY-VARIANTS dev: FLASHREAD handler (via_command_kb) linked')
            for f in debug_flags:
                c.check(f in flags, 'POLICY-VARIANTS dev: %s present' % f)
            for name, needle in traces.items():
                c.check(needle in payload, 'POLICY-VARIANTS dev: %s trace strings present' % name)
        else:
            c.check(not flashread, 'POLICY-VARIANTS ship: no FLASHREAD handler (via_command_kb absent)')
            for f in debug_flags:
                c.check(f not in flags, 'POLICY-VARIANTS ship: %s absent' % f)
            for name, needle in traces.items():
                c.check(needle not in payload, 'POLICY-VARIANTS ship: no %s trace strings' % name)
    else:
        c.check(bool(nkro_bitmap.search(payload)), 'base is a normal QMK build: NKRO bitmap report present')
        c.check('-DNKRO_ENABLE' in flags and '-DVIA_ENABLE' not in flags, 'base flags: NKRO_ENABLE on, VIA_ENABLE off')
        want = ()
    for w in want + ('-DEEPROM_WEAR_LEVELING', '-DRGBLIGHT_ENABLE', '-DRGBLIGHT_WS2812',
                     '-DBOOTLOADER_STM32DUINO', '-DQMK_BOARD_STM32_F103_STM32DUINO'):
        c.check(w in flags, '%s present' % w)

    # Reproducibility: any compiler build date must come from SOURCE_DATE_EPOCH.
    m = re.search(rb'BUILD: [^(]*\(([A-Z][a-z]{2} [ 0-9]\d \d{4})\)', payload)
    if m:
        import os, time
        epoch = os.environ.get('SOURCE_DATE_EPOCH')
        want = time.strftime('%b %e %Y', time.gmtime(int(epoch))) if epoch else None
        c.check(want is not None and m.group(1).decode() == want,
                'embedded build date "%s" == SOURCE_DATE_EPOCH date "%s" (day-independent build)' % (m.group(1).decode(), want))

    # ------------------------------------------------------------------
    c.section('5. Configuration matches the manufacturer firmware')
    vimg = vendor.Image((vendor.ORIG / 'falcon_flash_backup.bin').read_bytes())
    vrows = vendor.pin_table(vimg, 0x0800C464, 5)
    vcols = vendor.pin_table(vimg, 0x0800C10C, 15)
    c.check(kb['matrix_pins']['rows'] == vrows, 'keyboard.json rows == vendor row_pins %s' % ' '.join(vrows))
    c.check(kb['matrix_pins']['cols'] == vcols, 'keyboard.json cols == vendor col_pins %s' % ' '.join(vcols))
    c.check(bool(img.find(ioline_bytes(vrows))), 'compiled image contains row_pins table identical to vendor')
    c.check(bool(img.find(ioline_bytes(vcols))), 'compiled image contains col_pins table identical to vendor')
    c.check(kb.get('diode_direction') == 'COL2ROW', 'diode_direction COL2ROW')
    efl = struct.pack('<IIIIIII', 3, 2, 128, 0, 1024, 0x08000000, 0x20000)
    c.check(bool(img.find(efl)), 'ChibiOS EFL descriptor identical to vendor (128 x 1 KB sectors)')
    if not dev:
        # Not for dev: its FLASHREAD legitimately reads 0x1FFFF7E0; all variants share L1 config.h.
        c.check(not img.find(struct.pack('<I', 0x1FFFF000)) and not img.find(struct.pack('<I', 0x1FFFF7E0)),
                'no F_SIZE register access (WEAR_LEVELING_EFL_FLASH_SIZE pinned to 64 KB)')
    c.check(bool(img.find(struct.pack('<I', 0x001D6400))), 'RCC_CFGR value 0x001D6400 (PLL x9, USB /1.5, APB /2) identical to vendor')
    # ORR.W Rd, Rn, #0x001D0000 (any registers): PLLSRC=HSE, PLLMUL x9
    c.check(thumb2_find(payload, 0xFFF0, 0xF440, 0xF0FF, 0x10E8), 'PLLSRC=HSE / PLLMUL x9 instruction (orr #0x1D0000, any register) present')
    c.check(movw_find(payload, 0x424C) and movw_find(payload, 0x424D),
            'BKP_DR10 bootloader flags 0x424C / 0x424D present (movw, any register; STM32duino board)')
    # ORR.W Rd, Rn, #0x02000000 (T1 modified immediate, any registers): AFIO_MAPR |= SWJ_CFG_JTAGDISABLE
    c.check(thumb2_find(payload, 0xFFF0, 0xF040, 0xF0FF, 0x7000), 'AFIO_MAPR |= SWJ_CFG_JTAGDISABLE present (orr #0x02000000, any register)')
    wl = kb['eeprom']['wear_leveling']
    c.check((wl['backing_size'], wl['logical_size']) == (2048, 1024) and EEPROM_START + wl['backing_size'] == FLASH_END,
            'wear-leveling 2048/1024 at 0x%08X-0x%08X matches vendor readback' % (EEPROM_START, FLASH_END - 1))
    via_pos = set(tuple(p) for p in vendor.via_positions(json.loads((vendor.ORIG / 'manufacturer_jsonfile.json').read_text())))
    all_pos = set(tuple(k['matrix']) for k in kb['layouts']['LAYOUT_all']['layout'])
    hhkb_pos = set(tuple(k['matrix']) for k in kb['layouts']['LAYOUT_60_hhkb']['layout'])
    c.check(all_pos == via_pos, 'LAYOUT_all covers exactly the %d matrix positions of the vendor VIA JSON' % len(via_pos))
    c.check(hhkb_pos <= via_pos and len(hhkb_pos) == 60, 'LAYOUT_60_hhkb is a 60-key subset of the VIA positions')
    c.check(list(kb['layouts'])[0] == 'LAYOUT_60_hhkb', 'LAYOUT_60_hhkb is the first (default) layout')
    if args.via_json:
        vj = json.loads(args.via_json.read_text())
        default_view = set()
        for row in vj['layouts']['keymap']:
            for k in row:
                if isinstance(k, str) and k:
                    legend = k.split('\n')
                    opt = legend[3] if len(legend) > 3 else ''
                    if not opt or opt.split(',')[1] == '0':
                        default_view.add(tuple(int(x) for x in legend[0].split(',')))
        c.check(default_view == hhkb_pos, '%s: VIA default view (all layout options = 0) is exactly LAYOUT_60_hhkb' % args.via_json.name)
        c.check(set(tuple(p) for p in vendor.via_positions(vj)) == via_pos, '%s: same matrix positions as the vendor VIA JSON' % args.via_json.name)
    info_cfg = args.bin.with_name(args.bin.stem + '.info_config.h')
    cfg_text = info_cfg.read_text() if info_cfg.exists() else ''
    c.check(kb.get('indicators') == {'caps_lock': 'A10', 'on_state': 0}
            and re.search(r'define LED_CAPS_LOCK_PIN A10\b', cfg_text) and re.search(r'define LED_PIN_ON_STATE 0\b', cfg_text),
            'Caps Lock LED on A10, active-low, compiled in (stock: PA10 active-low)')
    c.check(kb['ws2812']['pin'] == 'A1' and kb['rgblight']['led_count'] == 4, 'WS2812 on A1, 4 LEDs (vendor: PA1, 4 LEDs, GRB)')

    c.lines.append('')
    c.lines.append('RESULT: %s (%d failure%s)' % ('PASS' if c.failures == 0 else 'FAIL', c.failures, '' if c.failures == 1 else 's'))
    c.lines.append('NOTE: offline checks only. Compilation and these checks do not establish hardware compatibility.')
    text = '\n'.join(c.lines) + '\n'
    sys.stdout.write(text)
    if args.report:
        args.report.write_text(text)
    return 1 if c.failures else 0


if __name__ == '__main__':
    sys.exit(main())
