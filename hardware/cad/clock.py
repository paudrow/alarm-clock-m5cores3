"""Parametric model of the bedside clock enclosure.

Builds every part in millimetres, checks that nothing collides, and writes:

  out/<variant>/*.step       one STEP per part, plus assembly.step
  out/<variant>/*.stl        one STL per part (for 3D-printed mock-ups)
  out/<variant>/clock.glb    the assembly, coloured, for the web viewer
  out/<variant>/report.json  sizes, fit checks, masses and stock usage

Axes: X is width (left to right), Y is depth (front is -Y), Z is up. The table
is at z = 0.

Run with CadQuery installed:  python clock.py
"""

import json
import math
import base64
import os
from dataclasses import dataclass, asdict

import cadquery as cq
import trimesh

IN = 25.4


@dataclass
class Variant:
    key: str
    label: str
    # Screen module (panel + its board), landscape. Measured for the 4.1"
    # T-Display-P4 core from LilyGO's published model; estimated for the 2.41".
    screen_w: float
    screen_h: float
    screen_d: float
    active_w: float
    active_h: float
    screen_measured: bool
    tube_len: float


VARIANTS = [
    Variant("full", '4.1" AMOLED (T-Display-P4)', 103.8, 51.3, 12.3, 94.5, 43.6, True, 130.0),
    Variant("lean", '2.41" AMOLED (Waveshare, estimated outline)', 60.0, 48.0, 11.0, 49.0, 36.8, False, 105.0),
]

# Stock and fixed dimensions ---------------------------------------------------
TUBE_OD = 3 * IN            # 76.2, 3" square tube
WALL = 0.125 * IN           # 3.175, 1/8" wall
TUBE_R_OUT = 1.2            # extruded tube corner radius (outside)
CAP_T = 0.25 * IN           # 6.35, end caps from 1/4" x 3" bar
CAP_STEP = 3.0              # how far each cap's boss goes into the tube
BOSS_WALL = 3.0             # wall of that boss (a machined ring)
FIT = 0.15                  # clearance per side for slip fits
GLASS_T = 0.5 * IN          # 12.7, 1/2" low-iron glass
GLASS_GROOVE = 1.0          # glass end sits this deep in each cap
GLASS_FLOAT = 1.5           # glass bottom above the caps' bottom
FELT = 0.5                  # wool felt between glass and tube
FOOT_T = 1.6                # cork foot thickness
SHEET = 1.6                 # 5052 sheet for sled, bulkhead, chamber floor
SPK_OD = 52.0               # Tectonic TEBM35C10-4 (datasheet)
SPK_TABS = 54.0
SPK_DEPTH = 25.1
SPK_MOTOR_D = 36.0          # approximate
ORING = 1.0
PCB_T = 1.6
STANDOFF = 5.0

Z_GLASS0 = GLASS_FLOAT
Z_GLASS1 = Z_GLASS0 + GLASS_T
Z_TUBE0 = Z_GLASS1 + FELT
Z_TUBE1 = Z_TUBE0 + TUBE_OD
Z_MID = (Z_TUBE0 + Z_TUBE1) / 2
Y_FRONT = -TUBE_OD / 2
Y_BACK = TUBE_OD / 2
Y_IN_FRONT = Y_FRONT + WALL
Y_IN_BACK = Y_BACK - WALL
Z_IN_BOT = Z_TUBE0 + WALL
Z_IN_TOP = Z_TUBE1 - WALL


def box(x0, x1, y0, y1, z0, z1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0).translate(
        ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))


def rounded_box(x0, x1, y0, y1, z0, z1, r, axis):
    b = box(x0, x1, y0, y1, z0, z1)
    return b.edges(f"|{axis}").fillet(r) if r > 0 else b


def cyl_y(cx, cz, d, y0, y1):
    return cq.Workplane("XZ").circle(d / 2).extrude(-(y1 - y0)).translate((cx, y0, cz))


def cyl_x(cy, cz, d, x0, x1):
    return cq.Workplane("YZ").circle(d / 2).extrude(x1 - x0).translate((x0, cy, cz))


def cyl_z(cx, cy, d, z0, z1):
    return cq.Workplane("XY").circle(d / 2).extrude(z1 - z0).translate((cx, cy, z0))


def build(v: Variant):
    L = v.tube_len
    parts = {}
    notes = {}

    # Body: square tube with screen window, rear grille and lamp slot --------
    tube = rounded_box(-L / 2, L / 2, Y_FRONT, Y_BACK, Z_TUBE0, Z_TUBE1, TUBE_R_OUT, "X")
    tube = tube.cut(rounded_box(-L / 2 - 1, L / 2 + 1, Y_IN_FRONT, Y_IN_BACK, Z_IN_BOT, Z_IN_TOP, 0.6, "X"))
    win_w, win_h = v.active_w + 1.5, v.active_h + 1.5
    win_r = 2.0
    screen_cz = Z_MID
    tube = tube.cut(rounded_box(-win_w / 2, win_w / 2, Y_FRONT - 1, Y_IN_FRONT + 1,
                                screen_cz - win_h / 2, screen_cz + win_h / 2, win_r, "Y"))
    try:  # 45° chamfer round the window on the outside face
        tube = tube.edges(cq.selectors.BoxSelector(
            (-win_w / 2 - 1, Y_FRONT - 0.1, screen_cz - win_h / 2 - 1),
            (win_w / 2 + 1, Y_FRONT + 0.1, screen_cz + win_h / 2 + 1))).chamfer(0.8)
    except Exception:
        pass

    # Speaker sits centred on the back wall, in the chamber above the PCB.
    chamber_floor_z0 = Z_IN_BOT + SHEET + STANDOFF + PCB_T + 6.0
    chamber_floor_z1 = chamber_floor_z0 + SHEET
    spk_cz = (chamber_floor_z1 + Z_IN_TOP) / 2
    grille_d = SPK_OD - 6
    slot_w, pitch = 2.4, 5.2
    n = int(grille_d // pitch)
    xs = [(i - (n - 1) / 2) * pitch for i in range(n)]
    for x in xs:
        half = math.sqrt(max((grille_d / 2) ** 2 - x ** 2, 0))
        if half < 4:
            continue
        tube = tube.cut(rounded_box(x - slot_w / 2, x + slot_w / 2, Y_IN_BACK - 1, Y_BACK + 1,
                                    spk_cz - half, spk_cz + half, slot_w / 2 - 0.01, "Y"))
    # Lamp slot in the bottom wall, over the glass
    lamp_len = min(L - 40, 90.0)
    lamp_y = -12.0
    tube = tube.cut(rounded_box(-lamp_len / 2, lamp_len / 2, lamp_y - 3, lamp_y + 3,
                                Z_TUBE0 - 1, Z_IN_BOT + 1, 2.99, "Z"))
    parts["tube"] = tube

    # End caps -----------------------------------------------------------------
    cap_h = Z_TUBE1
    boss = TUBE_OD - 2 * WALL - 2 * FIT

    def cap(side):
        s = 1 if side > 0 else -1
        x_in = s * L / 2
        x_out = s * (L / 2 + CAP_T)
        c = box(min(x_in, x_out), max(x_in, x_out), Y_FRONT, Y_BACK, 0, cap_h)
        c = c.edges("|X").fillet(TUBE_R_OUT)
        c = c.faces(">X" if s > 0 else "<X").chamfer(0.8)
        # Boss that locates in the tube
        bx0, bx1 = sorted([x_in, x_in - s * CAP_STEP])
        # A ring rather than a solid block, so the PCB and USB-C reach the cap.
        ring = rounded_box(bx0, bx1, -boss / 2, boss / 2, Z_MID - boss / 2, Z_MID + boss / 2, 0.8, "X")
        rw = BOSS_WALL
        ring = ring.cut(rounded_box(bx0 - 1, bx1 + 1, -boss / 2 + rw, boss / 2 - rw,
                                    Z_MID - boss / 2 + rw, Z_MID + boss / 2 - rw, 1.6, "X"))
        c = c.union(ring)
        # Groove that holds the glass end
        gx0, gx1 = sorted([x_in, x_in + s * (GLASS_GROOVE + 0.2)])
        c = c.cut(box(gx0, gx1, Y_FRONT - 1, Y_BACK + 1, Z_GLASS0 - 0.2, Z_GLASS1 + 0.2))
        return c

    cap_l = cap(-1)
    cap_r = cap(+1)
    # Snooze/stop button on the left cap
    btn_z, btn_d = Z_MID + 20, 10.0
    cap_l = cap_l.cut(cyl_x(0, btn_z, btn_d + 0.4, -L / 2 - CAP_T - 1, -L / 2 + 1))
    # USB-C on the right cap: through-slot plus an outside pocket for the plug
    pcb_z0 = Z_IN_BOT + SHEET + STANDOFF
    usb_z = pcb_z0 + PCB_T + 1.6
    usb_y = 12.0
    web = 1.2
    cap_r = cap_r.cut(rounded_box(L / 2 - 1, L / 2 + CAP_T + 1, usb_y - 4.6, usb_y + 4.6,
                                  usb_z - 1.8, usb_z + 1.8, 1.79, "X"))
    cap_r = cap_r.cut(rounded_box(L / 2 + web, L / 2 + CAP_T + 1, usb_y - 6.75, usb_y + 6.75,
                                  usb_z - 3.5, usb_z + 3.5, 3.49, "X"))
    parts["cap_left"] = cap_l
    parts["cap_right"] = cap_r

    parts["button"] = cyl_x(0, btn_z, btn_d, -L / 2 - CAP_T - 1.2, -L / 2 - CAP_T + 3.0).faces("<X").chamfer(0.5)

    # Glass base ---------------------------------------------------------------
    gl = L + 2 * GLASS_GROOVE
    glass = box(-gl / 2, gl / 2, Y_FRONT, Y_BACK, Z_GLASS0, Z_GLASS1)
    glass = glass.edges().chamfer(0.4)
    parts["glass_base"] = glass

    # Screen module -------------------------------------------------------------
    sy0 = Y_IN_FRONT + 0.5  # thin gasket between cover glass and front wall
    sy1 = sy0 + v.screen_d
    screen = box(-v.screen_w / 2, v.screen_w / 2, sy0, sy1,
                 screen_cz - v.screen_h / 2, screen_cz + v.screen_h / 2)
    parts["screen"] = screen

    # Retaining frame behind the screen
    fr = box(-v.screen_w / 2 - 2, v.screen_w / 2 + 2, sy1, sy1 + 2,
             screen_cz - v.screen_h / 2 - 2, screen_cz + v.screen_h / 2 + 2)
    fr = fr.cut(box(-v.screen_w / 2 + 4, v.screen_w / 2 - 4, sy1 - 1, sy1 + 3,
                    screen_cz - v.screen_h / 2 + 4, screen_cz + v.screen_h / 2 - 4))
    parts["screen_frame"] = fr

    # Sled floor, PCB, chamber -------------------------------------------------
    inner_x = L / 2 - CAP_STEP - 0.5
    sled = box(-inner_x, inner_x, Y_IN_FRONT + 1.0, Y_IN_BACK - 1.0, Z_IN_BOT, Z_IN_BOT + SHEET)
    sled = sled.cut(rounded_box(-lamp_len / 2, lamp_len / 2, lamp_y - 3, lamp_y + 3,
                                Z_IN_BOT - 1, Z_IN_BOT + SHEET + 1, 2.99, "Z"))
    parts["sled"] = sled

    pcb_y0 = sy1 + 2.5
    pcb_y1 = boss / 2 - BOSS_WALL - 0.5  # clear of the cap's ring
    pcb_x0, pcb_x1 = -inner_x + 1, L / 2 - 0.5
    pcb = box(pcb_x0, pcb_x1, pcb_y0, pcb_y1, pcb_z0, pcb_z0 + PCB_T)
    parts["pcb"] = pcb
    offs = []
    for x in (pcb_x0 + 4, pcb_x1 - 8):
        for y in (pcb_y0 + 4, pcb_y1 - 4):
            offs.append(cyl_z(x, y, 5.0, Z_IN_BOT + SHEET, pcb_z0))
    so = offs[0]
    for o in offs[1:]:
        so = so.union(o)
    parts["standoffs"] = so
    # Parts on the PCB: USB-C receptacle, amp, LEDs on the underside
    usb = box(L / 2 - 7.5, L / 2, usb_y - 4.5, usb_y + 4.5, pcb_z0 + PCB_T, pcb_z0 + PCB_T + 3.2)  # flush with the cap
    parts["usb_c"] = usb
    leds = None
    for i in range(4):
        x = (i - 1.5) * (lamp_len / 4.5)
        led = box(x - 1.5, x + 1.5, lamp_y - 1.5, lamp_y + 1.5, pcb_z0 - 0.6, pcb_z0)
        leds = led if leds is None else leds.union(led)
    parts["leds"] = leds
    # PCB sits behind the screen, so the LEDs must fall inside it.
    notes["leds_on_pcb"] = pcb_y0 <= lamp_y - 1.5

    bulk_y0 = 0.0
    chamber = box(-inner_x, inner_x, bulk_y0, bulk_y0 + SHEET, chamber_floor_z0, Z_IN_TOP - 0.3)
    chamber = chamber.union(box(-inner_x, inner_x, bulk_y0, Y_IN_BACK - 0.3, chamber_floor_z0, chamber_floor_z1))
    parts["chamber"] = chamber

    # Speaker: flange against the back wall on an O-ring, motor pointing forward
    fl_y1 = Y_IN_BACK - ORING
    fl_y0 = fl_y1 - 2.5
    spk = cyl_y(0, spk_cz, SPK_OD, fl_y0, fl_y1)
    spk = spk.union(cyl_y(0, spk_cz, SPK_MOTOR_D, fl_y1 - SPK_DEPTH, fl_y0))
    spk = spk.cut(cq.Workplane("XZ").circle(SPK_OD / 2 - 3).workplane(offset=-8).circle(10)
                  .loft().translate((0, fl_y1 + 0.01, spk_cz)))
    parts["speaker"] = spk

    # Cork feet under the caps
    feet = None
    for x in (-(L / 2 + CAP_T / 2), L / 2 + CAP_T / 2):
        for y in (-27.0, 27.0):
            f = cyl_z(x, y, 6.0, -FOOT_T, 0)
            feet = f if feet is None else feet.union(f)
    parts["feet"] = feet

    geo = dict(
        overall_w=L + 2 * CAP_T, overall_h=cap_h + FOOT_T, overall_d=TUBE_OD,
        window=[win_w, win_h], active=[v.active_w, v.active_h],
        screen_center=[0, sy0, screen_cz], active_center=[0, sy0, screen_cz],
        glass=[gl, TUBE_OD, GLASS_T], tube_len=L, cap_blank=[TUBE_OD, cap_h, CAP_T],
        speaker_center=[0, fl_y1, spk_cz], lamp_slot=[lamp_len, 6.0, lamp_y],
        chamber_box=[2 * inner_x, Y_IN_BACK - 0.3 - (bulk_y0 + SHEET), Z_IN_TOP - 0.3 - chamber_floor_z1],
    )
    return parts, geo, notes


# Metadata for the viewer and report ------------------------------------------
# label, material, look in the viewer, and where the part moves (mm, CAD axes)
# in the fully exploded view. The body lifts straight up so the inside shows;
# everything else moves out along the direction it is assembled.
PART_INFO = {
    "tube": ("Body", "6063-T52 square tube, black anodized", "alu_black", [0, 0, 110]),
    "cap_left": ("End cap, button side", "6061 bar, black anodized", "alu_black", [-75, 0, 0]),
    "cap_right": ("End cap, USB-C side", "6061 bar, black anodized", "alu_black", [75, 0, 0]),
    "button": ("Snooze / stop button", "6061, black anodized", "alu_black", [-105, 0, 0]),
    "glass_base": ("Glass base", '1/2" low-iron glass, sandblasted underside', "glass", [0, 0, -45]),
    "screen": ("Screen module", "AMOLED panel + board", "screen", [0, -95, 0]),
    "screen_frame": ("Screen retaining frame", "5052 aluminium", "alu_raw", [0, -55, 0]),
    "sled": ("Sled", "5052 sheet", "alu_raw", [0, 0, -22]),
    "pcb": ("Carrier PCB", "FR-4, matte black", "pcb", [0, 0, 0]),
    "standoffs": ("Standoffs", "M3 aluminium", "alu_raw", [0, 0, -10]),
    "usb_c": ("USB-C receptacle", "", "steel", [0, 0, 6]),
    "leds": ("660 nm LEDs", "OSRAM OSCONIQ P 3030", "led", [0, 0, -12]),
    "chamber": ("Speaker chamber", "5052 sheet", "alu_raw", [0, 40, 38]),
    "speaker": ("Speaker", "Tectonic TEBM35C10-4", "speaker", [0, 95, 38]),
    "feet": ("Cork feet", "natural cork", "cork", [0, 0, -62]),
}
DENSITY = {"alu_black": 2.70, "alu_raw": 2.70, "glass": 2.50, "cork": 0.24}


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    for v in VARIANTS:
        out = os.path.join(here, "out", v.key)
        os.makedirs(out, exist_ok=True)
        parts, geo, notes = build(v)

        # Fit check: every pair of parts must not overlap
        names = list(parts)
        clashes = []
        solids = {n: parts[n].val() for n in names}
        for i, a in enumerate(names):
            for b in names[i + 1:]:
                va = solids[a].BoundingBox()
                vb = solids[b].BoundingBox()
                if (va.xmax < vb.xmin or vb.xmax < va.xmin or va.ymax < vb.ymin or
                        vb.ymax < va.ymin or va.zmax < vb.zmin or vb.zmax < va.zmin):
                    continue
                vol = solids[a].intersect(solids[b]).Volume()
                if vol > 0.01:
                    clashes.append({"a": a, "b": b, "mm3": round(vol, 2)})

        # Chamber air volume: box minus the speaker inside it
        cb = geo["chamber_box"]
        spk_vol = solids["speaker"].Volume()
        chamber_l = cb[0] * cb[1] * cb[2] / 1e6 - spk_vol / 1e6

        report = {"variant": asdict(v), "geometry": geo, "notes": notes,
                  "clashes": clashes, "chamber_litres": round(chamber_l, 3), "parts": {}}
        asm = cq.Assembly(name=f"clock_{v.key}")
        for n in names:
            label, material, look, explode = PART_INFO[n]
            shape = parts[n]
            vol = solids[n].Volume()
            bb = solids[n].BoundingBox()
            mass = vol / 1000 * DENSITY.get(look, 0) if look in DENSITY else None
            report["parts"][n] = {
                "label": label, "material": material, "look": look, "explode": explode,
                "volume_mm3": round(vol, 1), "mass_g": round(mass, 1) if mass else None,
                "bbox": [round(bb.xlen, 2), round(bb.ylen, 2), round(bb.zlen, 2)],
            }
            cq.exporters.export(shape, os.path.join(out, f"{n}.step"))
            cq.exporters.export(shape, os.path.join(out, f"{n}.stl"), tolerance=0.05, angularTolerance=0.2)
            asm.add(shape, name=n)
        asm.save(os.path.join(out, "assembly.step"))

        # One GLB for the web viewer; materials are assigned there by part name.
        scene = trimesh.Scene()
        for n in names:
            mesh = trimesh.load(os.path.join(out, f"{n}.stl"))
            mesh.merge_vertices()
            scene.add_geometry(mesh, node_name=n, geom_name=n)
        scene.export(os.path.join(out, "clock.glb"))
        with open(os.path.join(out, "report.json"), "w") as f:
            json.dump(report, f, indent=2)
        print(f"{v.key}: {geo['overall_w']:.1f} x {geo['overall_h']:.1f} x {geo['overall_d']:.1f} mm, "
              f"chamber {chamber_l:.3f} L, clashes {clashes}, notes {notes}")


def write_viewer():
    """Fill the viewer template with both reports and both models."""
    here = os.path.dirname(os.path.abspath(__file__))
    reports, glbs = {}, {}
    viewer = os.path.join(here, "viewer")
    os.makedirs(viewer, exist_ok=True)
    for v in VARIANTS:
        with open(os.path.join(here, "out", v.key, "report.json")) as f:
            reports[v.key] = json.load(f)
        with open(os.path.join(here, "out", v.key, "clock.glb"), "rb") as f:
            glbs[v.key] = base64.b64encode(f.read()).decode()
    with open(os.path.join(here, "viewer_template.html")) as f:
        html = (f.read().replace("/*REPORTS*/", json.dumps(reports, separators=(",", ":")))
                .replace("/*GLB*/", json.dumps(glbs)))
    with open(os.path.join(viewer, "index.html"), "w") as f:
        f.write(html)


if __name__ == "__main__":
    main()
    write_viewer()
