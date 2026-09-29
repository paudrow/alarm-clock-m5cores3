// Browser entry points for the firmware simulator. JavaScript calls sim_init
// once, then sim_touch and sim_loop every animation frame; src/main.cpp's own
// setup() and loop() do the rest.

#include "M5Unified.h"

void setup();
void loop();

m5::M5Unified M5;
SimSerial Serial;

namespace {
int touch_x = 0;
int touch_y = 0;
bool touch_down = false;
}  // namespace

void m5::M5Unified::update() { Touch.latch(touch_x, touch_y, touch_down); }

#define SIM_EXPORT(name) extern "C" __attribute__((export_name(#name)))

SIM_EXPORT(sim_init) void sim_init(int w, int h) {
  M5.Display.setResolution(w, h);
  setup();
}

SIM_EXPORT(sim_loop) void sim_loop() { loop(); }

SIM_EXPORT(sim_touch) void sim_touch(int x, int y, int down) {
  touch_x = x;
  touch_y = y;
  touch_down = down != 0;
}
