# Manufacturing plan

How to build the enclosure and assemble the clock, from first prototypes in a home shop to small-batch sales and beyond. Parts and costs are in `BOM.md`; the electronics are in `DESIGN.md`.

Goals:

- **No plastic that you see or touch.** Aluminium body, glass, and natural materials elsewhere. The electronics inside (PCB, connectors, the display's own layers) will always contain some polymers.
- **Prototype fast** with the 3D printer, then move to metal.
- **Build in your own shop:** metal shop now, a CNC mill later.
- **Keep a clear path to outsourcing** if volume grows.

## Recommended construction: tube + two end caps + internal sled, on a glass base

Decided so far:
- **Black anodize.**
- **Speaker fires out the back.**
- **The lamp shines down into a glass base** that the clock stands on.

```
   FRONT                                               SIDE (section)
      ┌──┬───────────────────────────────────┬──┐         ┌──┬──────────────┬──┐
      │  │   ┌─────────────────────────┐     │  │         │  │ scr │  ░░spk  │  │
      │  │   │   screen window (96×46) │     │  │         │  │ een │ chamber  │  │ ← rear grille
      │  │   └─────────────────────────┘     │  │         │  │     │ ░░       │  │   slots
      │  │                                   │  │         │  │ ▓▓▓▓ PCB + LEDs▓│  │
      │  ├───────────────────────────────────┤  │         │  ├───▼▼▼ lamp slot ─┤  │
      │  │▒▒▒▒▒▒▒ ½″ low-iron glass ▒▒▒▒▒▒▒▒▒│  │         │  │▒▒▒▒▒ glass ▒▒▒▒▒│  │
      └──┴───────────────────────────────────┴──┘         └──┴────────────────┴──┘
    end cap                                 end cap      cork feet under the caps;
                                                         the glass floats ~1.5 mm above the table
```

### Sizing (from the components)

| Component | Size | Source |
| --- | --- | --- |
| Screen with cover glass (T-Display-P4, 4.1″ AMOLED) | ~111 × 61 mm face; ~18 mm deep including the board behind it (22 mm in LilyGO's own shell) | LilyGO's 3D model, `structure/H753.stp` in the [T-Display-P4 repo](https://github.com/Xinyuan-LilyGO/T-Display-P4) |
| Visible screen area | ~94.5 × 43.6 mm | 4.1″ diagonal at 1232 × 568 |
| Speaker (Tectonic TEBM35C10-4, 2″ BMR) | 2″ class, ~1″ deep | **Confirm the frame size on the Parts Express spec sheet before drawing the back wall** |
| Glass base | ½″ (12.7 mm) thick | |

Fitting these into a **3″ × 3″ × ⅛″ tube** (69.9 mm inside):

- **Height:** the 61 mm screen fits the 69.9 mm inside with ~4 mm to spare top and bottom. From outside, a 96 × 46 mm window leaves an even ~15 mm black border above and below the digits.
- **Depth, front to back (69.9 mm inside):**
  - ~18 mm for the screen and its board;
  - a sealed speaker chamber about 35 mm deep against the back wall;
  - ~15 mm left over for wiring and the chamber bulkhead.

  The chamber is 35 mm deep × ~60 mm tall × ~120 mm long, about 0.2 L after the driver and wool. That is in the right range for a 2″ BMR. Check it against the driver's recommended box, and listen to a **printed test box of the same volume** on the V2 bench first.
- **Floor:** the carrier PCB lies flat along the bottom of the tube, under the chamber. The lamp LEDs are on its **underside**, shining down through a slot in the tube's bottom wall.
- **Length:** tube cut to **130 mm**. The ~111 mm screen sits centred with room at each end for the FPC and the end-cap steps.
- **A 2.5″ driver (Dayton ND65) is too big** for the back wall of a 3″ tube. If it clearly wins the listening test, move to a 3″ × 4″ rectangular tube (4″ deep), which also doubles the chamber volume.

**Resulting size:** about **143 W × 89 H × 76 D mm** (5.6″ × 3.5″ × 3.0″). That's the tube (76.2 mm) plus the glass (12.7 mm) for the height, and 130 mm plus two ¼″ caps for the width. Weight is about 0.5 kg for the aluminium plus ~0.3 kg for the glass. Heavy enough that tapping the screen never moves it.

Print the whole thing in PLA first. Proportions are easier to judge on the nightstand than on screen.

### Parts

- **Body:** off-the-shelf **6063-T52 aluminium square tube**, 3″ × 3″ × ⅛″, cut to 130 mm.
  - Front face: the screen window.
  - Back face: a field of grille slots over the speaker.
  - Bottom face: the lamp slot, about 100 × 6 mm.

  Stock tube needs **no tooling**. A custom extrusion die (`BOM.md`) only makes sense much later.
- **End caps:** two pieces of ¼″ × 3″ 6061 flat bar, each cut to 89 mm tall. The upper part of each cap has a ~3 mm step that locates inside the tube. The lower 12.7 mm, below the tube, has a **1.5 mm-deep groove** that holds the end of the glass base. One cap has the USB-C opening, the other the snooze/stop button. **The caps are the feet**, with cork pads.
- **Sled:** a bent 5052 aluminium sheet screwed to one end cap. It carries:
  - the PCB on aluminium standoffs;
  - the speaker bulkhead.

  You build and **test the whole sled outside the case**, then slide it in and screw on the second cap. The caps screw into the sled, not the thin tube wall, so the tube is simply clamped between them.
- **Screen:** the cover glass sits against a lip in the front window, recessed ~0.5–1 mm with a chamfered edge. A frame on the sled holds it from behind through a thin felt gasket, so there's no adhesive and the glass is replaceable. Leave 0.2–0.3 mm clearance per side.
- **Speaker (rear-firing):** the driver's flange sits against the inside of the back wall on an O-ring, and the chamber bulkhead presses it there. **No screws show on the back face**, just the slots, with black wool felt behind them.
- **Glass base:** ½″ **low-iron** glass, **132 × 76 mm** (the tube length plus 1 mm into each cap groove), polished edges, **bottom face sandblasted in the shop**.
  - The glass is clamped between the two caps with wool felt in the grooves, so it isn't drilled or glued.
  - Light from the LEDs enters the top face and scatters off the frosted bottom. The glass glows red, its polished edges catch the light, and a soft wash spills onto the nightstand through the ~1.5 mm gap underneath.
  - Put a felt ring around the lamp slot so no light leaks up into the case.
  - Low-iron glass is what keeps the edges clean red. Ordinary glass edges look green.

Why this over the alternatives:

| Approach | Look | Per-unit effort in your shop | Tooling | Verdict |
| --- | --- | --- | --- | --- |
| **Tube + caps + sled** | Premium, clean | Low: saw, 2–3 short CNC ops | None | **Start here** |
| Bent sheet (5052 U-channel + end plates) | Honest, industrial | Low on a brake; no CNC needed | None | Good pre-CNC fallback |
| Billet unibody (Apple-style) | Most premium | High: hours per unit, ~80% of the stock becomes chips | None | Nice for a limited "founders" run, not the core product |
| Custom extrusion + caps | Premium | Lowest | $500–2k die | Once you're past a few hundred units |
| Die-cast / forged caps | Premium | Lowest | $5k+ | Thousands of units |

## Getting rid of plastics

| Usually plastic | Metal / natural alternative |
| --- | --- |
| Case | Aluminium (anodized) |
| Lamp diffuser (opal acrylic) | **Sandblasted or acid-etched glass.** You can sandblast in the shop. Or skip the diffuser: bounce the light off a bead-blasted aluminium surface or the nightstand (see Lamp placement). |
| Speaker grille | Hole or slot pattern machined into the case, with **wool felt or acoustic cloth** behind |
| Cabinet damping (polyfill) | **Wool felt / natural wool batting.** This is actually a very good acoustic absorber. |
| Rubber feet | **Cork or wool felt pads** |
| Button cap | Machined aluminium or brass over a PCB tactile switch |
| Standoffs, spacers | Aluminium or brass |
| Air seal for the speaker chamber | **O-ring in a machined groove** (silicone/nitrile rubber, an elastomer), or cork gasket |
| Speaker cone | Paper or aluminium cone drivers are common and sound good |
| Packaging | Molded paper pulp + cardboard |

## Finish

- **Anodizing (Type II)** is the fit: it's the aluminium's own oxide, not a coating. Bead blast first for an even matte surface that hides fingerprints and scratches.
- **Black anodize (decided).** It frames the red digits, doesn't reflect the lamp or the screen at night, and hides the gap around the glass.
- **Outsource anodizing at first.** Local job shops batch it; expect a lot minimum (often ~$100–200) and a few dollars per part in a batch. DIY Type II anodizing is doable later, but it means sulfuric acid, a power supply, dyes and waste handling. Only bring it in-house once you're producing steadily.
- **Chamfer every edge** with a chamfer mill as the last CNC pass. It's what makes machined aluminium feel expensive, and it removes burrs.
- **Grounding:** anodize is an insulator. Mask one spot, or machine it bare after anodizing, where a star washer bonds the case to the sled and circuit ground. That keeps capacitive touch stable and gives static discharges a path (CE requires ESD testing).

## Designing parts for your CNC

Rules that keep machine time and setups down:

- **One or two setups per part.** Every feature should be reachable from one side or its opposite. End caps: op 1 is the outside profile and pockets; op 2 flips onto soft jaws for the inside step and chamfers.
- **Inside corner radius ≥ your cutter's radius** (≥ 3 mm with a 6 mm end mill). The screen window corners will be rounded; match them to the glass corner radius.
- **Pocket depth ≤ 3–4× cutter diameter.**
- **Threads:** M3 tapped in aluminium with ≥ 1.5× diameter engagement. Where screws go in and out (the end caps), use **stainless helicoils** or thread into the thicker cap rather than the ⅛″ tube wall.
- **Speaker grille:** slots cut with a small end mill are much faster than hundreds of drilled holes, and look just as good.
- **Tube work needs support:** machining the screen window in a ⅛″ wall chatters unless a block inside supports it. Make an aluminium or printed **internal mandrel** that the tube slides onto.
- **Standard stock and tooling:** design around tube and plate sizes you can buy, and a small set of cutters (6 mm and 3 mm end mills, a 90° chamfer mill, M3 tap drill).

## What the 3D printer is for

For a metal product, the printer earns its keep on **everything except the product**:

- **Fit and proportion mock-ups** before cutting aluminium: a printed tube section, printed end caps, a printed sled. You can prove the whole assembly with PLA/PETG. The first ones double as the V2 breadboard's temporary case.
- **Soft jaws and fixtures** for the CNC: nests that hold end caps for op 2, the internal mandrel for window machining, and saw stops.
- **Assembly jigs:** a nest that holds the glass square while the frame is fitted, and a sled-building jig.
- **The pogo-pin test fixture body** for end-of-line testing (`BOM.md`).
- **Speaker chamber experiments:** print chambers of different volumes and listen before committing to metal.

## Lamp: glass base

The lamp is decided: it lights the glass base the clock stands on (see Parts above). Two things to check on the V2 bench with the actual LEDs and a sandblasted offcut:

- **Grit.** Coarser sandblasting scatters more light out of the bottom (more nightstand wash). Finer scatters less, so more light travels to the edges (brighter edge lines). Try both on offcuts.
- **Brightness.** A glowing base is a comfortable night-light and enough to find things by. It is not a reading lamp. If you want reading light later, that is a second, upward-facing channel.

## Assembly sequence

1. **Sled:**
   - PCB onto standoffs.
   - Speaker and O-ring on the bulkhead, wool damping, speaker cable.
   - Lamp LEDs (on the PCB underside) and the felt ring around them.
   - Screen FPC connected.
2. **Test the sled outside the case**, on the pogo fixture or plugged in: self-test mode, flash firmware, record the serial number (`BOM.md` → Firmware for production).
3. **Case:**
   - Glass + panel into the front window.
   - Slide the sled in (the frame presses the screen glass, the bulkhead presses the speaker), then fit the glass base into the fixed cap's groove.
   - Close the second end cap, which clamps the tube and the glass base.
   - Cork feet on the caps.
4. **Final check:** touch, sound, lamp, clock set. Then pack.

Aim for **only one screw size and one driver** throughout (M3 stainless, hex).

## Scaling plan

| Stage | Units | Enclosure | Electronics | You do |
| --- | --- | --- | --- | --- |
| Prototype (V2/V3) | 1–10 | Printed mock-ups → tube + caps by hand / manual mill | T-Display-P4 + breadboard → first PCB | Everything |
| Maker batch | 10–200 | Tube + caps on your CNC with fixtures; batch anodizing outsourced | PCBs assembled by JLCPCB / PCBWay | Machining, final assembly, test |
| Small production | 200–2,000 | Custom extrusion (features like PCB slots and screw bosses built in); caps outsourced to a CNC shop (Xometry, PCBWay CNC, local job shop) | Same, with test at the board house if possible | Final assembly + QA, or hand it to a contract assembler |
| Volume | 2,000+ | Die-cast or forged caps, contract manufacturer does everything | CM | Design + business |

At the maker-batch stage **your machine time is the cost**. Make the per-unit machining short: a 20–40 min target for all enclosure parts together is realistic with good fixtures. Batch operations (all op 1s, then all op 2s) rather than finishing one clock at a time.

## Open questions for the mock-up stage

- The exact panel outline and FPC position. LilyGO's 3D model gives the outline, but get the bare panel's mechanical drawing before a fully custom V3.
- Speaker frame size and recommended box volume (Tectonic spec sheet), and whether ~0.2 L sounds good enough in the printed test box.
- Sandblast grit for the glass base.
