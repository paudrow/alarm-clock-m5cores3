# Manufacturing plan

How to build the enclosure and assemble the clock, from first prototypes in a home shop to small-batch sales and beyond. Parts and costs are in `BOM.md`; the electronics are in `DESIGN.md`.

Goals:

- **No plastic that you see or touch.** Aluminium body, glass, and natural materials elsewhere. The electronics inside (PCB, connectors, the display's own layers) will always contain some polymers.
- **Prototype fast** with the 3D printer, then move to metal.
- **Build in your own shop:** metal shop now, a CNC mill later.
- **Keep a clear path to outsourcing** if volume grows.

## Recommended construction: tube + two end caps + internal sled

```
           end cap (CNC)                               end cap (CNC)
              ┌─┐  ┌───────────────────────────────────┐  ┌─┐
              │ │  │  ┌─────────────────────────┐       │  │ │
              │ │  │  │   screen window + glass │       │  │ │  ← aluminium
              │ │  │  └─────────────────────────┘       │  │ │    square tube,
              │ │  │                                    │  │ │    cut to length
              │ │  └────────────────────────────────────┘  │ │
              └─┘    ▲ lamp slot            ▲ speaker        └─┘
                                              (fires down)
   inside:  [end cap]══[ sled: PCB + speaker chamber + lamp board ]
            the whole sled slides into the tube, the second cap closes it
```

- **Body:** off-the-shelf **6063 aluminium square tube**, cut to length.
  - Example: 3″ × 3″ × ⅛″ wall, ~150 mm long. That gives about a 150 × 76 × 76 mm clock. The 4.1″ screen fits the front face with a generous bezel, and a 2″ driver fits the bottom face.
  - Weight is about 0.4 kg for the tube, ~0.5 kg complete. That heft reads as premium and stops the clock sliding when you tap the screen.
  - Stock tube needs **no tooling**. A custom extrusion die (`BOM.md`) only makes sense much later.
- **End caps:** machined from ¼″ (6 mm) 6061 plate or bar. They locate into the tube with a small step, carry the USB-C opening and the physical button, and hold everything together.
- **Sled:** a bent 5052 aluminium sheet (or a machined plate) screwed to one end cap. It carries:
  - the PCB, on aluminium standoffs;
  - the lamp LEDs;
  - a bulkhead forming the sealed speaker chamber.

  You assemble and **test the whole sled outside the case**, then slide it in. This is the main design-for-assembly win: every electrical connection is made in the open.
- **Screen:** the cover glass sits against a lip machined into the front window, recessed ~0.5–1 mm with a chamfered edge. A frame on the sled holds it from behind, pressing through a thin felt or silicone gasket, so no adhesive is needed and the glass can be replaced. Leave 0.2–0.3 mm clearance per side for glass tolerance.

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
- **Black anodize** is the strong default for a bedside clock. It frames the red digits, doesn't reflect the lamp or the screen at night, and hides the gap around the glass. Natural/clear is the alternative.
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

## Lamp placement (decide with mock-ups)

- **Downlight "wash":** a slot along the bottom front edge washes red light onto the nightstand. Indirect, diffuse, needs no diffuser, and is very comfortable at night. The best fit for "no plastic".
- **Front strip:** a sandblasted glass strip under the screen. More visible and more direct.
- **Top glow:** light bounced off the ceiling or wall. Good as a night-light, weaker for reading.

## Assembly sequence

1. **Sled:**
   - PCB onto standoffs.
   - Speaker into the chamber: O-ring, wool damping, speaker cable.
   - Lamp LEDs.
   - Screen FPC connected.
2. **Test the sled outside the case**, on the pogo fixture or plugged in: self-test mode, flash firmware, record the serial number (`BOM.md` → Firmware for production).
3. **Case:**
   - Glass + panel into the front window.
   - Slide the sled in (the frame presses the glass), then close the second end cap.
   - Feet on.
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

- The exact panel outline and FPC position. Get the panel's mechanical drawing before designing the window and sled.
- Tube size: 3″ × 3″ is a starting guess. It depends on the panel outline, speaker diameter and the proportions you like.
- Lamp placement: wash, strip or top.
- Speaker direction: down-firing through the bottom (hidden grille, uses the table as a reflector) or out the back.
- Finish: black or natural anodize.
