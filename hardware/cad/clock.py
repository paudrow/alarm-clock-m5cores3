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
    kind: str = "enclosure"     # "enclosure" (V3) or "bench" (V2 breadboard)
    version: str = "V3"


VARIANTS = [
    # V2: the whole T-Display-P4 in LilyGO's own shell (62.6 x 111.2 x 22.4 mm,
    # measured from their model), on a printed stand, with the other modules.
    Variant("bench", "V2 breadboard with printed holders", 111.2, 62.6, 22.4, 94.5, 43.6, True, 0.0,
            kind="bench", version="V2"),
    Variant("full", 'V3 enclosure, 4.1" AMOLED (T-Display-P4)', 103.8, 51.3, 12.3, 94.5, 43.6, True, 130.0),
    Variant("lean", 'V3 enclosure, 2.41" AMOLED (Waveshare, estimated outline)', 60.0, 48.0, 11.0, 49.0, 36.8, False, 105.0),
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
    return build_bench(v) if v.kind == "bench" else build_enclosure(v)


def build_enclosure(v: Variant):
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
        screen_node="screen", screen_tilt=0.0, glass_center=[0, 0], glass_node="glass_base", led_node="leds",
        view_target=[0, 0, 46], cutaway=["tube", "cap_left", "button"],
    )
    return parts, geo, notes


# V2 breadboard layout with 3D-printable holders --------------------------------
BB_L, BB_W, BB_H = 165.1, 54.6, 8.5   # Adafruit 239 full-size breadboard
TILT = 15.0                           # display leans back like a clock
LAMP_Y = -12.0                        # lamp line, same offset as the enclosure
LED_GAP = 9.7                         # LED face to glass top, same as the enclosure


def build_bench(v: Variant):
    parts, notes = {}, {}

    # Breadboard tray (print) with finger notches at the ends
    tray = box(-87.5, 87.5, -32, 32, 0, 5).edges("|Z").fillet(3)
    tray = tray.cut(box(-BB_L / 2 - 0.3, BB_L / 2 + 0.3, -BB_W / 2 - 0.3, BB_W / 2 + 0.3, 3, 6))
    tray = tray.cut(cyl_z(-87.5, 0, 22, 3, 6)).cut(cyl_z(87.5, 0, 22, 3, 6))
    parts["bb_tray"] = tray
    bb_top = 3 + BB_H
    parts["breadboard"] = box(-BB_L / 2, BB_L / 2, -BB_W / 2, BB_W / 2, 3, bb_top)

    # Modules on the breadboard (boards sit on their headers, ~2.5 mm up)
    zb = bb_top + 2.5
    parts["amp"] = box(-68, -49, -9, 9, zb, zb + 1.6)
    parts["light_sensor"] = box(-40, -14.6, -8.9, 8.9, zb, zb + 1.6)
    parts["rtc"] = box(-8, 22, -10, 10, zb, zb + 1.6)
    parts["opamp"] = box(30, 39.8, -3.2, 3.2, bb_top, bb_top + 3.3)
    fet_z = bb_top + 3
    fet = box(48, 58.2, -2.3, 2.3, fet_z, fet_z + 9.2).union(box(48, 58.2, 1.0, 2.3, fet_z + 9.2, fet_z + 15.9))
    parts["mosfet"] = fet.cut(cyl_y(53.1, fet_z + 12.6, 3.6, 0.5, 3))
    parts["mosfet_small"] = cyl_z(68, 0, 4.8, fet_z, fet_z + 5)

    # Display stand (print) and the T-Display-P4 leaning back on it
    W, H, T = v.screen_w, v.screen_h, v.screen_d
    th = math.radians(TILT)
    y0, base_top = 52.0, 4.0
    dz = base_top + 0.3 + T * math.sin(th)

    def place(wp):
        return wp.rotate((0, 0, 0), (1, 0, 0), -TILT).translate((0, y0, dz))

    parts["tdisplay"] = place(box(-W / 2, W / 2, 0, T, 0, H).edges("|Y").fillet(4))
    back = box(-45, 45, T + 0.4, T + 4.4, -14, H * 0.75)
    back = back.cut(box(-15, 15, T, T + 6, -20, 14))  # cable notch
    back = place(back).intersect(box(-70, 70, 0, 250, base_top - 0.01, 250))
    bby = back.val().BoundingBox()
    base = box(-62.5, 62.5, y0 - 7, bby.ymax + 3, 0, base_top).edges("|Z").fillet(3)
    lip = box(-50, 50, y0 - 5, y0 - 0.3, base_top - 0.01, dz + 6)
    parts["display_stand"] = base.union(lip).union(back)
    s_local = (0.0, -0.05, H / 2)
    screen_center = [0.0,
                     y0 + s_local[1] * math.cos(th) + s_local[2] * math.sin(th),
                     dz - s_local[1] * math.sin(th) + s_local[2] * math.cos(th)]

    # Sealed speaker test box (print): same air volume as the enclosure chamber
    SBX, SBY = 170.0, 20.0
    iw, ih, idp, wt, wf = 70.0, 60.0, 52.0, 3.0, 5.0
    bx0, bx1 = SBX - iw / 2 - wt, SBX + iw / 2 + wt
    by0 = SBY - idp / 2 - wf          # outside of the front baffle
    by1 = by0 + wf + idp              # open back, closed by the lid
    scz = wt + ih / 2
    body = box(bx0, bx1, by0, by1, 0, ih + 2 * wt).edges("|Y").fillet(3)
    cavity = box(SBX - iw / 2, SBX + iw / 2, by0 + wf, by1 + 1, wt, wt + ih)
    body = body.cut(cavity)
    bosses = None
    boss_pts = []
    for sx in (-1, 1):
        for sz in (-1, 1):
            cx, cz = SBX + sx * (iw / 2 - 3.5), scz + sz * (ih / 2 - 3.5)
            boss_pts.append((cx, cz))
            b = box(cx - 3.5, cx + 3.5, by0 + wf - 0.01, by1, cz - 3.5, cz + 3.5)
            bosses = b if bosses is None else bosses.union(b)
    body = body.union(bosses)
    for cx, cz in boss_pts:
        body = body.cut(cyl_y(cx, cz, 2.5, by1 - 12, by1 + 1))          # M3 pilot holes for the lid
    body = body.cut(cyl_y(SBX, scz, 46.0, by0 - 1, by0 + wf + 1))       # speaker opening
    body = body.cut(cyl_y(SBX, scz, SPK_OD + 0.6, by0 - 1, by0 + 2.5))  # flush recess for the flange
    ring_holes = [(SBX + 29 * math.cos(math.radians(a)), scz + 29 * math.sin(math.radians(a))) for a in (45, 135, 225, 315)]
    for px, pz in ring_holes:
        body = body.cut(cyl_y(px, pz, 2.5, by0 - 1, by0 + 4))
    body = body.cut(cyl_x(by0 + wf + 10, 10, 4.0, bx1 - wt - 1, bx1 + 1))  # speaker wire hole
    parts["spk_box"] = body

    spk = cyl_y(SBX, scz, SPK_OD, by0 + 0.05, by0 + 2.5)
    spk = spk.union(cyl_y(SBX, scz, SPK_MOTOR_D, by0 + 2.5, by0 + SPK_DEPTH))
    spk = spk.cut(cyl_y(SBX, scz, 42, by0 - 1, by0 + 1.8))
    parts["speaker"] = spk

    ring = cyl_y(SBX, scz, 64, by0 - 3, by0 - 0.05).cut(cyl_y(SBX, scz, 47, by0 - 4, by0 + 1))
    for px, pz in ring_holes:
        ring = ring.cut(cyl_y(px, pz, 3.4, by0 - 4, by0 + 1))
    parts["spk_ring"] = ring

    lid = box(bx0, bx1, by1 + 0.05, by1 + 3.05, 0, ih + 2 * wt).edges("|Y").fillet(3)
    for cx, cz in boss_pts:
        lid = lid.cut(cyl_y(cx, cz, 3.4, by1 - 1, by1 + 4))
    parts["spk_lid"] = lid

    # Filler block: slide it in behind the speaker to match the lean build (shown beside the box)
    fw, fh, fd = iw - 14.6, ih - 0.6, 13.5
    parts["spk_filler"] = box(SBX - fw / 2, SBX + fw / 2, by1 + 12, by1 + 12 + fd, 0, fh).edges("|Y").fillet(1)

    air = box(SBX - iw / 2, SBX + iw / 2, by0 + wf, by1, wt, wt + ih).cut(bosses).cut(spk)
    air_l = air.val().Volume() / 1e6
    filler_l = fw * fh * fd / 1e6

    # Lamp test cradle (print): holds the real glass base, LEDs above it as in the clock
    LCY = -115.0
    gz0, gz1 = GLASS_FLOAT, GLASS_FLOAT + GLASS_T
    rx_in, rx_out = 65.0, 77.0
    led_plane = gz1 + LED_GAP + 2.0       # underside of the star boards (LED domes ~2 mm)
    top = led_plane + 4.0
    cradle = box(-rx_out, -rx_in, LCY - 42.1, LCY + 42.1, 0, top).union(
        box(rx_in, rx_out, LCY - 42.1, LCY + 42.1, 0, top))
    cradle = cradle.union(box(-rx_out, rx_out, LCY + LAMP_Y - 13, LCY + LAMP_Y + 13, led_plane, top))
    for sx in (-1, 1):
        g0, g1 = sorted([sx * rx_in, sx * (rx_in + GLASS_GROOVE + 0.2)])
        cradle = cradle.cut(box(g0, g1, LCY - 50, LCY + 50, gz0 - 0.2, gz1 + 0.2))
    for x in (-22, 22):
        cradle = cradle.cut(cyl_z(x, LCY + LAMP_Y, 20.4, led_plane - 0.01, led_plane + 1.8))
        cradle = cradle.cut(cyl_z(x + (6 if x > 0 else -6), LCY + LAMP_Y, 3.5, led_plane - 1, top + 1))
    parts["lamp_cradle"] = cradle
    gl = 2 * (rx_in + GLASS_GROOVE)
    parts["glass_sample"] = box(-gl / 2, gl / 2, LCY - TUBE_OD / 2, LCY + TUBE_OD / 2, gz0, gz1).edges().chamfer(0.4)
    stars = None
    for x in (-22, 22):
        st = cyl_z(x, LCY + LAMP_Y, 20, led_plane + 0.2, led_plane + 1.8).union(
            cyl_z(x, LCY + LAMP_Y, 5, led_plane - 1.8, led_plane + 0.2))
        stars = st if stars is None else stars.union(st)
    parts["led_stars"] = stars

    allbb = None
    for shape in parts.values():
        bb = shape.val().BoundingBox()
        allbb = [bb.xmin, bb.xmax, bb.ymin, bb.ymax, bb.zmax] if allbb is None else [
            min(allbb[0], bb.xmin), max(allbb[1], bb.xmax), min(allbb[2], bb.ymin), max(allbb[3], bb.ymax), max(allbb[4], bb.zmax)]
    geo = dict(
        overall_w=allbb[1] - allbb[0], overall_d=allbb[3] - allbb[2], overall_h=allbb[4],
        active=[v.active_w, v.active_h], active_center=screen_center, screen_center=screen_center,
        screen_node="tdisplay", screen_tilt=TILT, glass=[gl, TUBE_OD, GLASS_T], glass_center=[0, LCY],
        glass_node="glass_sample", led_node="led_stars",
        speaker_box_litres=round(air_l, 3), speaker_box_lean_litres=round(air_l - filler_l, 3),
        led_gap=LED_GAP, tilt=TILT,
        view_target=[(allbb[0] + allbb[1]) / 2, (allbb[2] + allbb[3]) / 2, 25], cutaway=["spk_box", "spk_lid", "lamp_cradle"],
        chamber_litres=round(air_l, 3),
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
    # V2 breadboard
    "bb_tray": ("Breadboard tray", "3D print", "pla", [0, 0, 0]),
    "breadboard": ("Breadboard", "Adafruit 239, 830 points", "breadboard", [0, 0, 25]),
    "amp": ("Amp breakout", "Adafruit MAX98357A #3006 (outline estimated)", "board", [0, 0, 55]),
    "light_sensor": ("Light sensor", "Adafruit VEML7700 #4162 (outline estimated)", "board", [0, 0, 55]),
    "rtc": ("Clock chip board", "RV-3028-C7 evaluation board (outline estimated)", "board", [0, 0, 55]),
    "opamp": ("Op-amp", "MCP6022-I/P, DIP-8", "chip", [0, 0, 45]),
    "mosfet": ("Lamp MOSFET", "IRL540NPBF, TO-220", "chip", [0, 0, 45]),
    "mosfet_small": ("Range MOSFET", "TN0702N3-G, TO-92", "chip", [0, 0, 45]),
    "display_stand": ("Display stand", "3D print, holds it at 15°", "pla", [0, 0, 0]),
    "tdisplay": ("T-Display-P4", "LilyGO, 4.1\u2033 AMOLED, in its own shell", "device", [0, -30, 75]),
    "spk_box": ("Speaker test box", "3D print, 0.19 L sealed", "pla", [0, 0, 0]),
    "spk_ring": ("Speaker clamp ring", "3D print, 4 \u00d7 M3 screws", "pla", [0, -35, 0]),
    "spk_lid": ("Test box lid", "3D print, wool felt gasket, 4 \u00d7 M3", "pla", [0, 45, 0]),
    "spk_filler": ("Volume filler", "3D print; slide in to test the lean build's 0.15 L", "pla", [0, 60, 0]),
    "lamp_cradle": ("Lamp test cradle", "3D print, holds the glass and LEDs", "pla", [0, 0, 45]),
    "glass_sample": ("Glass base", "\u00bd\u2033 low-iron glass, 132 \u00d7 76 mm", "glass", [0, 0, 0]),
    "led_stars": ("660 nm LEDs", "LUXEON SP-01-D2 on star boards, \u00d7 2", "led", [0, 0, 65]),
}
PRINTABLE = {"bb_tray", "display_stand", "spk_box", "spk_ring", "spk_lid", "spk_filler", "lamp_cradle"}
DENSITY = {"alu_black": 2.70, "alu_raw": 2.70, "glass": 2.50, "cork": 0.24}
PRINT_FILL = 1.24 * 0.45    # PLA at ~45% effective fill (walls plus infill), g/cm3


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
        if "chamber_litres" in geo:
            chamber_l = geo["chamber_litres"]
        else:
            cb = geo["chamber_box"]
            chamber_l = cb[0] * cb[1] * cb[2] / 1e6 - solids["speaker"].Volume() / 1e6

        report = {"variant": asdict(v), "kind": v.kind, "geometry": geo, "notes": notes,
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
                "printable": n in PRINTABLE,
                "print_g": round(vol / 1000 * PRINT_FILL, 1) if n in PRINTABLE else None,
            }
            cq.exporters.export(shape, os.path.join(out, f"{n}.step"))
            cq.exporters.export(shape, os.path.join(out, f"{n}.stl"), tolerance=0.05, angularTolerance=0.2)
            asm.add(shape, name=n)
        asm.save(os.path.join(out, "assembly.step"))

        # One GLB for the web viewer; materials are assigned there by part name.
        # Smooth-shade curved faces but keep sharp edges crisp (split at 30°),
        # and include normals so the viewer can light the surfaces.
        scene = trimesh.Scene()
        for n in names:
            mesh = trimesh.load(os.path.join(out, f"{n}.stl"))
            mesh.merge_vertices()
            mesh = trimesh.graph.smooth_shade(mesh, angle=math.radians(30))
            scene.add_geometry(mesh, node_name=n, geom_name=n)
        scene.export(os.path.join(out, "clock.glb"), include_normals=True)
        with open(os.path.join(out, "report.json"), "w") as f:
            json.dump(report, f, indent=2)
        print(f"{v.key}: {geo['overall_w']:.1f} x {geo['overall_h']:.1f} x {geo['overall_d']:.1f} mm, "
              f"chamber {chamber_l:.3f} L, clashes {clashes}, notes {notes}")


def write_viewer():
    """Fill the viewer template with every variant's report and model."""
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
