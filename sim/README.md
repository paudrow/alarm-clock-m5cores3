# Firmware simulator

Runs the clock's real firmware, `src/main.cpp` and `include/alarm_face.hpp`, in a web browser and on a PC. The 3D viewer (`hardware/cad/`) uses it to bring the model to life: the screen, the lamp's glow in the glass, the sleep sounds, the alarm and the top button are all the firmware's own code.

## How it works

- `shim/M5Unified.h` and `shim/Preferences.h` stand in for the M5Stack libraries. They cover only what `main.cpp` uses.
  - **Drawing** (rectangles, rounded rectangles, lines, circles, triangles, text in the TFT fonts and FreeSans), **tones**, **raw sound buffers** (the sleep sounds), **brightness** and **`analogWrite`** (the lamp, pin 9) are passed to JavaScript, which draws on a canvas, plays sound through Web Audio and lights the model's glass.
  - **Touch**, the **button**, **serial input** and the **clock chip** read from JavaScript. The page can run the clock fast or set its time.
  - **Settings** (`Preferences`) are stored in the browser's localStorage, so they survive a reload the way they survive a power cut on the clock.
- `sim.cpp` exports the entry points:

  | Export | What it does |
  | --- | --- |
  | `sim_init(w, h)` | Boots (or reboots) the clock with a w × h screen |
  | `sim_loop()` | One pass of the firmware's `loop()`; the page calls it every animation frame |
  | `sim_touch(x, y, down)` | Where the finger is |
  | `sim_button()` | Presses the physical button once |
  | `sim_io()`, `sim_input(n)` | Types the first n bytes of the io buffer on the serial console |
  | `sim_state()` | Writes the clock's state as JSON into the io buffer |

- `build.py` compiles it all to `clock.wasm`, about 62 KB with no system imports.

## Serial commands

The clock reads these over USB serial (115200 baud), and the simulator, tests and viewer use the same path:

| Command | |
| --- | --- |
| `lamp on`, `lamp off`, `lamp toggle`, `lamp 1`–`100` | The lamp, and its brightness in % |
| `sound on`, `sound off`, `sound toggle`, `sound white`/`pink`/`brown` | Sleep sounds |
| `time HH:MM[:SS]` | Sets the clock |
| `alarm HH:MM`, `alarm on`, `alarm off` | Sets and turns on the alarm |
| `winddown on`, `winddown off`, `winddown HH:MM HH:MM` | The evening lamp: on from the first time, fading out over the last 15 minutes to off at the second |
| `wake on`, `wake off`, `wake <before> <after>` | The wake-up light: minutes before the alarm to brighten over, and minutes after to stay on (0, 10, 15, 20, 30, 45 or 60) |
| `go day`, `go night`, `go winddown`, `go sunrise`, `go alarm` | Jumps the time: noon; the middle of the night hours; the start of wind down; the start of the wake-up light; 10 s before the alarm (each turns on what it needs) |
| `button` | Presses the button: snooze while ringing, otherwise the lamp |
| `state` | Prints the state as one line of JSON |
| `help` | Lists these |

In the viewer, the same is available from the browser console:

```js
clock.command("lamp 40");   clock.goTo("night");   clock.state().alarm.state
clock.tap(x, y);            clock.button();        clock.setSpeed(60)
```

## Build and test

```sh
pip install ziglang==0.16.0     # Zig's C++ toolchain targets WebAssembly with libc++ included
python sim/build.py             # writes sim/clock.wasm
node sim/smoke_test.js          # boots it at three sizes and drives it through the commands
python hardware/cad/viewer.py   # re-embeds it in the viewer
```

`clock.wasm` is committed so the viewer can be rebuilt without Zig. **Rebuild and commit it whenever `src/main.cpp` or `include/` changes.** The build is reproducible: `build.py` pins the firmware's `__DATE__`/`__TIME__` stamp, and the Zig version is pinned. The same source therefore always gives the same bytes.

The same shim also builds natively. `test/host/` implements the JavaScript side in C++, so `test/firmware_test.cpp` can run the real firmware on a PC: it taps through every page at ten screen sizes, checks every draw call stays on screen and every label fits its row, rings, snoozes and stops alarms, streams sleep sounds through their fade and timer, runs the wind-down and wake-up lamp schedules, reboots with saved (and corrupt, and older versions') settings, and throws thousands of random inputs at it. `test/render_screens.cpp` draws every screen to `test/screens/`. The build lines are in `.github/workflows/ci.yml`.

## CI

The `simulator` job in `.github/workflows/ci.yml` (and the Forgejo copy) fails in three cases:
- **The firmware no longer builds for the browser.** Usually `main.cpp` started using an M5Unified call the shim doesn't cover, and the error names it (for example `no member named 'setRotation' in 'SimDisplay'`). Add the call to `shim/M5Unified.h`.
- **`clock.wasm` doesn't match the source.** The firmware changed and `sim/clock.wasm` wasn't rebuilt. Run `python sim/build.py` and commit it.
- **`smoke_test.js` fails.** The browser build misbehaved: no clock face, the gear didn't open Settings, a command didn't work, it crashed or drew off screen under random input, or settings didn't survive a reboot.

The `firmware` job builds the real CoreS3 firmware with PlatformIO, which catches a call the shim accepts but M5Unified doesn't.

## Differences from the real clock

- **Fonts** are close stand-ins (Chivo for the TFT fonts and FreeSans, and a drawn seven-segment font for font 7), so text widths differ slightly from the device. The firmware measures text and fits it, so layouts adapt either way.
- **Screen size** is the one being designed for, not the CoreS3's 320 × 240: 616 × 284 for the 4.1″ AMOLED (half its native 1232 × 568), 600 × 450 for the 2.41″. The firmware lays itself out from `width()` and `height()`.
- **Sound** is Web Audio: square-wave beeps at the firmware's frequencies and volumes, and the firmware's own noise samples, not the CoreS3 speaker.
- **The lamp** is the model's glass glowing with the level the firmware writes to pin 9. On the CoreS3 that's Port B, for a lamp driver on the bench.
