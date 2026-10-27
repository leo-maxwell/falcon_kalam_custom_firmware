# User manual

For the owner and for enthusiasts who want to use, test or flash this
firmware. It says what the keyboard does and how to work with it; the
internals are in the other documents (`docs/policy.md` for the design,
`docs/operations.md` for the maintainer's procedures).

The keyboard is the 迦蓝 clone of MatrixLAB's Falcon: a 60-key HHKB-layout
hot-swap board with a four-LED light module. This firmware replaces the
manufacturer's and is **not** the firmware the keyboard was sold with. It is
built for this unit only; never flash it onto MatrixLAB's own Falcon (an AVR
board with a different bootloader).

## 1. The two images

| Image | File | For |
|---|---|---|
| **ship** | `artifacts/falcon_kalam_ship.bin` | daily use |
| **dev** | `artifacts/falcon_kalam_dev.bin` | testing and diagnosis: adds the console traces, the full command set, the fault and light tests, the key tester in VIA, the bootloader dump |

Both behave the same at the keys and the lights. Each has a release number
(`docs/firmware.md` §1 lists the current one with the image hashes); VIA
shows it as the firmware version, and `tools/falcon_kalam_probe.py info`
prints it.

## 2. Layout and modes

The keymap is a copy of the HHKB Professional Classic: the same keys, the
same three setting modes, the same key codes.

```
Esc 1 2 3 4 5 6 7 8 9 0 - = \ `
Tab  Q W E R T Y U I O P [ ] Delete
Ctrl  A S D F G H J K L ; ' Return
Shift  Z X C V B N M , . / Shift Fn
      Alt ◇      Space      ◇ Alt
```

**Modes.** HHK mode is the factory setting, as on the Classic.

| Mode | Delete key | ◇ keys | Alt keys | Select it |
|---|---|---|---|---|
| HHK | Delete (Backspace with SW3 on) | Muhenkan / Henkan (no Command key) | Alt | SW1 and SW2 off |
| Mac | Backspace | Command | Option | Fn+Ctrl+M, or SW2 on |
| Win | Delete (Backspace with SW3 on) | Windows key | Alt | Fn+Ctrl+W, or SW1 on |

Fn+Ctrl+M and Fn+Ctrl+W (press the keys in that order) switch the mode until
the next plug-in; Fn+Ctrl+W returns to HHK mode when the switches select
HHK. On a Mac, switch to Mac mode once with Fn+Ctrl+Shift+2 (SW2) and replug;
the setting is saved.

**Fn layer** (Fn is the key right of the right Shift):

| Fn + | Sends | Modes |
|---|---|---|
| 1 … = | F1 … F12 | all |
| \ · ` | Insert · Delete | all |
| I · O · P | Print Screen · Scroll Lock · Pause | all |
| [ · / · ; · ' | Up · Down · Left · Right | all |
| K · L · , · . | Home · Page Up · End · Page Down | all |
| Delete | Backspace (HHK, Win) · Clear (Mac) | all |
| Tab | Caps Lock | Mac, Win |
| H · J · N · M | keypad * · / · + · − | Mac, Win |
| Return | keypad Enter | Mac |
| A · S · D · F | Volume − · Volume + · Mute · Eject | Mac |
| Esc | Power (macOS ignores it) | Mac |
| right ◇ | Stop | HHK |

A combination a mode does not have sends the plain key. Caps Lock on macOS:
a short Fn+Tab switches the input source, a long press toggles Caps Lock
(macOS's own rule). The Caps Lock LED follows the host, lighting 200 ms after
the host turns Caps Lock on and going off at once; macOS briefly asserts Caps
Lock on some short presses when it switches the input source, and the delay
keeps that from flashing the LED.

**Virtual DIP switches.** The Classic has five switches on its underside;
here they are saved settings, flipped with Fn+Ctrl+Shift+1 … 5. Like the
Classic's, a flip takes effect at the next plug-in. The lights answer each
flip: one off-blink = switched off, two = switched on.

| Switch | On |
|---|---|
| SW1, SW2 | power-on mode: both off HHK · SW1 on Win · SW2 on Mac · both on acts as HHK |
| SW3 | the Delete key sends Backspace (HHK and Win mode) |
| SW4 | the left ◇ key becomes a second Fn key |
| SW5 | Alt and ◇ swap places |

**Settings display.** Fn+Ctrl+Shift+0 (and every mode key) shows the
settings on the lights: 1.5 s the settings in effect, then, if a flip is
waiting for the next plug-in, a short dark gap and 1.5 s the settings after
the replug. LED 1 shows the mode (red HHK, blue Mac, green Win), LEDs 2–4
show SW3, SW4, SW5 (blue = on, dark = off). LEDs are counted from the left.

**Remapping.** VIA (§6) edits any key. The six VIA layers are the three
modes, base and Fn each, in the order HHK, Mac, Win. The switches act on top
of the keymap, so VIA shows the unchanged layers.

## 3. Reading the lights

The four LEDs are a status indicator, in pure red, green and blue only.
**All four green is normal.** Anything else means something; the colours
mean: green = fine, blue = a state worth knowing, red = attention. LED 1 is
the leftmost; "channel A" is LEDs 1–2, "channel B" LEDs 3–4.

| Lights | Meaning | What to do |
|---|---|---|
| all green | normal | — |
| all dark at plug-in, then green | the keyboard waits until the computer has configured it (at most 2 s) | nothing; this is every plug-in |
| all dark, three red blinks, then green | the settings were reset to the factory state (first boot of a new release, or Esc held at plug-in) | set your switches again; VIA keymaps are also reset |
| **all red, steady** | the keyboard is in the bootloader (DFU): it does not type | flash an image (§5), or replug to start the application; after a crash it stays red until replugged |
| LEDs 3–4 dim green | powered, but no computer has configured it: a charger, a power bank, a dead port or a hub that only powers | plug into a computer port; it also shows on a port that keeps power while the computer is off |
| LEDs 3–4 blue | a layer is locked on, or the base layer is not one of the three modes (a VIA key did that) | press the key that locked the layer; Fn+Ctrl+M/W restores a mode |
| LEDs 1–2 blue | debug output is on (dev: always, from boot) | Shift+Shift+D switches it off |
| LEDs 1–2 red | key presses are being printed to the console (a privacy warning) | Shift+Shift+K switches it off |
| LED 1 red / blue / green, LEDs 2–4 blue or dark, for 1.5 s | the settings display (§2) | — |
| all off-blink once or twice | a virtual switch was flipped (once = off, twice = on) | — |
| all dark while the computer sleeps | the host is asleep | — |
| dev: all blue blink | the bootloader dump is being served | — |
| Caps Lock LED (separate, under the Caps key) | the host's Caps Lock state, lit 200 ms after it turns on | — |

What the lights never do: react to typing, show effects, or change colour
through VIA. The brightness is fixed at 69 of 255 (bright) and the lowest step the LEDs have (dim).

## 4. Key combinations

**Command** combinations: hold **both Shift keys** and tap the key. They
work in every mode.

| Shift+Shift+ | Ship | Dev | Effect |
|---|---|---|---|
| H | ✓ | ✓ | lists QMK's commands on the console (the dev extras F and L are not in that list) |
| B | ✓ | ✓ | **enters the bootloader** (all red); the keyboard stops typing until flashed or replugged |
| D | ✓ | ✓ | debug output on/off (LEDs 1–2 blue while on) |
| X | ✓ | ✓ | matrix (key position) printout on/off, with debug on |
| K | ✓ | ✓ | key report printout on/off (LEDs 1–2 red while on) |
| V | ✓ | ✓ | prints the firmware version and build options |
| S | ✓ | ✓ | prints the status (layers, debug flags) |
| Esc | — | ✓ | enters the bootloader (alternate) |
| Backspace | — | ✓ | wipes the saved settings (switches, VIA keymaps) |
| E | — | ✓ | prints the saved settings |
| M, C, / | — | ✓ | mouse debug, console sub-mode, help (alternate) |
| F | — | ✓ | **fault test**: crashes the firmware on purpose to prove it lands in the bootloader (all red), 0.2 s after the console line |
| L | — | ✓ | **light test**: shows every indicator state in turn, about 20 s, each named on the console |

In ship every other combination types normally (the console shows a "?").
N, Caps and Z do nothing in dev either.

**At plug-in:** hold **Esc** while plugging in and keep it for two seconds.
This wipes the saved settings and enters the bootloader (all four LEDs red).
It is the recovery route when nothing else works and the way into DFU before
every flash.

**Mode and switch keys** (§2): Fn+Ctrl+M, Fn+Ctrl+W, Fn+Ctrl+Shift+0 … 5.

## 5. Flashing

What you need: a Mac with the toolchain described in `docs/operations.md`
§1 if you build yourself, or just `dfu-util` to flash a built image; the
keyboard; nothing else. There is no reset button and no programming header
in use: the bootloader is the only way in, so follow the order below and do
the test at the end.

1. **Choose the image**: ship for daily use. Its SHA-256 is in
   `docs/firmware.md` §1; flashing is refused for any other hash.
2. **Enter the bootloader**: unplug, hold Esc, plug in, hold for two seconds.
   All four LEDs turn red. (With the manufacturer firmware they stay dark.)
   `scripts/flash.sh check ship <sha256>` confirms the image and that the
   keyboard is in DFU without touching it.
3. **Flash**: `scripts/flash.sh flash ship <sha256>`. The script reads the
   old contents back first, writes the image, and the keyboard restarts by
   itself as "Falcon". If the download fails, **do not unplug**: the keyboard
   is still in the bootloader, run the command again.
4. **Test the way back in**: unplug, hold Esc, plug in: red again. Then
   `scripts/flash.sh verify ship` compares the flash with the image. Replug
   normally. Every flash ends with this test, because a release that could
   not get back into the bootloader would be the last one.
5. The first start after a new release resets the settings (three red
   blinks): set the switches and VIA keymaps again.

**Restoring the manufacturer firmware**: `docs/operations.md` §4 has the
command; it uses the same bootloader entry.

**If the keyboard does not type**: all red = bootloader, replug or flash.
Dark and silent on a computer = retry Esc at plug-in; if that never shows the
bootloader, the only remaining route is the reset pads on the board. Dark
with LEDs 3–4 dim green = the port gives power but no USB.

## 6. VIA

Open the VIA app, enable *Use V2 definitions* in its settings, and load
`via/falcon_kalam.json` from the *Design* tab; the keyboard appears as
"Falcon". Six layers: HHK base, HHK Fn, Mac base, Mac Fn, Win base, Win Fn.
Edit the layer of the mode you use. Keys outside the HHKB layout (a split
left Shift, a split space bar) exist in the matrix and can be assigned.

Rules: a `DF(n)` key must point at layer 0, 2 or 4 (the mode bases). There
is no lighting menu, and lighting commands cannot change the base light. The
key tester shows nothing in ship (the matrix is hidden from the computer);
use dev, or Shift+Shift+X on the console. Macros: 16, in 1,107 bytes.

## 7. Console

`qmk console -t -d 9DD1:9D89` (the QMK CLI) prints the keyboard's console:
command output, debug traces (D, X, K) and, in dev, every key action and
every settings read and write. Keep it running while testing; the fault
and light tests announce themselves there.

## 8. What the keyboard will not do

* 6-key rollover only: N-key rollover is off, because with it macOS loses
  the Caps Lock / input-source toggle. Enough for typing.
* Fn+Esc in Mac mode sends the Classic's Power code; current macOS ignores it.
* No key enters the bootloader on the default layers (Shift+Shift+B does).
* HHK mode has no Command key, as on the Classic; use Mac mode on a Mac.
* The key-selected mode and a locked layer are forgotten at the next plug-in;
  the switches and VIA keymaps are saved.
* After a crash the keyboard sits in the bootloader (all red) until replugged.
