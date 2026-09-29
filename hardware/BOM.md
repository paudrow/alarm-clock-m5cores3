# Bill of materials

Three buying lists, then the costs:

1. **V2 breadboard:** electronics to buy now, quantity 1.
2. **Enclosure prototype:** metal, glass, hardware, finishing and tooling for the first shop-built case.
3. **Production electronics:** exact parts with LCSC numbers for JLCPCB assembly.
4. **Prototype budget:** what each later stage (V3, V4, a pilot batch) is likely to cost.
5. **Production cost estimate:** per unit at ~1,000 units, then **unit cost at 10 / 100 / 1,000 / 10,000 units**, plus one-time costs.

**How far these are checked.** Every part number and product link below appeared in web-search results for that exact product page. The vendor sites themselves couldn't be opened from here.
- Prices are from search snippets, so treat them as close but **re-check price and stock in the cart**.
- Lines marked *unconfirmed* didn't show a price, or the SKU was inferred from neighbouring part numbers.
- JLCPCB's Basic/Extended status changes often; check it in their BOM tool.

See `DESIGN.md` for why each part was chosen and `MANUFACTURING.md` for how the case goes together.

## 1. V2 breadboard

### Brain and screen

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | LilyGO T-Display-P4, **4.1″ AMOLED** (1232×568, RM69A10 display driver, GT9895 touch) | [LilyGO](https://lilygo.cc/products/t-display-p4), [Amazon](https://www.amazon.com/dp/B0GTLC4D9H), [Rokland](https://store.rokland.com/products/lilygo-t-display-p4) | Amazon ASIN B0GTLC4D9H | $119.30 (LilyGO), $144.97 (Rokland) | Pick the **AMOLED** variant, US band (830–945 MHz). Every variant also has LoRa, GPS and a Wi-Fi chip; none of them get enabled. ameriDroid lists it at $119 but out of stock. |

### Timekeeping

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | Micro Crystal RV-3028-C7 evaluation board | [Digi-Key](https://www.digikey.com/en/products/detail/micro-crystal-ag/RV-3028-C7-EVALUATION-BOARD/10431075) | 2195-RV-3028-C7-EVALUATION-BOARD-ND | $17.47 | The same RTC chip as production. The T-Display-P4 has a less accurate PCF8563 on board (and LilyGO's v2.0 board drops it). |
| (alt) | TexDuino RV-3028-C7 breakout (Qwiic, CR1025 holder) | [Tindie](https://www.tindie.com/products/texduino/rv-3028-c7-real-time-clock-breakout-board/) | — | $14.99 | Plugs straight into the board's Qwiic port |
| (alt) | Pimoroni RV3028 breakout | [PiShop.us](https://www.pishop.us/product/rv3028-real-time-clock-rtc-breakout/) | PIM449 | *unconfirmed* | |

### Sound

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | Adafruit MAX98357A I2S amp breakout | [Adafruit](https://www.adafruit.com/product/3006) | 3006 | $5.95 | 3.2 W into 4 Ω at 5 V. The same chip as production. |
| 1 | **Tectonic TEBM35C10-4**, 2″ BMR full-range, 4 Ω | [Parts Express](https://www.parts-express.com/Tectonic-TEBM35C10-4-BMR-2-Full-Range-Speaker-4-Ohm-297-216) | 297-216 | $9.98 | **Reference driver for the case.** Shallow (~1″ deep), very wide dispersion, made for small boxes. |
| 1 | Dayton Audio ND65-4, 2.5″ aluminium cone, 4 Ω | [Parts Express](https://www.parts-express.com/Dayton-Audio-ND65-4-2-1-2-Aluminum-Cone-Full-Range-Driver-290-204) | 290-204 | $22.98 | For a listening comparison. Probably too big for the back wall of a 3″ tube (see `MANUFACTURING.md`). |
| (alt) | Dayton Audio DMA58-4, 2″ dual-magnet, 4 Ω | [Parts Express](https://www.parts-express.com/Dayton-Audio-DMA58-4-2-Dual-Magnet-Aluminum-Cone-Full-Range-Driver-4-Ohm-295-582) | 295-582 | *unconfirmed* (~$14) | Another 2″ to compare |
| 1 | Samsung PRO Endurance 32 GB microSD | [Amazon](https://www.amazon.com/dp/B09WB35BXS) | MB-MJ32KA/AM | ~$12 | Sound files. SanDisk High Endurance 32 GB (ASIN B07P14QHB7) also fine. |

### Red lamp (660 nm, DC driver)

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 2 | LUXEON Rebel deep red 655 nm on a 20 mm star board | [Luxeon Star](https://luxeonstar.com/product/sp-01-d2/) | SP-01-D2 | $9.37 | Easiest to breadboard: solder wires to the pads. Built to order, so allow extra time. |
| 5 | ams OSRAM OSCONIQ P 3030 Hyper Red, 660 nm peak | [Digi-Key](https://www.digikey.com/en/products/detail/ams-osram-usa-inc/GH-QSSPA1-24-4T2U-1-FJ-350-R18/10483574) | GH QSSPA1.24-4T2U-1-FJ-350-R18 | $1.73 | **Production candidate.** SMD, so reflow or hot-air it onto a small board. 660 nm is the *peak*; the colour your eye reads (dominant wavelength) is ~640 nm, which is normal for "660 nm" LEDs. |
| 2 | Microchip MCP6022-I/P, dual rail-to-rail op-amp, DIP-8 | [Digi-Key](https://www.digikey.com/en/products/detail/microchip-technology/MCP6022-I-P/417828) | MCP6022-I/P | $1.86 | ±0.5 mV offset, which matters on the low-current range |
| 2 | Infineon IRL540NPBF logic-level N-MOSFET, TO-220 | [Digi-Key](https://www.digikey.com/en/products/detail/infineon-technologies/IRL540NPBF/812000) | IRL540NPBF | $2.39 | High-current range. TO-220 handles ~0.7 W in linear mode. |
| 2 | Microchip TN0702N3-G logic-level N-MOSFET, TO-92 | [Digi-Key](https://www.digikey.com/en/products/detail/microchip-technology/TN0702N3-G/4902376) | TN0702N3-G-ND | $1.52 | Range switch |
| 5 | Stackpole RSMF1JB2R00, 2 Ω 1 W | [Digi-Key](https://www.digikey.com/en/products/detail/stackpole-electronics-inc/RSMF1JB2R00/1686644) | RSMF1JB2R00 | *unconfirmed* (<$1) | High-range sense resistor |
| 10 | Stackpole RNF14FTD200R, 200 Ω 1% ¼ W | [Digi-Key](https://www.digikey.com/en/products/detail/stackpole-electronics-inc/RNF14FTD200R/1706701) | RNF14FTD200R | $0.10 | Low-range sense resistor |

Also needed: a few 10 kΩ resistors and 1 µF capacitors for the RC set-point filter, from any assortment.

### Sensing and measurement

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | Adafruit VEML7700 light sensor, STEMMA QT | [Adafruit](https://www.adafruit.com/product/4162) | 4162 | $4.95 | Auto day/night |
| 2 | Vishay BPW34 photodiode | [Digi-Key](https://www.digikey.com/en/products/detail/vishay-semiconductor-opto-division/BPW34/1681149) | 751-1015-ND | $1.37 | Flicker measurement with a scope |

### Power and prototyping

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | Raspberry Pi 27 W USB-C power supply (US) | [Adafruit](https://www.adafruit.com/product/5814) | 5814 (Raspberry Pi SC1153) | $14.04 | Fixed cable. The 15 W version (Adafruit 4298, $8.74) also works but was showing out of stock. |
| 1 | USB A to C cable, 1 m | [Adafruit](https://www.adafruit.com/product/4474) | 4474 | $4.95 | Flashing from a computer. C to C: Adafruit 4199, $9.95. |
| 1 | Full-size breadboard, 830 points | [Adafruit](https://www.adafruit.com/product/239) | 239 | $5.95 | |
| 1 | Male-male jumper wires, 40 × 6″ | [Adafruit](https://www.adafruit.com/product/758) | 758 | $3.95 | |
| 4 | STEMMA QT / Qwiic to male header cable, 150 mm | [Adafruit](https://www.adafruit.com/product/4209) | 4209 | $0.95 | For the T-Display-P4's Qwiic ports |

**V2 total: about $270** before shipping (six vendors: LilyGO or Amazon, Digi-Key, Adafruit, Parts Express, Luxeon Star, Amazon).

## 2. Enclosure prototype (shop build)

Sized in `MANUFACTURING.md`: a 3″ × 3″ tube, 130 mm long, two end caps, and a ½″ low-iron glass base. Overall about 143 × 93 × 76 mm (W × H × D) including feet. Quantities below build **two** cases, because the first is always a learning piece.

### Metal

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 12″ | 6063-T52 aluminium square tube, 3″ × 3″ × ⅛″ wall | [Speedy Metals](https://www.speedymetals.com/pc-4685-8379-3-sq-wall-sq-tube-6063-t52-aluminum.aspx) | pc-4685 | $1.53/in (~$18) | Cut to the inch. 6063 anodizes the most evenly. Alt: [OnlineMetals pid 4602](https://www.onlinemetals.com/en/buy/aluminum/3-od-x-0-125-wall-aluminum-square-tube-6063-t52-extruded/pid/4602). |
| 12″ | 6061-T6511 flat bar, ¼″ × 3″ | [Speedy Metals](https://www.speedymetals.com/p-2249-14-x-3-6061-t6511-aluminum-extruded.aspx) | p-2249 | *unconfirmed* | End caps. 3″ wide matches the tube, so each cap is just an 89 mm cut. Alt: [OnlineMetals ¼″ 6061-T651 plate, pid 1248](https://www.onlinemetals.com/en/buy/aluminum/0-25-aluminum-plate-6061-t651/pid/1248), from $12.88. |
| 12″ × 12″ | 5052-H32 aluminium sheet, 0.063″ | [OnlineMetals](https://www.onlinemetals.com/en/buy/aluminum/0-063-aluminum-sheet-5052-h32/pid/7128) | pid 7128 | *unconfirmed* | Internal sled. 5052 bends without cracking. Alt: [Speedy Metals p-1939](https://www.speedymetals.com/p-1939-0063-5052-h32-aluminum-sheet.aspx). |

### Glass base (lamp)

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 2 | Low-iron glass, ½″ thick, **132 × 76 mm (5.20″ × 3.00″)**, **flat polished edges**, not tempered | [Dulles Glass](https://www.dullesglass.com/low-iron-glass) (custom size form) | — | quote at checkout | Low-iron so the edges glow clean red rather than green. Sandblast the bottom face in the shop. Alt: [Behrenberg Glass](https://www.behrenbergglass.com/Flat-Clear-4-MM-532-Low-Iron-Glass_c_221.html). A local glass shop can also cut and polish this. |

### Hardware

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 25 | M3 × 8 mm socket cap screw, 18-8 stainless | [Bolt Depot](https://boltdepot.com/Product-Details?product=6379) | 6379 | *unconfirmed* | Internal |
| 25 | M3 × 6 mm button head screw, 18-8 stainless | [Bolt Depot](https://boltdepot.com/Product-Details?product=7218) | 7218 | *unconfirmed* | Visible screws on the end caps |
| 1 | E-Z Coil thread-insert kit, M3-0.5 × 1D (3 mm long) | [E-Z LOK](https://www.ezlok.com/ezcoil-kit-SK40210) | SK40210 | *unconfirmed* (~$30) | 1D inserts fit the ⅛″ tube wall. Includes drill, tap and tools. |
| 1 | Recoil M3-0.5 insert kit (1.5D) | [Bay Supply](https://www.baysupply.com/products/35038-recoil) | 35038 | *unconfirmed* | End caps only (¼″ thick) |
| 8 | RAF M3 × 10 mm aluminium hex standoff, male-female | [Digi-Key](https://www.digikey.com/en/products/detail/raf-electronic-hardware/M2105-3005-AL/7681378) | M2105-3005-AL | $0.52 | PCB to sled |

### Acoustic and finishing materials (plastic-free)

| Qty | Part | Where | SKU / part # | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- |
| 1 | F-1 wool felt, 1/16″, black | [The Felt Company](https://www.thefeltcompany.com/f-1-sae-wool-felt-1-16-thick-x-60-wide-black-industrial-wool-felt/) | — | *unconfirmed* | Behind the speaker grille, glass-to-metal pads, glass gasket. Alt: [The Felt Store F-1 by the foot](https://thefeltstore.com/products/f-1-industrial-felt-by-foot). |
| 1 | Lambswool speaker stuffing, 500 g | [KJF Audio](https://kjfaudio.com/product/lambs-wool-speaker-stuffing-500g/) | — | $16 | Real wool. Keep the chamber sealed, since moths like wool. |
| 1 ft | Buna-N O-ring cord, 2.0 mm | [The O-Ring Store](https://www.theoringstore.com/store/index.php?main_page=product_info&products_id=18589) | N70.079 | *unconfirmed* | Speaker chamber seal. Cut to length and join with cyanoacrylate. |
| 1 | Self-adhesive cork sheet, 12″ × 24″ × 1/16″ | [Rockler](https://www.rockler.com/pressure-sensitive-cork-sheets) | — | *unconfirmed* | Cut feet to fit the end caps. Or [Rockler cork dots](https://www.rockler.com/self-adhesive-cork-dots). |
| 1 | Dynamat Xtreme Speaker Kit, 2 × 10″ × 10″ | [Dynamat](https://dynamat.com/products/dynamat-xtreme-speaker-kit) | 10415 | $25.95 | Stops the tube ringing. Butyl rubber with an aluminium layer, inside only. Cheaper: Noico 80 mil (ASIN B01KZ5X7KO, ~$18). |

### Anodizing (black, Type II)

| Option | Where | ≈ Price | Notes |
| --- | --- | --- | --- |
| Mail-in, no strict minimum | [Diamond Metal Finishing](https://www.diamondmf.com/small-parts-anodizing/) (Houston) | quote | Type II, black available, 3–5 business days |
| Mail-in | [DMF Anodizing](https://dmfanodizing.com/) | $125 lot minimum | Put both prototype cases in one lot |
| Mail-in | [Quick Turn Anodizing](https://www.quickturnanodizing.net/) (IN / TN) | quote (~$65 lot minimum cited) | |
| Local | Search "Type II sulfuric anodize" + your city | typically $65–150 lot | Often the cheapest and fastest |
| DIY, later | [Caswell anodizing kits](https://caswellplating.com/deluxe-anodizing-kit.html) | ~$660+ | Only once you're producing steadily |

When you send parts:
- Ask for black Type II, sealed.
- Anodize the tube and caps **in the same batch**. 6063 and 6061 can come out slightly different shades.
- Mask the thread inserts and the ground-contact spot.

### Tooling (for the CNC)

| Part | Where | SKU / part # | Notes |
| --- | --- | --- | --- |
| ¼″ 3-flute carbide end mill, ZrN coated, 0.5″ flute length | [Maritool](https://www.maritool.com/p12766/1/4-3-Flute-Carbide-End-Mill-SE-38-Deg-Helix-.500-loc-ZrN-Coated/product_info.html) | p12766 | Main roughing and profiling. Short flutes suit a small machine. |
| 6 mm 3-flute carbide end mill | [Maritool](https://www.maritool.com/Cutting-Tools-End-Mills-Finishers-Square-End-Aluminum-Specific-End-Mills-Square-End-3-Flute/c78_79_80_201_202/p2102/.2362-(6mm)-3-Flute-Carbide-End-Mill-SE-38-Deg-Helix-19mm-LOC/product_info.html) | p2102 | Metric alternative |
| ⅛″ 3-flute carbide end mill, ZrN | [Maritool](https://www.maritool.com/Cutting-Tools-End-Mills-Finishers-Square-End-Aluminum-Specific-End-Mills-Square-End-3-Flute/c78_79_80_201_202/p13495/1/8-3-Flute-Carbide-End-Mill-SE-38-Deg-Helix-.375-loc-ZrN-Coated/product_info.html) | p13495 | Grille slots, window corners |
| 3 mm single-flute (Datron), 5-pack | [Penta Machine](https://www.pentamachine.com/all-products/p/datron-single-flute-end-mill-3mm-dia-18-in-shank) | — | Fine slots on a light machine |
| ¼″ 90° chamfer mill, 4-flute | [Maritool](https://www.maritool.com/Cutting-Tools-End-Mills-Chamfer-Mills-Long-Length-Chamfer-Mills/c78_79_154_427/p17650/Chamfer-Mill-.250-Dia-X-90-Degree-x-3.0-OAL-4-Flute-DE/product_info.html) | p17650 | The last pass on every edge |

**Enclosure prototype total** (two cases, excluding tooling and glass): roughly **$150–250**, most of it the Dynamat kit, thread-insert kits, felt and the anodizing lot charge. The metal itself is about $50.

## 3. Production electronics (exact parts)

For the custom PCB (V3 route (b), or the parts of route (a) that are on your carrier board). LCSC numbers are for JLCPCB assembly.

| Function | Part (MPN) | Maker | Package | LCSC # | Second source | ≈ Price | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| MCU | **ESP32-P4NRW32X** | Espressif | QFN-104 10 × 10 | C54540373 | — | ~$4.6 | **Chip revision v3.x. Use this, not ESP32-P4NRW32 (C22387510), which is the older v1.x and not for new designs.** v3 needs PCB changes (see `DESIGN.md`). 32 MB PSRAM in package, no flash. |
| P4 core supply (VDD_HP, 0.99–1.3 V) | SY8088AAC | Silergy | SOT-23-5 | C79313 | TLV62569DBVR (C141836, Digi-Key) | $0.06 | On Espressif's verified list; the P4 sets its voltage |
| 3.3 V rail, 2 A | SY8089AAAC | Silergy | SOT-23-5 | C78988 | — | $0.08 | Main rail |
| Panel VBAT rail (~3.8–4.0 V) | SY8089AAAC, set by feedback resistors | Silergy | SOT-23-5 | C78988 | — | $0.08 | The 4.1″ panel's FPC takes a battery-level VBAT supply (T-Display-P4 schematic). **Confirm the range in the panel datasheet.** |
| Firmware flash, 16 MB | W25Q128JVSIQ | Winbond | SOIC-8 | C97521 | Digi-Key/Mouser same MPN | $1.64 | 3.3 V flash works with the P4 as-is |
| Sound storage, 128 MB SPI NAND | W25N01GVZEIG | Winbond | WSON-8 6 × 8 | C88868 or C907680 | Digi-Key/Mouser same MPN | $3.41 | Same part, two LCSC listings; use whichever is in stock |
| 40 MHz crystal, 12 pF, ±10 ppm | 3225 40M 12PF 10PPM | SOSET | 3225 | C5380316 | YXC X322540MOB4SI | $0.04 | Tune load caps on the first boards |
| USB-C receptacle, 16-pin | TYPE-C-31-M-12 | HRO | SMD + THT shell legs | C165948 | GCT USB4105-GF-A (C3020560) | $0.10 | |
| CC pull-downs (×2) | 0402WGF5101TCE, 5.1 kΩ 1% | UNI-ROYAL | 0402 | C25905 (Basic) | any | <$0.01 | |
| USB ESD | USBLC6-2SC6 | ST | SOT-23-6 | C7519 | Mouser same MPN | $0.09 | |
| VBUS TVS | SMF5.0A | Littelfuse | SOD-123FL | C151296 | SMAJ5.0A-13-F (C87074) | ~$0.05 | |
| VBUS fuse, 2 A hold | MINISMDC200F-2 | Littelfuse | 1812 | C720175 | Digi-Key same MPN | ~$0.15 | |
| RTC, ±1 ppm | RV-3028-C7 32.768kHz 1ppm TA QC | Micro Crystal | C7 (3.2 × 1.5) | C3019759 | Digi-Key 10431070 | ~$1.5 | Has a built-in trickle charger for the supercap |
| RTC fallback | DS3231SN#T&R | ADI/Maxim | SOIC-16W | C9866 | Digi-Key same MPN | $3.71 | Alternative footprint on the PCB |
| RTC backup, 0.1 F 5.5 V | SE-5R5-D104VY | KAMCAP | coin, through-hole | C150567 (*partly confirmed*) | Eaton KR-5R5V104-R (Digi-Key 1556240) | ~$0.4 | The one through-hole part; I found no SMD 0.1 F on LCSC |
| Audio amp | MAX98357AETE+T | ADI/Maxim | TQFN-16 3 × 3 | C910544 | Digi-Key/Mouser same MPN | $0.82 | NS4168 (C910588, $0.38) is a cost-down, but **not pin-compatible** |
| Speaker connector | S2B-PH-SM4-TB(LF)(SN) | JST | PH 2-pin RA SMD | C295747 | Digi-Key 455-1749-1-ND | $0.12 | |
| Lamp op-amp | MCP6001T-I/OT | Microchip | SOT-23-5 | C116490 | TLV9001IDBVR (C398363) | $0.14 | |
| Lamp MOSFETs (×2) | AO3400A | AOS | SOT-23 | C20917 (Basic) | Digi-Key same MPN | $0.05 | Check the full-current MOSFET's heat on the board; a larger package may be needed |
| Lamp LEDs (×4), 660 nm | GH QSSPA1.24-4T2U-1-FJ-350-R18 | ams OSRAM | 3030 | **not on LCSC** | Luminus SST-10-DR-B130-K660 (3535, Digi-Key 15903659) | ~$0.6–1.7 | Order via JLCPCB global sourcing or consign them. Look for a Chinese 660 nm 3030 "horticulture" LED later as a cost-down. |
| Ambient light sensor | LTR-303ALS-01 | Lite-On | 2 × 2 | C364577 | VEML7700-TT (C1850416) | ~$0.4 | |
| Snooze/stop switch | TS-1187A-B-A-B | XKB | SMD 5.1 × 5.1 | C318884 (Basic) | — | $0.02 | Under a machined aluminium button cap |
| Display FPC | **placeholder**: 0.5 mm, pin count per panel | — | — | e.g. C19273956 (40-pin) | — | ~$0.07 | The T-Display-P4 panel uses a **31-pin** MIPI FPC. Pick the exact connector from the panel's drawing. |
| Touch FPC | **placeholder**: 0.5 mm 6-pin ZIF | JUSHUO | — | C262655 | — | ~$0.05 | Only if touch is on a separate FPC (on the T-Display-P4 it shares the 31-pin one) |

## 4. Prototype budget (V2 → first sales)

Estimates for **cash spent**, not counting your time or the CNC mill itself. They assume JLCPCB-style prototype pricing and roughly two builds per stage (one to learn from, one that works).

### V2: breadboard (from sections 1–2)

| Item | ≈ Cost |
| --- | --- |
| Electronics (section 1) | $270 |
| Printed mock-up cases, filament | $10–20 |
| Shipping from ~6 vendors | $40–60 |
| **Total** | **≈ $320–350** |

### V3 route (a): carrier board, with the T-Display-P4 inside

This is the cheapest way to a working clock in a real aluminium case.

| Item | ≈ Cost | Notes |
| --- | --- | --- |
| One more T-Display-P4 (the second unit; the V2 one becomes the first) | $120 | |
| Carrier PCB: 2-layer, 5 boards assembled, **× 2 spins** | $150–300 | ~$60–120 per spin. It has power, amp, lamp driver and RTC, and no high-speed lines. |
| Speakers, LEDs, RTC, supercaps for 2 units | $40–60 | |
| Enclosure for 2 cases: metal, glass, hardware, wool, cork, Dynamat (section 2) | $250–400 | The glass is the least certain line: ~$30–80 a piece in ones and twos |
| Black anodizing, one lot | $65–150 | Put both cases in the same lot |
| CNC tooling (section 2), one-time | $150–300 | |
| Shipping and extras | $100 | |
| **Total for 2 working clocks** | **≈ $900–1,400** | |

### V3 route (b): fully custom PCB (ESP32-P4 + bare panel)

| Item | ≈ Cost | Notes |
| --- | --- | --- |
| 4-layer impedance-controlled PCB, 5 boards assembled, **× 2–3 spins** | $400–900 | ~$150–300 per spin. The P4's 104-pin QFN and the DSI routing mean the first spin rarely works completely. |
| Bare 4.1″ AMOLED panels + touch, 3–5 samples | $100–300 | Depends on finding a supplier with a datasheet. The fallback is a panel from a T-Display-P4 ($120). |
| Debug gear, if you don't have it: hot-air station, USB logic analyser | $100–250 | |
| Enclosure, anodizing, tooling, speakers, as route (a) | $500–850 | |
| Shipping and extras | $150 | |
| **Total for 2–3 working clocks** | **≈ $1,250–2,450** | |

### V4: Wi-Fi and/or battery (optional)

| Item | ≈ Cost |
| --- | --- |
| One more board spin with an ESP32-C6-MINI-1U module, antenna, LiPo and charger | $250–500 |
| Case changes (antenna window or bulkhead connector) | $50–150 |
| **Total** | **≈ $300–650** |

### Pilot batch: 10–20 clocks to sell or give to beta testers

| Item | ≈ Cost | Notes |
| --- | --- | --- |
| Per clock: assembled PCB ~$25–40, panel ~$25–40, glass ~$20–40, speaker ~$9, aluminium and hardware ~$15, anodizing share ~$8, packaging ~$5 | **$110–160 each** | Small-quantity prices; they fall towards the section 5 figures with volume |
| 15 clocks | $1,650–2,400 | |
| Test fixture and jigs (mostly printed) | $200–500 | |
| EMC pre-compliance scan at a lab (a few hours) | $500–1,500 | Catches problems before the real test |
| FCC Part 15B + ICES-003 (US + Canada) | $1,500–3,000 | Needed before selling; see `CERTIFICATION.md`. EU/UK comes later. |
| **Total** | **≈ $3,850–7,400** | |

### Running total to a first sellable batch

| Path | ≈ Cash |
| --- | --- |
| V2 → V3 (a) carrier → pilot of 15 | **$5,000–9,000** |
| V2 → V3 (a) → V3 (b) custom → pilot of 15 | **$6,300–11,600** |

Certification and the pilot batch are most of it. **The prototypes themselves are about $1,200–1,750** (V2 + carrier V3), or up to ~$4,200 if you also build the custom board.

Ways to keep it down:
- Do route (a) first. It proves the case, sound and lamp for under $1,500 before committing to the harder custom board.
- Order 5 boards per spin, and panelize the small lamp or carrier boards with the main board.
- Use JLCPCB Basic parts where possible, since each Extended part adds a setup fee per order.
- Batch the anodizing: finish several iterations' parts in one lot.
- Buy glass bases in tens from a local glass shop once the size is fixed.

## 5. Production cost estimate

Per unit at ~1,000 units. These are estimates; replace each with a quote before setting a price.

### Main PCB

| Item | ≈ Each | Notes |
| --- | --- | --- |
| Parts in section 3 | $15–17 | P4 ~$4.6, NAND ~$3.4, flash ~$1.6, RTC ~$1.5, LEDs ~$2.4, amp ~$0.8, the rest small |
| ~80 passives, test points | $0.80 | |
| PCB: 4-layer, impedance-controlled (for the DSI pairs), ~100 × 50 mm | $2.50 | |
| SMT assembly, single-sided, + AOI | $3.50 | Plus hand-fitting the one through-hole supercap |
| **Subtotal** | **≈ $22–24** | |

### Display

| Part | ≈ Each | Notes |
| --- | --- | --- |
| ~4.1″ AMOLED panel + capacitive touch + cover lens | **$15–35** | **Get quotes.** Ask the panel vendor for a custom black-printed cover lens ("dead front") so the panel edges disappear. |

### Enclosure (aluminium, no visible plastic)

Construction is tube + two end caps + an internal sled; see `MANUFACTURING.md`. Costs are for the small-production stage (stock tube, outsourced caps, batch anodizing). In your own shop, most of the cost is machine time instead.

| Part | ≈ Each | Notes |
| --- | --- | --- |
| Body: 6063 square tube, 3″ × 3″ × ⅛″, cut to 130 mm, CNC'd for screen window, rear grille slots, lamp slot; bead-blast + black anodize | $6–12 | Stock tube needs no tooling. A custom extrusion (one-time ~$500–2,000) only pays off after a few hundred units. |
| Two end caps, CNC 6061 | $4–10 | One carries the USB-C opening, one the button |
| Sled: bent 5052 sheet or machined plate, with speaker bulkhead | $2–4 | Holds PCB, speaker chamber and lamp; slides in as one tested unit |
| Glass retaining frame (aluminium) + thin gasket | $1–2 | Holds the cover glass against the window lip, no adhesive |
| Glass base: ½″ low-iron glass, 132 × 76 mm, polished edges, sandblasted underside | $4–8 | |
| Speaker O-ring, wool felt damping, wool acoustic cloth behind grille | $0.80 | |
| M3 stainless screws, aluminium/brass standoffs, cork or wool felt feet | $0.80 | One screw size throughout |
| **Subtotal** | **≈ $19–37** | |

### Speaker

| Part | ≈ Each | Notes |
| --- | --- | --- |
| 2″ full-range driver, 4 Ω (reference: Tectonic TEBM35C10-4 BMR, $8.70 at retail in 4+) | $3–8 | **Spend here.** It matters more than any chip in the sound chain. Get volume pricing from Tectonic, and OEM samples to compare. |

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
| Main PCB | $22 | $24 |
| Display | $15 | $35 |
| Enclosure | $19 | $37 |
| Speaker | $3 | $8 |
| Box contents | $6 | $10 |
| End-of-line test + programming | $0.50 | $1 |
| **Landed BOM** | **≈ $65** | **≈ $116** |

Hardware usually retails at 3–5× BOM, so this lands in high-end nightstand territory ($199–349) with healthy margin.

### Unit cost by volume

The same design at four volumes. How it gets made changes along the way, and that drives most of the drop:

| | 10 | 100 | 1,000 | 10,000 |
| --- | --- | --- | --- | --- |
| **How it's made** | Your shop; PCBs assembled by JLCPCB; glass cut singly | Your CNC with fixtures; JLCPCB; glass bought in batches | Case parts machined by a job shop; panels bought in bulk | Custom extrusion, cast or forged caps; a contract manufacturer (CM) builds it |
| Main PCB, assembled | $40–55 | $25–30 | $20–24 | $15–18 |
| AMOLED panel + touch + cover glass | $40–60 | $30–45 | $15–35 | $12–25 |
| Enclosure (materials, glass base, anodizing, hardware) | $70–100 | $30–40 | $19–37 | $12–20 |
| Speaker | $10 | $7–9 | $4–7 | $3–5 |
| Box, cable, insert | $10–20 | $8–12 | $6–10 | $4–7 |
| Freight in, import duties | $10–15 | $5–10 | $4–8 | $3–6 |
| Final assembly + test | your time | your time | $10–15 (hired help) | $4–6 (CM) |
| Scrap allowance (3–5%) | $5–10 | $4–6 | $3–5 | $2–3 |
| **Cash cost per unit** | **$185–270** | **$110–150** | **$80–140** | **$55–90** |
| Your shop time per unit | ~2.5 h | ~1.25 h | — | — |
| **Including your time at $40/h** | **$285–370** | **$160–200** | **$80–140** | **$55–90** |
| One-time costs spread per unit (certification, fixtures, cover-glass setup; at 10k also the extrusion die and cap tooling) | ~$300–450 | ~$30–45 | ~$5 | ~$1–2 |

Notes:
- **The panel is the biggest unknown** at every volume. Get real quotes early, since they move these numbers more than anything else.
- **At 10 units,** building on the T-Display-P4 (carrier route) instead of the custom board costs about **$230–300 cash** per unit. That's still a sensible way to make the first 10.
- **Import duties** on Chinese-made boards and panels have changed repeatedly since 2025. Check the current rate before pricing; it can move the 1,000 and 10,000 figures by 10–30%.
- **What it means for price:**
  - Selling direct from your own site, you want roughly **2× cost** or more. At ~$299 that works from about 100 units.
  - Through retailers, who take 40–50%, you want **4–5× cost**. That only works from ~1,000 units up.

### One-time costs (rough)

| Item | ≈ Cost |
| --- | --- |
| Extrusion die (only at the small-production stage; stock tube until then) | $500–2,000 |
| CNC fixtures, soft jaws, mandrel for window machining (mostly 3D printed or shop-made) | $100–500 |
| Custom cover-lens print setup | $300–1,000 |
| Certification: FCC Part 15B + ICES-003 first ($1,500–3,000), EU/UK later ($5,000–10,000). See `CERTIFICATION.md`. | $1,500–13,000 |
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
