# Custom hardware: design notes

Goal: take the CoreS3 alarm clock onto a custom PCB in a shop-made metal case.

Requirements:

- **Sound:** white noise and other masking sounds at decent quality, plus the alarm.
- **Red light:** a dimmable red lamp, separate from the screen.
- **Screen:** touch, with true black (self-emissive, no backlight).
- **Flicker:** little or none from both the screen and the lamp.
- **Power:** from a wall outlet, ideally through USB-C.

## Summary of recommendations

| Block | Recommendation | Why |
| --- | --- | --- |
| MCU | ESP32-S3-WROOM-1**U**-N16R8 (U.FL antenna) | Same chip as the CoreS3, so the firmware carries over. Native USB. 8 MB PSRAM holds a full-screen framebuffer. The U.FL connector matters because the case is metal. |
| Screen | AMOLED + capacitive touch, QSPI (for example 2.41″ 600×450, RM690B0 driver + FT6336 touch) | Emissive, so black pixels are off. The 4:3 aspect matches the current 320×240 layouts. |
| Audio | I2S class-D amp (MAX98357A) + a good 2–2.5″ full-range driver in a sealed, damped chamber | Simple and runs from 5 V. The speaker and the box set the quality far more than the DAC does. |
| Red lamp | Deep-red LEDs on a **linear constant-current sink** with analog set-point (no PWM) | Pure DC means zero flicker at any brightness, including very dim night-light levels. |
| Power | USB-C, 5 V / 3 A (5.1 kΩ on CC1/CC2), same port for flashing | 15 W is plenty, and one cable does power and programming. |
| Timekeeping | RV-3028-C7 RTC + supercap or CR2032, NTP over Wi-Fi when available | Keeps time through outages. |

## Screen

### Why AMOLED

- **LCD (what the CoreS3 has):** always has a backlight, so black glows grey in a dark room.
- **E-paper:** truly flicker-free, but you can't read it in the dark without a frontlight, and a frontlight is just a backlight again.
- **Sharp memory LCD:** same problem as e-paper.
- **Passive-matrix OLED (small SSD13xx-type modules):** has true black, but it lights one row at a time, so it flickers by design. The panels are also small.
- **Active-matrix OLED (AMOLED):** each pixel holds its current for the whole frame, gives true black, and comes in 1.8–2.4″ sizes with touch. **This is the pick.**

Candidate panels. These all ship on ESP32-S3 dev boards, so there is working driver code to start from:

| Size | Resolution | Display IC | Touch IC | Reference board |
| --- | --- | --- | --- | --- |
| 2.41″ | 600×450 | RM690B0 | FT6336 | Waveshare ESP32-S3-Touch-AMOLED-2.41 |
| 2.06″ | 410×502 | CO5300 | FT3168 | Waveshare ESP32-S3-Touch-AMOLED-2.06 |
| 1.8″ | 368×448 | CO5300 | CST820 | Waveshare ESP32-S3-Touch-AMOLED-1.8 |

The 2.41″ panel is the closest match: it is slightly bigger than the CoreS3's 2.0″, and its 4:3 aspect means the row- and width-relative layout code in `alarm_face.hpp` mostly scales without changes.

### Flicker on AMOLED

AMOLED avoids the backlight PWM problem, but many AMOLED driver ICs reduce **global** brightness (the `0x51` brightness command) by PWM-ing the emission duty cycle. They often switch to PWM only below some threshold, at a few hundred Hz. I haven't found datasheet confirmation either way for RM690B0 or CO5300, so **measure it** (see "How to measure flicker" below).

Mitigation that works whatever the IC does:

- Leave the panel's global brightness at a level that measures flicker-free.
- Dim by **pixel value** instead. Draw the digits in `color565(r, 0, 0)` with a smaller `r` at night rather than lowering the panel brightness. In AMOLED, gray levels are set by the pixel drive current, not by blinking the pixel.
- The firmware already has one place that sets the ink colour (`ink` in `src/main.cpp`) and one that sets brightness (`M5.Display.setBrightness`). Moving night dimming from the second to the first is a small change.

### Burn-in

A clock face showing the same static digits all night for years is the worst case for OLED burn-in. Mitigations:

- Red-only content helps. Red subpixels age much more slowly than blue.
- **Pixel-shift** the whole face by a few pixels every minute or so. The layout already centres things, so a small global offset is easy.
- Keep night brightness low (which you want anyway), and blank or dim the screen during the day when no one has touched it.

### Touch through the case

Capacitive touch works fine through the panel's own cover glass. Leave the glass flush with or just proud of the metal bezel, and don't let bare metal touch the active area. Ground the case (see below) so touch readings stay stable.

## Sound

### "Noise cancelling": what is realistic

True active noise cancellation doesn't work in a bedroom. ANC works in headphones because the space it controls is tiny and fixed. In a room it only cancels low frequencies at one point in space, and moving your head breaks it. What noise machines do instead is **masking**: steady broadband sound that hides intermittent noises.

What this board can do well:

- **White / pink / brown noise, generated live** on the ESP32-S3, rather than a recorded loop. There are no loop seams and it uses no storage.
  - White: xorshift32 PRNG → 16-bit samples at 44.1 or 48 kHz.
  - Pink: white noise through Paul Kellet's pink filter, or Voss-McCartney.
  - Brown: white noise through a leaky integrator.
- **Shaped noise:** a few biquad EQ bands on top of the above, tuned to what you want to mask. Low shelf up for traffic rumble, a mid lift around 300 Hz–3 kHz for voices.
- **Adaptive masking (optional):** an I2S MEMS mic (for example ICS-43434) measures the room level and raises the masking a little when the room gets louder. This is the useful version of "noise cancelling" in a room.
- Fade-in at bedtime, fade-out on a timer, and a crossfade into the alarm sound.
- Nature sounds (rain, ocean) as compressed files in flash, if wanted. 16 MB is plenty.

### Hardware

- **Amp (v1): MAX98357A.** I2S in, mono class-D out, filterless, up to about 3 W into 4 Ω at 5 V. It runs directly off VBUS. Its SD_MODE pin gives firmware a hard mute/shutdown, so there is no idle hiss when nothing is playing.
- **Upgrade path, if v1 isn't good enough:** a class-D amp with built-in DSP, such as TI's TAS58xx family. This needs a higher PVDD, so it also needs a USB-PD trigger (see Power). Only go this way if the MAX98357A version disappoints.
- **Speaker:** a 2–2.5″ full-range driver, 4 Ω. At bedside volume, the driver and box set the quality, not the DAC.
- **Enclosure (this matters most in a metal case):**
  - Give the driver a sealed rear chamber, a few hundred mL, lined with polyfill or felt.
  - Metal panels ring. Add constrained-layer damping (butyl or bitumen mat) to the large flat panels and gasket the speaker mount.
  - Use a perforated grille in front of the driver with at least 30–40% open area.
- Mono is fine for noise. Stereo adds little on a nightstand.

## Red lamp

### Wavelength

- **~620–630 nm red:** looks brighter per watt, reads as "red light".
- **~655–660 nm deep red:** even less effect on melatonin, but looks dimmer per watt and more "burgundy".

Both are far better than any white light at night. I'd put a few of each on the first board and decide by eye. Use mid-power LEDs (3528/5050, 0.1–0.2 W each) behind a diffuser rather than one high-power emitter, so the lamp has no hot spot.

### Driver: DC, not PWM

PWM dimming is the usual source of LED flicker, and it gets worse (shorter duty cycle) exactly at the dim settings you'd use at night. Instead:

```
 5V ──┬── LEDs ──┐
      │          D
      │   op-amp ┤G   N-MOSFET
 set ─┤+        S│
 point│   ┌──────┴── sense R (range A: ~2 Ω, range B: ~200 Ω)
      └─ -┘                  │
                            GND
```

- An op-amp holds the MOSFET so that V(sense) equals the set-point voltage. LED current = V_set / R_sense, as pure DC.
- **Set-point:** the ESP32-S3 has **no DAC**, so use one of:
  - a small I2C DAC (12-bit, for example MCP4725), or
  - LEDC PWM at 20 kHz or more through a two-stage RC filter. What reaches the LEDs is still DC, because the op-amp loop is far slower than the PWM.
- **Two current ranges:** switch the sense resistor with a second MOSFET. A range of 0–1 mA gives a smooth, stable, very dim night-light, and 0–250 mA gives reading brightness. One range can't cover both well, because op-amp offset swamps the bottom end.
- Pick a rail-to-rail, low-offset op-amp (for example the TI OPA333 or MCP6001 class) and add a small RC on the gate for stability.
- Heat: roughly (5 V − 2.1 V − V_sense) × I, so about 0.7 W at 250 mA. Give the MOSFET a copper pour, or bolt the board to the case. Putting two LEDs in series per string reduces this.

This circuit can also produce a **sunrise ramp** before the alarm, rising from zero in fine steps with no flicker.

## Power

- **USB-C receptacle, 16-pin (USB 2.0).**
  - 5.1 kΩ pull-down on each of CC1/CC2. Any USB-C charger then supplies 5 V.
  - D+/D− go to the ESP32-S3's native USB, so the same port flashes the board and gives a serial console. No USB-UART chip is needed.
  - **Read CC with an ADC pin.** The CC voltage tells you whether the adapter offers 0.5, 1.5, or 3 A. Firmware can cap lamp and volume on a weak adapter instead of browning out.
- **Protection:**
  - VBUS: TVS diode + polyfuse or eFuse.
  - D+/D−: ESD array (USBLC6-2 class).
  - Reverse-current protection if you add a battery.
- **3.3 V rail:** a small buck (1 A+) for the ESP32-S3 and panel logic. Wi-Fi TX peaks are a few hundred mA. The AMOLED module usually makes its own panel voltages from 3.3 V or VBAT; check the chosen panel's datasheet.
- **Budget at 5 V / 3 A = 15 W:**
  - ESP32 + Wi-Fi: ~0.5 W
  - AMOLED, mostly black: well under 0.5 W
  - Lamp: ~1.3 W max
  - Audio: ~3 W peak

  That leaves plenty of headroom.
- **Higher voltage (optional):** if you want a louder amp later, a USB-PD trigger IC (CH224K, IP2721 or STUSB4500 class) can request 9/12/15 V from a PD charger. Not needed for v1.

### Power outages (decide early)

An alarm clock that stays silent after a power blip is worse than useless.

- **Minimum:** an RTC with its own backup. For example, the RV-3028-C7 draws tens of nA and is very accurate, with a supercap or CR2032. Settings already live in NVS (flash), so they survive.
- **Better:** a single-cell LiPo with a power-path charger (BQ24074 class), so the alarm still **rings** during an outage. It adds a charger, a battery, and one more thing in the case to think about.

## Metal case gotchas

- **Wi-Fi:** a closed metal box is a Faraday cage. Either use the -1U module with a U.FL pigtail to a bulkhead antenna, or put a non-metal window (wood, acrylic, the screen itself) right next to the PCB antenna. This matters because NTP is how the clock keeps exact time.
- **Grounding / ESD:** tie the case to circuit ground at one point near the USB-C connector. The usual choice is the USB shield to GND through 1 MΩ ∥ 4.7 nF, or a direct bond. USB-C wall adapters are isolated, so a bonded case is safe. This also keeps capacitive touch stable.
- **Heat:** almost none. The lamp MOSFET is the hottest part and can sink into the case.
- **Light leaks:** make the lamp's diffuser and the screen bezel light-tight to each other, so the lamp doesn't glow through the panel edge.

## Sensors worth adding (cheap, optional)

- **Ambient light sensor** (VEML7700 or LTR-303 class): auto-switch day/night faces and brightness. The CoreS3 has one, which could replace the fixed night schedule.
- **I2S MEMS mic:** for adaptive masking (above).
- **A physical button:** a big snooze/stop button on top of the case is nicer than finding the screen half-asleep, and works if the touch layer ever fails.

## Firmware impact

- `include/alarm_face.hpp` is pure logic (alarm state machine, layout, hit testing, formatting) and carries over as-is. The host tests in `test/` keep working.
- `src/main.cpp` talks to `M5.Display`, `M5.Touch`, `M5.Speaker`, and `M5.Rtc`. M5Unified only knows M5Stack boards, so the port is:
  - **Display:** LovyanGFX / M5GFX with a custom panel config if it supports the chosen AMOLED IC; otherwise Arduino_GFX, which has QSPI AMOLED drivers. Both expose a similar drawing API.
  - **Touch:** a small I2C driver for FT6336/CST8xx. These are simple register reads.
  - **Audio:** the ESP-IDF I2S driver feeding the noise generator and alarm sounds.
  - **RTC:** a small driver for the RV-3028.
- Suggested first refactor, before any hardware exists: put a thin `Board` interface between `main.cpp` and M5Unified (draw, touch, play, time, light), with a CoreS3 implementation. The custom board then becomes a second implementation, and the CoreS3 keeps working as a reference.
- 600×450×16 bit = 540 KB for a full framebuffer, which needs PSRAM. That's why the N16R8 module variant.

## How to measure flicker

- **Quick:** point a phone's slow-motion video (240 fps) or the camera's shutter at the screen or lamp. PWM shows up as rolling bands.
- **Proper:** a photodiode (BPW34) into a transimpedance amp or a 10 kΩ load, viewed on a scope. Look at percent modulation and frequency at every brightness step, especially the dimmest night settings.
- Test the screen at its night setting **before** committing to a panel.

## Suggested plan

1. **Bench prototype (no PCB):**
   - An ESP32-S3 AMOLED touch dev board (for example the 2.41″ above), a MAX98357A breakout, and the speaker in a cardboard or MDF test box.
   - The LED sink circuit on a breadboard.
   - Port the firmware, write the noise generator, and measure flicker on both screen and lamp.
2. **PCB v1: carrier board.**
   - Power, USB-C, audio, lamp driver, RTC, and sensors on your PCB.
   - The dev board plugs in as the brain + screen.
   - This dodges the riskiest part of a fully custom board: sourcing the bare AMOLED panel with a real datasheet and FPC pinout.
3. **PCB v2: fully custom.**
   - ESP32-S3-WROOM-1U + AMOLED panel FPC directly on your board, once you have the panel's datasheet and a reliable supplier.
4. **Case:** design around the v1 board's dimensions. Speaker chamber, lamp diffuser, and antenna placement first, cosmetics last.

## Open decisions

- Screen size: 2.41″ 600×450 (recommended) vs. smaller.
- Battery backup: RTC-only vs. LiPo so the alarm rings during outages.
- Lamp colour: ~625 nm vs. ~660 nm (or both on separate channels).
- Plain 5 V USB-C vs. adding USB-PD for a bigger amp.
