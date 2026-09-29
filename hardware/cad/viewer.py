"""Build the 3D viewer page and the website from the files clock.py writes to out/.

Needs only the Python standard library, so the site can be rebuilt and deployed
without CadQuery installed:

    python viewer.py
    (cd site && npx wrangler deploy)

Writes (both git-ignored; regenerate rather than edit):
  viewer/index.html     the viewer as a page, for publishing as a Claude artifact
  site/public/          the viewer as a standalone site, plus STL/STEP downloads and zips
"""

import base64
import json
import os
import shutil
import zipfile

VARIANT_KEYS = ["bench", "full", "lean"]


def write_viewer():
    """Fill the viewer template with every variant's report and model."""
    here = os.path.dirname(os.path.abspath(__file__))
    reports, glbs = {}, {}
    viewer = os.path.join(here, "viewer")
    os.makedirs(viewer, exist_ok=True)
    for key in VARIANT_KEYS:
        with open(os.path.join(here, "out", key, "report.json")) as f:
            reports[key] = json.load(f)
        with open(os.path.join(here, "out", key, "clock.glb"), "rb") as f:
            glbs[key] = base64.b64encode(f.read()).decode()
    with open(os.path.join(here, "viewer_template.html")) as f:
        html = (f.read().replace("/*REPORTS*/", json.dumps(reports, separators=(",", ":")))
                .replace("/*GLB*/", json.dumps(glbs)))
    with open(os.path.join(viewer, "index.html"), "w") as f:
        f.write(html)

    # Part files for download from the site: one STL and STEP per part, plus zips
    for key in VARIANT_KEYS:
        src = os.path.join(here, "out", key)
        dst = os.path.join(here, "site", "public", "files", key)
        os.makedirs(dst, exist_ok=True)
        rep = reports[key]
        with zipfile.ZipFile(os.path.join(dst, f"{key}-all-parts.zip"), "w", zipfile.ZIP_DEFLATED) as allz:
            for n in rep["parts"]:
                for ext in ("stl", "step"):
                    shutil.copy(os.path.join(src, f"{n}.{ext}"), os.path.join(dst, f"{n}.{ext}"))
                    allz.write(os.path.join(src, f"{n}.{ext}"), f"{key}/{n}.{ext}")
        printable = [n for n, p in rep["parts"].items() if p.get("printable")]
        if printable:
            with zipfile.ZipFile(os.path.join(dst, f"{key}-prints.zip"), "w", zipfile.ZIP_DEFLATED) as pz:
                for n in printable:
                    pz.write(os.path.join(src, f"{n}.stl"), f"{n}.stl")
                pz.write(os.path.join(here, "README.md"), "README.md")

    # The same page as a standalone site (Cloudflare Workers static assets; see site/README.md)
    head_end = html.index("</style>") + len("</style>")
    site = os.path.join(here, "site", "public")
    os.makedirs(site, exist_ok=True)
    with open(os.path.join(site, "index.html"), "w") as f:
        f.write(
            "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, viewport-fit=cover\">\n"
            "<meta name=\"description\" content=\"An aluminium-and-glass bedside alarm clock, from breadboard "
            "prototype to enclosure: interactive 3D model, fit checks and costs.\">\n"
            "<link rel=\"icon\" href=\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E"
            "%3Crect x='3' y='7' width='26' height='16' rx='2' fill='%231b1a1d'/%3E%3Ctext x='16' y='19.5' font-size='9' "
            "text-anchor='middle' fill='%23e0142c' font-family='monospace'%3E7:41%3C/text%3E"
            "%3Crect x='3' y='24' width='26' height='3' fill='%23e0142c' opacity='.6'/%3E%3C/svg%3E\">\n"
            "<style>:root{color-scheme:light}body{margin:0}img{max-width:100%}[hidden]{display:none!important}</style>\n"
            + html[:head_end] + "\n</head>\n<body>\n" + html[head_end:] + "\n</body>\n</html>\n")


if __name__ == "__main__":
    write_viewer()
