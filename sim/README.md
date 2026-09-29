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
pip install ziglang     # Zig's C++ toolchain targets WebAssembly with libc++ included
python sim/build.py     # writes sim/clock.wasm
python hardware/cad/viewer.py   # re-embeds it in the viewer
```

`clock.wasm` is committed so the viewer can be rebuilt without Zig. Rebuild it whenever `src/main.cpp` or `include/` changes.

If `main.cpp` starts using an M5Unified call the shim doesn't cover, the build fails with an unknown-member error. That's the cue to add the call to `shim/M5Unified.h`.

## Differences from the real clock

- **Fonts** are close stand-ins (Chivo for fonts 2, 4 and 8, and a drawn seven-segment font for font 7), so text widths differ slightly from the device.
- **Screen size** is the one being designed for, not the CoreS3's 320 × 240: 616 × 284 for the 4.1″ AMOLED (half its native 1232 × 568), 600 × 450 for the 2.41″. The firmware lays itself out from `width()` and `height()`, so this also shows how the current UI adapts to the new screens.
- **Sound** is a square-wave tone at the firmware's frequencies and volumes, not the CoreS3 speaker.
