# Hardware

Plans for taking the CoreS3 alarm clock (V1, the firmware in this repo) to custom hardware: a high-end, all-metal bedside clock that could be sold.

| File | What's in it |
| --- | --- |
| [`DESIGN.md`](DESIGN.md) | Electronics: chip, screen, sound, red lamp, timekeeping, power, and the V1 → V4 plan |
| [`BOM.md`](BOM.md) | Bill of materials for the V2 breadboard and for production (~1k units), one-time costs, and design-for-manufacture notes |
| [`MANUFACTURING.md`](MANUFACTURING.md) | Enclosure construction, getting rid of plastics, finish, CNC design rules, uses for the 3D printer, assembly sequence, and how production scales |

## Requirements

- Sounds: white, pink and brown noise, plus ambient recordings such as a fire pit, at decent quality.
- A red lamp at 660 nm.
- A touch screen with true black (no backlight), readable from about 10 ft.
- Little or no flicker from the screen or the lamp.
- Powered from the wall through USB-C.
- No Wi-Fi and no "smart" features.
- Sellable as a high-end product: premium materials, cheap components, designed for manufacture.
- No plastic that you see or touch. Aluminium case, built in our own metal shop (CNC later), with the 3D printer used for prototyping.

## Decisions so far

| Decision | Choice | Why |
| --- | --- | --- |
| Masking sounds | Generated white/pink/brown noise + long ambient recordings | Masking is what works in a room. Generated noise never loops. |
| Wi-Fi | **None** (maybe V4) | Only needed for network time. Leaving it out also avoids radio certification. |
| Time setting | Manual, on the touch screen, with an accurate RTC (RV-3028) + drift trim + automatic DST | Keeps good time without a network |
| Keeping time through outages | Supercap on the RTC | No battery to ship or replace. A battery so the alarm rings during an outage is a V4 maybe. |
| Screen | ~4″ wide AMOLED (reference: 4.1″ 1232×568) | True black, and ~1.4″ digits readable at 10 ft |
| Chip | ESP32-P4 | Drives small and 4″ AMOLEDs, has no radio, and has enough memory for a full frame |
| Lamp | 660 nm LEDs, DC current drive | Deep red, zero flicker |
| Power | USB-C, 5 V / 3 A | Simple, and never touches mains |
| Enclosure | Aluminium square tube + CNC end caps + internal sled, anodized | Premium look, no tooling to start, suits a small shop |

## Versions

| Version | What it is |
| --- | --- |
| V1 | M5Stack CoreS3. Done; the firmware in this repo. |
| V2 | Breadboard: LilyGO T-Display-P4 (4.1″ AMOLED) plus modules for audio, lamp and RTC, in a 3D-printed mock-up case |
| V3 | Custom PCB in a shop-made aluminium case |
| V4 | Optional: Wi-Fi time sync, battery so the alarm rings during an outage |

## Still open

- Screen: 4.1″ wide (recommended) vs. a 4″ 4:3 panel.
- V3 PCB: a carrier board for the T-Display-P4, or fully custom.
- Tube size and proportions (3″ × 3″ is a starting guess).
- Lamp placement: downward wash, front glass strip, or top glow.
- Speaker direction: firing down or out the back.
- Finish: black or natural anodize.

## Next steps

1. **V1 firmware:**
   - "Set time" screen.
   - Dim by colour instead of screen brightness.
   - A `Board` layer so the app isn't tied to M5Stack's library.
2. **V2:**
   - Buy the breadboard list in `BOM.md`.
   - Measure flicker.
   - Check readability at 10 ft.
   - Pick the speaker.
3. **Quotes:** request them for the 4.1″ AMOLED with a custom cover glass, and speaker samples.
4. **Mock-ups:** 3D-print case mock-ups, then cut the first aluminium one.
