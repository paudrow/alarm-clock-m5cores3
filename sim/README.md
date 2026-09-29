# Firmware simulator

Runs the clock's real firmware, `src/main.cpp` and `include/alarm_face.hpp`, in a web browser. The 3D viewer (`hardware/cad/`) uses it to put a working touchscreen on the model: the clock face, menus, alarm, sound previews and day/night dimming are all the firmware's own code.

## How it works

- `shim/M5Unified.h` and `shim/Preferences.h` stand in for the M5Stack libraries. They cover only what `main.cpp` uses.
  - **Drawing** (rectangles, lines, circles, text in fonts 2, 4, 7 and 8), **tones** and **brightness** are passed to JavaScript, which draws on a canvas and plays sound through Web Audio.
  - **Touch** and the **clock chip** read from JavaScript. The page can run the clock fast or set its time.
  - **Settings** (`Preferences`) are stored in the browser's localStorage, so they survive a reload the way they survive a power cut on the clock.
- `sim.cpp` exports `sim_init(w, h)`, `sim_loop()` and `sim_touch(x, y, down)`. The page calls `sim_loop()` once per animation frame, which is the firmware's own `loop()`.
- `build.py` compiles it all to `clock.wasm`, about 44 KB with no system imports.

## Build

```sh
pip install ziglang==0.16.0     # Zig's C++ toolchain targets WebAssembly with libc++ included
python sim/build.py             # writes sim/clock.wasm
node sim/smoke_test.js          # boots it, taps the menu, runs random input
python hardware/cad/viewer.py   # re-embeds it in the viewer
```

`clock.wasm` is committed so the viewer can be rebuilt without Zig. **Rebuild and commit it whenever `src/main.cpp` or `include/` changes.** The build is reproducible: `build.py` pins the firmware's `__DATE__`/`__TIME__` stamp, and the Zig version is pinned. The same source therefore always gives the same bytes.

## CI

The `simulator` job in `.github/workflows/ci.yml` (and the Forgejo copy) fails in three cases:
- **The firmware no longer builds for the browser.** Usually `main.cpp` started using an M5Unified call the shim doesn't cover, and the error names it (for example `no member named 'setRotation' in 'SimDisplay'`). Add the call to `shim/M5Unified.h`.
- **`clock.wasm` doesn't match the source.** The firmware changed and `sim/clock.wasm` wasn't rebuilt. Run `python sim/build.py` and commit it.
- **`smoke_test.js` fails.** The firmware didn't draw a clock at one of the screen sizes, the gear didn't open the menu, it crashed or drew garbage under random input, or saved settings didn't load after a reboot.

## Differences from the real clock

- **Fonts** are close stand-ins (Chivo for fonts 2, 4 and 8, and a drawn seven-segment font for font 7), so text widths differ slightly from the device.
- **Screen size** is the one being designed for, not the CoreS3's 320 × 240: 616 × 284 for the 4.1″ AMOLED (half its native 1232 × 568), 600 × 450 for the 2.41″. The firmware lays itself out from `width()` and `height()`, so this also shows how the current UI adapts to the new screens.
- **Sound** is a square-wave tone at the firmware's frequencies and volumes, not the CoreS3 speaker.
