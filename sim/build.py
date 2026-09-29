"""Build the firmware simulator: src/main.cpp + include/ + sim/ -> sim/clock.wasm.

Compiles the real firmware for the browser (WebAssembly), against the stand-in
M5Unified and Preferences headers in sim/shim/. Needs Zig's C++ toolchain:

    pip install ziglang
    python sim/build.py

hardware/cad/viewer.py embeds sim/clock.wasm in the 3D viewer, where it drives
the clock's touchscreen.
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
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
        "-Wno-date-time",         # main.cpp stamps a blank RTC with __DATE__ / __TIME__
        "-I", os.path.join(HERE, "shim"),
        "-I", os.path.join(ROOT, "include"),
        os.path.join(ROOT, "src", "main.cpp"),
        os.path.join(HERE, "sim.cpp"),
        "-o", out,
    ]
    subprocess.run(cmd, check=True)
    print(f"wrote {os.path.relpath(out, ROOT)} ({os.path.getsize(out) // 1024} KB)")


if __name__ == "__main__":
    main()
