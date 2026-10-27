#!/opt/homebrew/opt/qmk/libexec/bin/python
"""Log USB arrivals/departures of falcon_kalam (bootloader and app) with timestamps.

Purely passive: it only lists the USB bus (no open, no requests). Start it,
then unplug and replug the keyboard normally; Ctrl-C (or --seconds) to stop.

    tools/usb_watch.py [--seconds 30] [--log artifacts/usb_watch.txt]

Answers: does the bootloader enumerate (1EAF:0003) for a short window after
power-on before the application (9DD1:9D89) appears, and for how long? If yes,
DFU can be entered by plug-in alone (`dfu-util -w ...`), without keys or pads.
Result for this unit: no window (docs/hardware.md §3, artifacts/usb_watch_replug.txt).

Uses pyusb (usb.core) from the Homebrew QMK CLI's Python, hence the shebang.
Polls the OS device list every 20 ms, so it only sees devices the host has
finished enumerating.
"""

import argparse
import sys
import time
from pathlib import Path

import usb.core

sys.path.insert(0, str(Path(__file__).resolve().parent))  # python3 -I drops the script directory
from falcon_kalam_facts import APP_PID, APP_VID, DFU_PID, DFU_VID

WATCH = {(DFU_VID, DFU_PID): 'bootloader (DFU)', (APP_VID, APP_PID): 'keyboard application (stock or falcon_kalam)'}


def present():
    return {(d.idVendor, d.idProduct) for d in usb.core.find(find_all=True) if (d.idVendor, d.idProduct) in WATCH}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--seconds', type=float, default=60)
    ap.add_argument('--log')
    a = ap.parse_args()
    out = open(a.log, 'w') if a.log else None
    t0, prev, since = time.monotonic(), set(), {}

    def emit(msg):
        line = '%8.3f s  %s' % (time.monotonic() - t0, msg)
        print(line, flush=True)
        if out:
            out.write(line + '\n')
            out.flush()

    emit('watching (now present: %s)' % (', '.join(WATCH[k] for k in sorted(present())) or 'none'))
    try:
        while time.monotonic() - t0 < a.seconds:
            cur = present()
            now = time.monotonic()
            for k in cur - prev:
                since[k] = now
                emit('+ %04X:%04X %s' % (k[0], k[1], WATCH[k]))
            for k in prev - cur:
                emit('- %04X:%04X %s (was present %.3f s)' % (k[0], k[1], WATCH[k], now - since.get(k, t0)))
            prev = cur
            time.sleep(0.02)
    except KeyboardInterrupt:
        pass
    emit('stopped')


if __name__ == '__main__':
    main()
