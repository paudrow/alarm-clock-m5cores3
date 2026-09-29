# Hardware

Plans for taking the CoreS3 alarm clock (V1, the firmware in this repo) to custom hardware: a high-end, all-metal bedside clock that could be sold.

| File | What's in it |
| --- | --- |
| [`DESIGN.md`](DESIGN.md) | Electronics: chip, screen, sound, red lamp, timekeeping, power, and the V1 → V4 plan |
| [`BOM.md`](BOM.md) | Bill of materials for the V2 breadboard and for production (~1k units), one-time costs, and design-for-manufacture notes |
| [`FIRST-BATCH.md`](FIRST-BATCH.md) | A lean plan for a first run of 10 at ~$200: display options, cost per unit, pricing and how not to go broke |
| [`CERTIFICATION.md`](CERTIFICATION.md) | What's needed to sell it legally in the US, Canada, EU and UK, what it costs, and how to pass first time |
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
| Screen | 4.1″ wide AMOLED, 1232×568 (as on the LilyGO T-Display-P4) | True black, and ~1.4″ digits readable at 10 ft |
| Chip | ESP32-P4, revision v3 (P4NRW32X) for the custom board | Drives small and 4″ AMOLEDs, has no radio, and has enough memory for a full frame |
| Lamp | 660 nm LEDs, DC current drive, shining down into a ½″ low-iron glass base | Deep red, zero flicker; the glass glows and washes the nightstand |
| Power | USB-C, 5 V / 3 A | Simple, and never touches mains |
| Enclosure | 3″ × 3″ aluminium square tube (130 mm) + CNC end caps + internal sled, **black anodize**; about 143 × 89 × 76 mm overall | Sized around the screen and a 2″ speaker; premium look, no tooling to start |
| Speaker | 2″ full-range (reference: Tectonic TEBM35C10-4), **firing out the back** | Clean front; a shallow driver fits the 3″ tube |

## Versions

| Version | What it is |
| --- | --- |
| V1 | M5Stack CoreS3. Done; the firmware in this repo. |
| V2 | Breadboard: LilyGO T-Display-P4 (4.1″ AMOLED) plus modules for audio, lamp and RTC, in a 3D-printed mock-up case |
| V3 | Custom PCB in a shop-made aluminium case |
| V4 | Optional: Wi-Fi time sync, battery so the alarm rings during an outage |

## Still open

- **Screen for the first batch:** lean 2.41″ AMOLED all-in-one (recommended for the first 10), the 4.1″ AMOLED, or big LED digits with a knob. See `FIRST-BATCH.md`. Test tonight: can you read the CoreS3 from 10 ft? The 2.41″ digits are ~20% bigger.
- V3 PCB: a carrier board for the T-Display-P4, or fully custom. Decide after V2.
- Speaker: confirm the Tectonic BMR sounds good in a ~0.2 L printed test box, against the Dayton ND65.
- Sandblast grit for the glass base.

## Next steps

1. **V1 firmware:**
   - "Set time" screen.
   - Dim by colour instead of screen brightness.
   - A `Board` layer so the app isn't tied to M5Stack's library.
2. **V2:**
   - Buy the breadboard list in `BOM.md` (section 1, about $270). Budgets for the later stages are in `BOM.md` section 4.
   - Measure flicker.
   - Check readability at 10 ft.
   - Pick the speaker.
3. **Quotes:** request them for the 4.1″ AMOLED with a custom cover glass, and speaker samples.
4. **Mock-ups:** 3D-print the 143 × 89 × 76 mm case, then cut the first aluminium one (materials in `BOM.md` section 2).
