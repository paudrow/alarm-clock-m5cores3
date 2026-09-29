# CAD

A parametric model of the clock, written in Python with [CadQuery](https://cadquery.readthedocs.io/). The code is the source of truth: change a dimension, re-run, and every file below is rebuilt and re-checked.

| File | What it is |
| --- | --- |
| `clock.py` | The model. Three variants: `bench` (V2 breadboard with 3D-printed holders), `full` (V3, 4.1″ AMOLED, 130 mm tube) and `lean` (V3, 2.41″ AMOLED, 105 mm tube). |
| `out/<variant>/*.step` | One STEP per part, plus `assembly.step`. Open these in Fusion 360, FreeCAD or Onshape for CAM or further design. |
| `out/<variant>/*.stl` | One STL per part, for 3D-printed mock-ups. Print `tube`, `cap_left`, `cap_right` and `glass_base` to judge size and proportions on the nightstand. |
| `out/<variant>/report.json` | Sizes, masses, the collision check and the speaker-chamber volume |
| `viewer_template.html` → `viewer/index.html` | The interactive 3D viewer, with all three versions embedded |

## V2 breadboard: parts to 3D-print

`out/bench/` has STLs for seven holders. Together they make the breadboard build a tidy bench rig that tests the same things the finished clock depends on. Each fits a 180 mm bed, about 160 g of PLA or PETG in all.

| STL | What it does | How to print |
| --- | --- | --- |
| `display_stand.stl` | Holds the T-Display-P4 (in LilyGO's shell) leaning back 15°, like the clock, with a cable notch | On its base, no supports |
| `bb_tray.stl` | Holds the full-size breadboard, with finger notches to lift it out | Flat, no supports |
| `spk_box.stl` | Sealed speaker test box, **0.19 L of air**, the same as the 4.1″ clock's chamber | Front face down, open back up. 4 walls so it's airtight. |
| `spk_ring.stl` | Clamps the Tectonic speaker's flange into the box's front recess | Flat |
| `spk_lid.stl` | Closes the back. Put a wool-felt gasket under it. | Flat |
| `spk_filler.stl` | Slide in behind the speaker to drop the box to **0.14 L**, close to the 2.41″ clock's chamber | Flat |
| `lamp_cradle.stl` | Holds the real ½″ glass base in end grooves, with two LED star boards **9.7 mm above the glass**, as in the clock | Upside down (bridge on the bed); the grooves print as small overhangs |

Hardware: 8 × M3 × 10 mm screws, driven straight into the 2.5 mm pilot holes (4 for the lid, 4 for the ring). Seal the speaker-wire hole with putty.

## Run it

```sh
pip install cadquery trimesh
python clock.py
```

Each run:
- builds every part;
- intersects every pair of parts, and fails loudly in the report if any overlap;
- exports STEP, STL and GLB;
- regenerates the viewer.

## What's measured and what's estimated

- **Screen module, 4.1″:** 103.8 × 51.3 × 12.3 mm (panel plus its board). This is measured from LilyGO's published T-Display-P4 model (`structure/H753.stp` in their GPL repo). Only the dimensions are used; none of their geometry is copied into this repo.
- **Screen module, 2.41″:** estimated until the Waveshare drawing is checked.
- **Speaker:** Tectonic TEBM35C10-4, 52 mm across (54 mm at the tabs) and 25.1 mm deep, from its datasheet. The motor diameter is approximate.
- **Stock:** 3″ × ⅛″ square tube, ¼″ end caps, ½″ glass, 1.6 mm 5052 sheet. These are the real stock sizes from `BOM.md`.

## LLMs and CAD

Code-based CAD works best with an assistant like Claude: the model is plain text that can be read, edited, diffed and re-run.

| Tool | Kind | Output | Notes |
| --- | --- | --- | --- |
| **CadQuery** / **build123d** | Python on the OpenCascade kernel | STEP (true solids), STL | Used here. STEP is what CAM and machine shops want. |
| OpenSCAD | Its own scripting language | STL (mesh) | Simple and popular, but mesh-only, so it's weaker for CNC work |
| [DingCAD](https://github.com/yacineMTB/dingcad) | JavaScript scripting on the Manifold mesh kernel, live reload | Mesh | Early-stage and undocumented; mesh output rather than STEP |
| Zoo (formerly KittyCAD) | Text-to-CAD service and the KCL language | STEP, others | Generates parts from a prompt; good for quick single parts |
| FreeCAD / Fusion 360 / Onshape | Traditional CAD, with scripting APIs and community MCP connectors | STEP | Use these for CAM (toolpaths for your mill) from the STEP files here |

Suggested workflow:
1. Keep `clock.py` as the design. Ask Claude to change it and re-run.
2. Open `out/<variant>/*.step` in Fusion 360 or FreeCAD to program the CNC.
3. Print the STLs to check fit and proportions before cutting aluminium.
