# Custom hardware: design notes

Start at `README.md` in this folder. Parts and costs are in `BOM.md`; enclosure and production in `MANUFACTURING.md`.

Goal: take the CoreS3 alarm clock onto custom hardware in a shop-made metal case.

Requirements:

- **Sounds:** white, pink and brown noise, plus ambient sounds like a fire pit, at decent quality. Also the alarm.
- **Red lamp:** 660 nm, dimmable, separate from the screen.
- **Screen:** touch, true black (self-emissive, no backlight). **Readable from ~10 ft**, so around 4″.
- **Flicker:** little or none from both the screen and the lamp.
- **Power:** from a wall outlet via USB-C.
- **No Wi-Fi, no "smart" features.** Time is set by hand on the touch screen.

## Versions

| Version | What it is | Status |
| --- | --- | --- |
| V1 | M5Stack CoreS3 | Done (this repo) |
| V2 | Breadboard: a dev board with the final kind of screen, plus separate modules for audio, lamp and RTC | Next |
| V3 | Enclosed: custom PCB + metal case, USB-C power, **no radio on the board** | |
| V4 | Maybe: Wi-Fi time sync, battery so the alarm rings during an outage | Optional |

## Summary of recommendations

| Block | Recommendation |
| --- | --- |
| MCU | **ESP32-P4**. It has no radio at all, drives both small QSPI screens and large MIPI-DSI screens, and has 32 MB of in-package PSRAM (P4NRW32). For V4, Wi-Fi is added as a separate ESP32-C6, which is how Espressif's own P4 boards do it. |
| Screen | **~4″ AMOLED + capacitive touch over MIPI-DSI.** The reference part is LilyGO's 4.1″ 1232×568 AMOLED (RM69A10 driver, GT9895 touch) on the T-Display-P4. |
| Firmware display stack | Draw into an off-screen canvas, then hand the frame to ESP-IDF's `esp_lcd` panel driver. The panel driver is the only per-screen code. |
| Sounds | White/pink/brown noise generated live; fire pit, rain, etc. as long recordings, on a microSD card in V2 and onboard SPI NAND in production. |
| Audio out | I2S amp (MAX98357A) + a good 2–2.5″ full-range driver in a sealed, damped chamber. |
| Red lamp | 660 nm LEDs on a linear constant-current sink with an analog set-point, so no PWM and zero flicker. |
| Timekeeping | High-accuracy RTC (RV-3028-C7, ±1 ppm at 25 °C) backed by a supercap (no battery to ship or replace). Manual set and drift trim in the UI, DST rules in firmware. |
| Power | USB-C, 5 V / 3 A. The same port flashes the board. |

## Timekeeping without Wi-Fi

Wi-Fi in V1/V2 would only ever have been for setting the clock over the network (NTP). Dropping it means:

- **A manual "Set time" screen is needed.** Today the firmware only sets the RTC from the build timestamp when the RTC is blank (`rtc_needs_set` in `alarm_face.hpp`). Worth adding in V1 now: an hours/minutes/date page in the same style as the alarm page.
- **The RTC has to be accurate on its own.**
  - The common PCF8563/BM8563 (CoreS3 and most dev boards) drifts on the order of **minutes per month**.
  - The **RV-3028-C7** is factory-calibrated to ±1 ppm at 25 °C, which works out to **~30 s per year** in a room at steady temperature. It draws ~45 nA, so a small supercap or CR2032 keeps time for weeks to years with the power off.
  - The DS3231 (temperature-compensated, ±2 ppm) is the other good choice. It is bigger and draws more.
- **Drift trim:** both RTCs have an offset/aging register. A "clock runs N seconds/month fast" setting lets you correct the last bit by hand.
- **Daylight saving time:** put the DST rule for your region in firmware, so the clock changes itself twice a year with no network. A "DST: auto/off" setting covers the rest.
- **Settings** already live in flash (NVS), so they survive power loss today.

With a backup cell on the RTC, a power cut only silences the clock while the power is out. It comes back with the right time and settings. A full LiPo so the alarm **rings** during an outage stays in V4.

## Screen

### Type: AMOLED

AMOLED is emissive, so black pixels are fully off, and each pixel holds its current through the frame instead of being row-scanned. LCD always has a backlight. E-paper and memory LCD can't be read in the dark. Passive-matrix OLED flickers by design and doesn't come in 4″.

### Size for 10 ft

A common signage rule of thumb is about **1″ of character height per 10 ft** for easy reading, so aim for digits of 1–1.5″ or more.

A **wide** panel suits a clock better than a 4:3 panel, because "12:34" is itself about 2.5:1:

| Panel | Active area (approx.) | Largest "HH:MM" digit height |
| --- | --- | --- |
| 2.0″ 4:3 (CoreS3, V1) | 1.6″ × 1.2″ | ~0.6″ |
| 4.0″ 4:3 | 3.2″ × 2.4″ | ~1.2″ |
| **4.1″ 1232×568 (≈2.2:1)** | **3.7″ × 1.7″** | **~1.4″** |

Menus get less vertical room on the wide panel. They'll need a two-column layout rather than today's stacked rows, but at 1232×568 there's plenty of resolution for it.

### One firmware setup across sizes

The trap to avoid: small AMOLEDs (≤ ~2.4″) almost all use **QSPI**, while 4″-class AMOLEDs use **MIPI-DSI**. The ESP32-S3 can only do the former, while the **ESP32-P4 does both**. Using the P4 from V2 on means screen size never forces a chip change.

To keep the application code identical across screens:

- **Draw into an off-screen framebuffer** in PSRAM. The obvious engine is LovyanGFX/M5GFX's `LGFX_Sprite`, which has the same API `src/main.cpp` already calls on `M5.Display`, so most drawing code carries over. At 1232×568×16-bit the framebuffer is 1.4 MB, which is easy in 32 MB of PSRAM.
- **Push finished frames through ESP-IDF's `esp_lcd` API**, which has one interface for QSPI and DSI panels. Vendors ship `esp_lcd` drivers for their panels (LilyGO recommends ESP-IDF for the T-Display-P4), and touch comes through `esp_lcd_touch` the same way.
- **Describe each screen in a board header:** resolution, panel driver, touch driver, pins. Layout code works in fractions of width/height, as `alarm_face.hpp` already mostly does. Swapping to a different size or panel then means adding one board file.
- **Fonts:** at ~330 ppi, scaled bitmap fonts look blocky. Use smooth (anti-aliased) fonts rendered at the target size. LovyanGFX's VLW fonts or LVGL's font converter both work.
- `include/alarm_face.hpp` (alarm state machine, layout maths, formatting) is plain C++ and carries over unchanged, host tests included.

If LovyanGFX turns out not to build cleanly on the P4 with this panel, LVGL on top of `esp_lcd` is the fallback. It is the same framebuffer-and-driver structure with a different drawing API.

### Flicker on AMOLED

Many AMOLED driver ICs dim the **whole panel** (brightness command `0x51`) by PWM-ing the emission. They often switch to PWM only below some brightness threshold, at a few hundred Hz. I couldn't confirm either way for the RM69A10, so **measure it in V2** (see "How to measure flicker" below).

The mitigation works whatever the IC does:

- Leave panel brightness at a level that measures flicker-free.
- Dim by **pixel value** instead: draw the digits in `color565(r, 0, 0)` with a smaller `r` at night. AMOLED sets gray levels by pixel drive current, not by blinking.

In the current code that means night dimming changes `ink` rather than calling `setBrightness`.

### Burn-in

The same digits all night for years is OLED's worst case. Mitigations:

- Red-only content helps, because red subpixels age much more slowly than blue.
- **Pixel-shift** the whole face by a few pixels every minute or so.
- Keep night levels low.
- Blank or dim the screen after a period with no touches during the day.

### Touch through the case

The panel's own cover glass is the touch surface. Keep it flush with or slightly proud of the metal bezel, with no bare metal over the active area. Ground the case (see below) so touch stays stable.

## Sounds

### What to play

- **White / pink / brown noise, generated live**, never looped:
  - White: xorshift PRNG → 16-bit samples at 44.1 kHz.
  - Pink: white through Paul Kellet's pink filter.
  - Brown: white through a leaky integrator.

  A couple of EQ bands (a low shelf for "deeper" or "brighter") give a tone control.
- **Fire pit, rain, ocean, fan, etc.: recordings.** Procedural fire (rumble plus random filtered crackles) is possible, but good recordings sound better. So:
  - In V2, store them on a **microSD card**. A 60-minute mono recording is ~60 MB as MP3/Opus or ~150–200 MB as FLAC, and you can add sounds without reflashing. For production, use 128 MB of onboard SPI NAND loaded over USB-C instead (see `BOM.md`).
  - Use long recordings (30–60 min) so the loop point isn't noticeable, and crossfade a few seconds across it.
  - Sources: CC0 recordings on freesound.org, or your own recorded at an actual fire pit.
  - Decoding MP3/FLAC on the P4 is light work (ESP-ADF, or the arduino-audio-tools library if staying on Arduino).
- **Mixing:** layering is cheap. For example, brown noise under the fire recording, each with its own level.
- Sleep timer with a slow fade-out, and a crossfade from the noise into the alarm sound.

### Hardware

- **Amp: MAX98357A.** I2S in, mono class-D, up to about 3 W into 4 Ω from 5 V. It runs directly off VBUS, and its SD_MODE pin lets firmware hard-mute it so there's no idle hiss.
- **Speaker:** a 2–2.5″ full-range driver, 4 Ω. At bedside volume the speaker and its box set the quality, far more than the DAC.
- **Enclosure (matters most in a metal case):**
  - Sealed rear chamber of a few hundred mL, filled with wool felt or wool batting.
  - Constrained-layer damping (butyl or bitumen mat) on large flat panels, so the case doesn't ring or buzz.
  - Gasket the speaker mount, and put a perforated grille in front with at least 30–40% open area.
- Mono is fine for a nightstand.

## Red lamp (660 nm)

- Use **655–660 nm deep-red mid-power LEDs** (3528/5050 class) behind a diffuser, so there's no hot spot. 660 nm looks dimmer per watt than 625 nm, so plan for a few more LEDs than you'd expect.
- **Driver: DC current, not PWM.** PWM gets worse (shorter pulses) exactly at dim night settings.

```
 5V ── LEDs ──┐
              D
   op-amp ───G   N-MOSFET
  +  set     S
  -  ────────┴── R_sense (range A ≈ 2 Ω, range B ≈ 200 Ω) ── GND
```

- An op-amp holds V(R_sense) equal to the set-point voltage, so LED current = V_set / R_sense, as pure DC.
- **Set-point:** the ESP32-P4 (like the S3) has **no DAC**. Use a small I2C DAC (12-bit, for example MCP4725), or ≥20 kHz PWM through a two-stage RC filter. The LEDs still see DC, because the op-amp loop is much slower than the PWM.
- **Two ranges:** switch R_sense with a second MOSFET. 0–1 mA gives a smooth, very dim night-light; 0–250 mA gives reading light. One range can't cover both, because op-amp offset swamps the bottom end.
- **Op-amp:** rail-to-rail, low offset (OPA333 / MCP6001 class), with a small RC on the gate for stability.
- **Heat:** about (5 V − V_led − V_sense) × I, which is under 1 W at full. Give the MOSFET a copper pour, or bolt the board to the case. Two LEDs in series per string (660 nm Vf ≈ 2.0–2.2 V) cuts the waste roughly in half.
- The same circuit can do a **sunrise ramp** before the alarm: a smooth fade from zero with no flicker.

## Power (V3)

- **USB-C receptacle, 16-pin (USB 2.0):**
  - 5.1 kΩ pull-down on each of CC1/CC2, so any USB-C charger supplies 5 V.
  - D+/D− go to the P4's USB, so the same port flashes the board and gives a serial console.
  - Read the CC voltage with an ADC pin. It says whether the adapter offers 0.5, 1.5 or 3 A, so firmware can cap lamp brightness and volume on a weak adapter instead of browning out.
- **Protection:**
  - TVS diode + polyfuse or eFuse on VBUS.
  - ESD array (USBLC6-2 class) on D+/D−.
- **Rails:** a buck to 3.3 V. The P4 and the AMOLED need extra rails (panel ELVDD/ELVSS, P4 core); take these from the module's or panel's reference design rather than inventing them.
- **Budget at 15 W:**
  - P4 + screen: ~1–1.5 W
  - Lamp: < 1.5 W
  - Audio: ~3 W peak

  That leaves plenty of margin.

## Metal case notes

- **No Wi-Fi means no antenna problem.** V3 has no radio, so the metal box doesn't matter. For V4, add the C6 with a U.FL pigtail to a bulkhead antenna, or put a non-metal window near it.
- **Grounding / ESD:** tie the case to circuit ground at one point near the USB-C connector, either directly or USB shield → GND through 1 MΩ ∥ 4.7 nF. USB-C adapters are isolated, so this is safe, and it keeps capacitive touch stable.
- **Light leaks:** make the lamp diffuser and screen bezel light-tight to each other.
- **Heat:** minimal. The lamp MOSFET can sink into the case.

## Optional extras

- **Ambient light sensor** (VEML7700 class): auto day/night face and brightness, instead of a fixed schedule.
- **Big physical snooze/stop button** on top of the case, easier than finding the screen half-asleep.

## How to measure flicker

- **Quick:** phone slow-motion video (240 fps) pointed at the screen or lamp. PWM shows as rolling bands.
- **Proper:** a photodiode (BPW34) into a 10 kΩ load or transimpedance amp, viewed on a scope. Check every brightness step, especially the dimmest night settings.

## Plan

1. **V1 (now, CoreS3):**
   - Add a manual "Set time" page.
   - Move day/night dimming toward "dim by colour" so the code is ready for AMOLED.
   - Split `main.cpp` from M5Unified behind a thin `Board` interface (draw, touch, audio out, RTC, lamp), with the CoreS3 as the first implementation.
2. **V2 (breadboard):**
   - Brain + screen: LilyGO T-Display-P4 with the **4.1″ AMOLED** assembly. It already has a P4, SD card slot, RTC and an audio codec. Its C6 radio is simply never enabled.
   - On the breadboard:
     - RV-3028 breakout
     - MAX98357A breakout + speaker in an MDF/cardboard test box
     - 660 nm LED sink circuit
   - Port the firmware, write the noise generator and SD playback, measure flicker on both screen and lamp, and check readability at 10 ft.
3. **V3 (enclosed):** decide based on what V2 showed.
   - **(a) Carrier PCB:** the T-Display-P4 bolts in as brain + screen. Your PCB has USB-C power, the amp, the lamp driver and the RTC. Lowest risk.
   - **(b) Fully custom PCB:** P4 + bare AMOLED panel on its FPC. Only if you can get the panel's datasheet and a reliable supply. The 2-lane DSI pairs need 100 Ω differential routing (4-layer impedance-controlled stackup; standard at JLCPCB/PCBWay).

   Design the case around the chosen board: speaker chamber and lamp diffuser first, cosmetics last. The shop-made case proves the proportions; production moves to an aluminium extrusion + two end caps (see `BOM.md`).
4. **V4 (optional):** ESP32-C6 for NTP time sync, and a LiPo + power-path charger so the alarm rings during outages.

## Still open

- 4.1″ wide (recommended, bigger digits) vs. a 4″ 4:3 panel.
- V3 route (a) carrier or (b) fully custom. Decide after V2.
