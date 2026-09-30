// Pictures of every screen, drawn by the real firmware (src/main.cpp through
// test/host/) at the three screen sizes the clock is designed for. CI renders
// them again and fails if they differ from the committed test/screens/, so a
// layout change shows up in review as changed pictures.
//
// The text is a blocky stand-in font with the real layout's sizes, so these
// show where things are, not what the fonts look like. The simulator in the
// 3D viewer (sim/) shows the real thing.

#include <cstdio>
#include <string>

#include "alarm_face.hpp"
#include "host.hpp"

using host::st;

namespace {

std::string dir;
int failures = 0;

void save(const std::string& name) {
  host::frames(2);
  const std::string path = dir + "/" + name + ".png";
  if (!host::write_png(path)) {
    std::fprintf(stderr, "couldn't write %s\n", path.c_str());
    failures = 1;
  }
}

void tap_box(Box b) { host::tap(b.x + b.w / 2, b.y + b.h / 2); }

void home(const Ui& u) {
  for (int i = 0; i < 3 && host::jget(host::state(), "screen") != "clock"; ++i)
    tap_box(header_back_box(u));
}

void open(const Ui& u, Screen s) {
  home(u);
  tap_box(clock_icon_box(u, ClockHit::Gear));
  if (s == Screen::Settings) return;
  if (s == Screen::NightHours) {
    open(u, Screen::NightScreen);
    RowKind kinds[kMaxRows];
    tap_box(row_box(u, 2, page_rows(Screen::NightScreen, kinds)));
    return;
  }
  if (s == Screen::WindDown || s == Screen::WakeUp) {
    open(u, Screen::Lamp);
    RowKind kinds[kMaxRows];
    tap_box(row_box(u, s == Screen::WindDown ? 2 : 3, page_rows(Screen::Lamp, kinds)));
    return;
  }
  for (int i = 0; i < kMenuItems; ++i)
    if (menu_item(i) == s) tap_box(tile_box(u, i, kMenuItems));
}

void render(int w, int h) {
  const std::string size = std::to_string(w) + "x" + std::to_string(h);
  st.prefs.clear();
  host::boot(w, h, 2026, 9, 29, 12, 0, 0);
  const Ui u = ui_for(w, h);
  host::command("alarm 07:00");
  host::frames(2);
  save(size + "-clock-day");

  host::command("sound pink");
  host::command("lamp on");
  host::command("time 22:00:00");
  save(size + "-clock-night");

  host::command("sound off");
  host::command("lamp off");
  host::command("time 06:59:55");
  host::frames(6 * 1000 / 16);
  save(size + "-clock-ringing");
  tap_box(ring_box(u, true));

  host::command("time 12:00:00");
  const Screen pages[] = {Screen::Settings,    Screen::Alarm, Screen::AlarmSound,
                          Screen::SleepSounds, Screen::Lamp,  Screen::DayScreen,
                          Screen::NightScreen, Screen::NightHours, Screen::WindDown,
                          Screen::WakeUp};
  for (Screen s : pages) {
    open(u, s);
    save(size + "-" + screen_key(s));
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: render_screens directory\n");
    return 1;
  }
  dir = argv[1];
  render(320, 240);
  render(616, 284);
  render(600, 450);
  if (!st.bad.empty()) {
    std::fprintf(stderr, "drew off screen: %s\n", st.bad[0].c_str());
    return 1;
  }
  return failures;
}
