"""Build the firmware simulator: src/main.cpp + include/ + sim/ -> sim/clock.wasm.

Compiles the real firmware for the browser (WebAssembly), against the stand-in
M5Unified and Preferences headers in sim/shim/. Needs Zig's C++ toolchain:

    pip install ziglang==0.16.0   # pinned: CI checks the output is byte-identical
    python sim/build.py

hardware/cad/viewer.py embeds sim/clock.wasm in the 3D viewer, where it drives
the clock's touchscreen.
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE_DATE_EPOCH = "1767225600"  # 2026-01-01 00:00:00 UTC
ROOT = os.path.dirname(HERE)


def main():
    out = os.path.join(HERE, "clock.wasm")
    cmd = [
        sys.executable, "-m", "ziglang", "c++",
        "-target", "wasm32-wasi",
        "-mexec-model=reactor",   # a library the page calls into, not a program with main()
        "-std=gnu++17", "-O2", "-s",
        "-fno-exceptions", "-fno-rtti",
        "-Wall", "-Wno-nullability-completeness",
        "-DSIM_REPRODUCIBLE=1",   # part of Zig's cache key, so older time-stamped builds aren't reused
        "-Wno-date-time",         # main.cpp stamps a blank RTC with __DATE__ / __TIME__ (pinned below)
        "-I", os.path.join(HERE, "shim"),
        "-I", os.path.join(ROOT, "include"),
        os.path.join(ROOT, "src", "main.cpp"),
        os.path.join(HERE, "sim.cpp"),
        "-o", out,
    ]
    # Pin __DATE__ / __TIME__ so the same source always gives the same clock.wasm; CI
    # rebuilds it and checks it matches the committed file. The stamp only sets a blank
    # RTC, and the simulator's RTC always has the time, so its value doesn't matter.
    env = dict(os.environ, SOURCE_DATE_EPOCH=SOURCE_DATE_EPOCH)
    result = subprocess.run(cmd, env=env, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stderr)
        sys.exit("sim/build.py: the firmware didn't build for the browser (see above).")
    # On a cold cache Zig also compiles its own libc++, which is noisy; show only our warnings.
    ours = [line for line in result.stderr.splitlines()
            if "warning" in line and any(d in line for d in (os.path.join(ROOT, "src"), os.path.join(ROOT, "include"), HERE))]
    for line in ours:
        print(line, file=sys.stderr)
    print(f"wrote {os.path.relpath(out, ROOT)} ({os.path.getsize(out) // 1024} KB)")


if __name__ == "__main__":
    main()
