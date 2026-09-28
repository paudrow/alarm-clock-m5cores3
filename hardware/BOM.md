# Bill of materials

Two lists:

1. **V2 breadboard:** what to buy now, at retail, quantity 1.
2. **Production:** the sellable version, estimated **per unit at ~1,000 units**.

Prices are rough planning numbers in USD, not quotes. The ESP32-P4 price is from LCSC; most others are typical distributor or China-OEM ranges. **The AMOLED panel is the biggest unknown and needs real quotes.** Replace every estimate with a quote before setting a retail price.

See `DESIGN.md` for why each part was chosen.

## 1. V2 breadboard (buy now)

| Qty | Part | Notes | ≈ Price |
| --- | --- | --- | --- |
| 1 | LilyGO T-Display-P4, **4.1″ AMOLED** version | Brain + screen + SD slot. Its radio is never enabled. | $120 |
| 1 | RV-3028-C7 RTC breakout | Accurate timekeeping | $10–15 |
| 1 | MAX98357A I2S amp breakout | | $6–8 |
| 1 | 2–2.5″ full-range speaker, 4 Ω | Buy two different ones and compare | $10–25 |
| 10 | 660 nm deep-red LEDs (3528/5050 or on a star board) | | $5–10 |
| 1 set | LED driver parts: rail-to-rail op-amp (MCP6001 / OPA333), 2× logic-level N-MOSFETs (AO3400 class), sense resistors (2 Ω, 200 Ω), RC-filter parts | Breadboard version of the DC lamp driver | $5 |
| 1 | Opal acrylic scrap | Lamp diffuser | $5 |
| 1 | VEML7700 light sensor breakout (optional) | | $5 |
| 1 | BPW34 photodiode | Flicker measurement | $2 |
| 1 | microSD card, 8–32 GB | Sound files | $5 |
| 1 | USB-C 5 V / 3 A power supply + cable | | $15 |
| — | Breadboard, jumpers, MDF/cardboard speaker test box, polyfill | | $20 |
| | | **Total** | **≈ $210–250** |

## 2. Production (per unit, ~1k volume, estimates)

### Main PCB

| Block | Part (example) | Qty | ≈ Each | ≈ Line | Notes |
| --- | --- | --- | --- | --- | --- |
| MCU | ESP32-P4NRW32 (32 MB PSRAM in package) | 1 | $4.60 | $4.60 | Chip-down. With no radio, no RF layout or radio certification is needed. |
| Flash | W25Q128JV, 16 MB QSPI NOR | 1 | $0.90 | $0.90 | Firmware |
| Sound storage | W25N01GV, 1 Gbit (128 MB) SPI NAND | 1 | $1.80 | $1.80 | Replaces the SD card. See DFM notes. |
| Clock | 40 MHz crystal | 1 | $0.15 | $0.15 | |
| Power | USB-C 16-pin mid-mount receptacle | 1 | $0.25 | $0.25 | SMT + through-hole anchor legs, for strength |
| | CC resistors, TVS, polyfuse, USBLC6-2 ESD | 1 set | $0.40 | $0.40 | |
| | 3.3 V buck + P4 core/LDO parts per Espressif reference design | 1 set | $0.90 | $0.90 | |
| | AMOLED supply rails (ELVDD/ELVSS), if the panel needs them externally | 1 set | $1.00 | $1.00 | Check the panel datasheet; many panels include this |
| Screen | FPC connectors (panel + touch) | 2 | $0.25 | $0.50 | |
| RTC | RV-3028-C7 | 1 | $2.00 | $2.00 | Single-source (Micro Crystal). DS3231 is the fallback footprint. |
| | Supercapacitor, 0.1–0.22 F, low leakage | 1 | $0.50 | $0.50 | Keeps time through outages. No battery to ship or replace. |
| Audio | MAX98357A | 1 | $1.50 | $1.50 | NS4168 (~$0.30) is a cost-down to A/B test by ear |
| | Speaker connector (JST-PH 2-pin) | 1 | $0.08 | $0.08 | |
| Lamp | 660 nm mid-power LEDs | 8 | $0.10 | $0.80 | On the main PCB, or a small LED board (see DFM notes) |
| | Op-amp, 2× MOSFET, sense resistors, RC set-point filter | 1 set | $0.45 | $0.45 | PWM + RC set-point, no DAC chip |
| Sensors / UI | Ambient light sensor (LTR-303 / VEML7700) | 1 | $0.60 | $0.60 | |
| | Physical snooze/stop button (tactile + metal cap) | 1 | $0.80 | $0.80 | |
| Misc | ~80 passives, test points | — | — | $0.80 | |
| PCB | 4-layer, impedance-controlled (for the DSI pairs), ~100 × 50 mm | 1 | $2.50 | $2.50 | |
| Assembly | SMT, single-sided, + AOI | 1 | $3.50 | $3.50 | |
| | | | | **≈ $24** | |

### Display

| Part | ≈ Each | Notes |
| --- | --- | --- |
| ~4.1″ AMOLED panel + capacitive touch + cover lens | **$15–35** | **Get quotes.** Ask the panel vendor for a custom black-printed cover lens ("dead front") so the panel edges disappear. |

### Enclosure (aluminium, no visible plastic)

Construction is tube + two end caps + an internal sled; see `MANUFACTURING.md`. Costs are for the small-production stage (stock tube, outsourced caps, batch anodizing). In your own shop, most of the cost is machine time instead.

| Part | ≈ Each | Notes |
| --- | --- | --- |
| Body: 6063 square tube (e.g. 3″ × 3″ × ⅛″), cut to ~150 mm, CNC'd for screen window, grille slots, lamp slot; bead-blast + anodize | $6–12 | Stock tube needs no tooling. A custom extrusion (one-time ~$500–2,000) only pays off after a few hundred units. |
| Two end caps, CNC 6061 | $4–10 | One carries the USB-C opening, one the button |
| Sled: bent 5052 sheet or machined plate, with speaker bulkhead | $2–4 | Holds PCB, speaker chamber and lamp; slides in as one tested unit |
| Glass retaining frame (aluminium) + thin gasket | $1–2 | Holds the cover glass against the window lip, no adhesive |
| Lamp diffuser: sandblasted or acid-etched glass, or none (indirect wash off the table) | $0–1 | |
| Speaker O-ring, wool felt damping, wool acoustic cloth behind grille | $0.80 | |
| M3 stainless screws, aluminium/brass standoffs, cork or wool felt feet | $0.80 | One screw size throughout |
| **Subtotal** | **≈ $15–30** | |

### Speaker

| Part | ≈ Each | Notes |
| --- | --- | --- |
| 2–2.5″ full-range driver, 4 Ω, OEM | $3–8 | **Spend here.** It matters more than any chip in the sound chain. Buy samples from several OEM vendors. |

### Box contents

| Part | ≈ Each | Notes |
| --- | --- | --- |
| Retail box + molded pulp insert | $2–4 | |
| USB-C to USB-C cable, 1.5–2 m, braided | $1.00 | |
| Certified 5 V / 3 A USB-C adapter (UL/CE, bought in) | $3–5 | Optional. Many premium products now ship without one. |
| Quick-start card | $0.20 | |
| **Subtotal** | **≈ $6–10** | |

### Per-unit total

| | Low | High |
| --- | --- | --- |
| Main PCB | $24 | $26 |
| Display | $15 | $35 |
| Enclosure | $15 | $30 |
| Speaker | $3 | $8 |
| Box contents | $6 | $10 |
| End-of-line test + programming | $0.50 | $1 |
| **Landed BOM** | **≈ $63** | **≈ $111** |

Hardware usually retails at 3–5× BOM, so this lands in high-end nightstand territory ($199–349) with healthy margin.

### One-time costs (rough)

| Item | ≈ Cost |
| --- | --- |
| Extrusion die (only at the small-production stage; stock tube until then) | $500–2,000 |
| CNC fixtures, soft jaws, mandrel for window machining (mostly 3D printed or shop-made) | $100–500 |
| Custom cover-lens print setup | $300–1,000 |
| EMC testing: FCC Part 15B + CE (EMC) + ICES-003. No radio, so no FCC ID, no RED. | $3,000–6,000 |
| Pogo-pin test fixture | $300–800 |
| Prototype runs (2–3 PCB spins, CNC case samples) | $1,500–3,000 |

## Design-for-manufacture notes

### Keep it cheap to certify

- **No radio saves the most money.** An unintentional radiator only needs FCC Part 15B verification (Supplier's Declaration of Conformity), not an FCC ID, and CE marking needs no Radio Equipment Directive assessment.
  - If V4 adds Wi-Fi, use a **pre-certified module** (ESP32-C6-MINI-1U) to inherit its approvals rather than certifying a chip-down radio.
- **Never touch mains.** USB-C input means the certified wall adapter carries all the mains safety. The product itself is a 5 V device.
- **No lithium battery.** A supercap on the RTC avoids battery-shipping rules (UN38.3, dangerous-goods paperwork) and a part that wears out. That's a good reason to keep "rings during an outage" optional even in V4.
- **Pre-compliance scan** before the real EMC test. The riskiest emitters are the DSI lines and the class-D amp's speaker wires: keep them short, twisted, and add ferrites on the speaker leads.

### Board

- **One board, parts on one side**, so there is only one SMT pass. The only off-board items are the screen (FPC), the speaker (JST) and the USB-C cable.
- **Lamp LEDs on a small second board** only if the case needs the light somewhere the main PCB can't reach. Panelize it with the main board so it's still one order, and connect it with a short FPC or JST.
- **Prefer JLCPCB/LCSC "basic" parts**, which avoid setup fees, and **give every part a second source** except the deliberate single-sources:
  - Panel: keep the firmware panel-agnostic (already the plan) and qualify a second panel before launch.
  - RTC: lay out a DS3231 alternative footprint.
  - MCU: Espressif is the only source for the P4. Buy a buffer.
- **Onboard SPI NAND instead of an SD card:**
  - Cheaper (no socket, no card).
  - Nothing to lose or come loose.
  - Can't be pulled out mid-play.
  - 128 MB holds several hours of ambient sound at 64–128 kbps.
  - Add or replace sounds over USB-C: the P4's USB can appear as a small drive.
  - Keep an SD footprint as do-not-populate for development.
- **Test points** on every rail, I2C, I2S, the lamp sense resistor and the USB pins, on a 2.54 mm-friendly grid for a pogo fixture.
- **Mounting:** the PCB sits on a sled screwed to one end cap, so the whole electronics assembly is tested outside the case and then slides into the tube as a unit (`MANUFACTURING.md`).

### Firmware for production

- **Factory self-test mode:**
  - Tone sweep through the speaker.
  - Lamp ramps through both current ranges, with the fixture reading the sense resistor.
  - Touch grid.
  - RTC tick check.
  - Flash/NAND checksum.

  The fixture records pass/fail against the unit's serial number.
- **Program over USB-C** at end of line. The serial number goes in eFuse or NVS.
- **Secure boot + flash encryption** (ESP32-P4 supports both) to make cloning harder. Decide before the first production run, because eFuses are one-way.
- **Field updates** over USB-C, via a simple desktop or browser updater (WebSerial / esptool-js), since there's no Wi-Fi.

### Enclosure

- **Tube + two end caps** gives a premium machined-aluminium look without paying for a full CNC body. Many high-end audio products are built this way. Start with stock square tube; move to a custom extrusion only once volume justifies the die. Details, scaling stages and CNC design rules are in `MANUFACTURING.md`.
- **Anodize after all machining.** Plan a bare-metal ground contact (masked spot or star washer) so the case can be bonded to circuit ground.
- **Screen:** hold the glass from inside against a lip in the screen window with a retaining frame and thin gasket, with no adhesive. Black-printed cover glass flush with the aluminium face looks high-end and hides the panel border.
- **Tolerances:** design for the tube's cut-length tolerance (±0.2–0.5 mm typical) with a compressible gasket at one end cap.

### Sound library

- **Commercial use needs clear rights.** Use CC0 recordings only (check each file's licence on freesound.org), or record your own. Your own recordings, like a real fire pit, are a nice product story.
- The generated white/pink/brown noise has no licensing question.

## Next steps

1. Buy the V2 list and validate screen flicker, readability at 10 ft, lamp driver and speaker choice.
2. Request quotes: 4.1″ AMOLED + touch + custom cover lens from 2–3 panel vendors (LilyGO's panel supplier and others), and speaker samples from 3–4 OEM vendors.
3. Draw the V3 schematic in KiCad under `hardware/`, then generate a real BOM CSV (with LCSC part numbers) from it to replace these estimates.
