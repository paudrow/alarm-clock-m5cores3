// Entry points for the firmware simulator. JavaScript (or a native test, see
// test/host/) calls sim_init once, then sim_touch and sim_loop every frame;
// src/main.cpp's own setup() and loop() do the rest.

#include "M5Unified.h"

void setup();
void loop();
int write_state_json(char* out, size_t n);

m5::M5Unified M5;
SimSerial Serial;

namespace {
int touch_x = 0;
int touch_y = 0;
bool touch_down = false;
bool button_clicked = false;
char io[1024];
}  // namespace

void m5::M5Unified::update() {
  Touch.latch(touch_x, touch_y, touch_down);
  BtnPWR.latch(button_clicked);
  button_clicked = false;
}

#if defined(__wasm__)
#define SIM_EXPORT(name) extern "C" __attribute__((export_name(#name)))
#else
#define SIM_EXPORT(name) extern "C"
#endif

// Boots (or reboots) the clock with a w x h screen. Settings come back from
// storage, as after a power cut.
SIM_EXPORT(sim_init) void sim_init(int w, int h) {
  touch_x = touch_y = 0;
  touch_down = false;
  button_clicked = false;
  M5.Touch.reset();
  M5.BtnPWR.latch(false);
  Serial.clear();
  M5.Display.setResolution(w, h);
  setup();
}

SIM_EXPORT(sim_loop) void sim_loop() { loop(); }

SIM_EXPORT(sim_touch) void sim_touch(int x, int y, int down) {
  touch_x = x;
  touch_y = y;
  touch_down = down != 0;
}

// Presses the physical button once.
SIM_EXPORT(sim_button) void sim_button() { button_clicked = true; }

// A scratch buffer for passing text in and out.
SIM_EXPORT(sim_io) char* sim_io() { return io; }

// Types the first n bytes of the io buffer, plus a newline, on the serial
// console: the same commands work over USB on the clock ("help" lists them).
SIM_EXPORT(sim_input) void sim_input(int n) {
  if (n < 0) n = 0;
  if (n > static_cast<int>(sizeof io)) n = sizeof io;
  Serial.feed(io, n);
  Serial.feed("\n", 1);
}

// Writes the clock's state as JSON into the io buffer; returns its length.
SIM_EXPORT(sim_state) int sim_state() {
  const int n = write_state_json(io, sizeof io);
  return n < 0 ? 0 : (n < static_cast<int>(sizeof io) ? n : static_cast<int>(sizeof io) - 1);
}
