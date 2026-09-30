// Firmware tests: the real src/main.cpp, run on a PC through test/host/.
// Each test boots the clock, drives it with taps, the button, serial commands
// and the passing of time, and checks what it did.
//
//   see .github/workflows/ci.yml for the build line

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "alarm_face.hpp"
#include "host.hpp"

using host::st;

namespace {

int failures = 0;
std::string current;

void check(bool ok, const std::string& what) {
  if (!ok) {
    std::fprintf(stderr, "FAIL [%s] %s\n", current.c_str(), what.c_str());
    ++failures;
  }
}

void no_bad_draws(const std::string& where) {
  if (!st.bad.empty()) {
    check(false, where + ": " + std::to_string(st.bad.size()) + " bad calls, first: " + st.bad[0]);
    st.bad.clear();
  }
}

Ui ui() { return ui_for(st.w, st.h); }

void center_tap(Box b) { host::tap(b.x + b.w / 2, b.y + b.h / 2); }

std::string screen() { return host::jget(host::state(), "screen"); }

void fresh(int w, int h) {
  st.prefs.clear();
  host::boot(w, h);
  host::frames(3);
}

// Opens a settings page from the clock face.
void open(Screen s) {
  const Ui u = ui();
  if (screen() != "clock") {
    for (int i = 0; i < 3 && screen() != "clock"; ++i) center_tap(header_back_box(u));
  }
  center_tap(clock_icon_box(u, ClockHit::Gear));
  for (int i = 0; i < kMenuItems; ++i) {
    if (menu_item(i) == s) {
      center_tap(tile_box(u, i, kMenuItems));
      return;
    }
  }
  if (s == Screen::NightHours) {
    open(Screen::NightScreen);
    RowKind kinds[kMaxRows];
    const int n = page_rows(Screen::NightScreen, kinds);
    center_tap(row_box(u, 2, n));
  }
}

Box part(Screen s, int row, Part p) {
  RowKind kinds[kMaxRows];
  const int n = page_rows(s, kinds);
  return row_part_box(ui(), row_box(ui(), row, n), p);
}

const std::vector<std::pair<int, int>>& sizes() {
  static const std::vector<std::pair<int, int>> all = {
      {320, 240}, {616, 284}, {600, 450}, {1232, 568}, {480, 320},
      {240, 240}, {280, 240}, {536, 240}, {800, 480}, {160, 128}};
  return all;
}

// ---------------------------------------------------------------------------

void boots_to_a_clock_face_at_every_size() {
  for (auto wh : sizes()) {
    current = "boot " + std::to_string(wh.first) + "x" + std::to_string(wh.second);
    st.prefs.clear();
    host::boot(wh.first, wh.second);
    host::clear_frame_record();
    host::frames(2);
    check(host::drew_prefix("07:41"), "draws the time");
    check(screen() == "clock", "starts on the clock");
    check(host::lamp_duty() == 0, "lamp starts off");
    check(!st.prefs.empty(), "first boot saves settings");
    no_bad_draws("boot");
  }
}

void every_page_and_control_at_every_size() {
  const Screen pages[] = {Screen::Alarm,     Screen::AlarmSound,  Screen::SleepSounds,
                          Screen::Lamp,      Screen::DayScreen,   Screen::NightScreen,
                          Screen::NightHours};
  for (auto wh : sizes()) {
    current = "pages " + std::to_string(wh.first) + "x" + std::to_string(wh.second);
    fresh(wh.first, wh.second);
    const Ui u = ui();
    center_tap(clock_icon_box(u, ClockHit::Gear));
    check(screen() == "settings", "gear opens settings");
    for (int i = 0; i < kMenuItems; ++i) {
      host::clear_frame_record();
      host::frames(1);
    }
    no_bad_draws("settings menu");
    for (Screen s : pages) {
      open(s);
      check(screen() == screen_key(s), std::string("opens ") + screen_key(s));
      RowKind kinds[kMaxRows];
      const int n = page_rows(s, kinds);
      for (int r = 0; r < n; ++r) {
        const Box row = row_box(u, r, n);
        if (kinds[r] == RowKind::Stepper) {
          center_tap(row_part_box(u, row, Part::Plus));
          center_tap(row_part_box(u, row, Part::Minus));
        } else if (kinds[r] == RowKind::Pair) {
          center_tap(row_part_box(u, row, Part::Left));
          center_tap(row_part_box(u, row, Part::Right));
        } else if (kinds[r] != RowKind::Link) {
          center_tap(row);
          center_tap(row);
        }
      }
      no_bad_draws(screen_key(s));
      center_tap(header_back_box(u));
      check(screen() == screen_key(parent_screen(s)),
            std::string("back from ") + screen_key(s));
    }
    for (int i = 0; i < 3 && screen() != "clock"; ++i) center_tap(header_back_box(u));
    check(screen() == "clock", "Back leads home to the clock");
    no_bad_draws("pages");
  }
}

// Every piece of text sits inside its row or tile, and on the screens the
// clock is designed for nothing needs cutting short.
void text_fits() {
  for (auto wh : sizes()) {
    current = "text fit " + std::to_string(wh.first) + "x" + std::to_string(wh.second);
    const bool target = (wh.first == 320 && wh.second == 240) ||
                        (wh.first == 616 && wh.second == 284) ||
                        (wh.first == 600 && wh.second == 450);
    fresh(wh.first, wh.second);
    const Ui u = ui();
    auto no_cuts = [&](const char* where) {
      if (!target) return;
      for (const host::Text& t : st.texts) {
        check(t.s.size() < 2 || t.s.compare(t.s.size() - 2, 2, "..") != 0,
              std::string(where) + ": \"" + t.s + "\" was cut short");
      }
    };
    center_tap(clock_icon_box(u, ClockHit::Gear));
    host::command("sound on");  // longest tile text: "On, 30 min left"
    host::command("lamp on");
    for (const host::Text& t : st.texts) {
      if (t.y + t.h <= u.header) continue;
      bool inside = false;
      for (int i = 0; i < kMenuItems; ++i) {
        const Box b = tile_box(u, i, kMenuItems);
        inside = inside || (t.x >= b.x && t.x + t.w <= b.x + b.w && t.y >= b.y - 1 &&
                            t.y + t.h <= b.y + b.h + 1);
      }
      check(inside, "settings: \"" + t.s + "\" spills out of its tile");
    }
    no_cuts("settings");
    host::command("sound off");
    const Screen pages[] = {Screen::Alarm, Screen::AlarmSound, Screen::SleepSounds, Screen::Lamp,
                            Screen::DayScreen, Screen::NightScreen, Screen::NightHours};
    for (Screen s : pages) {
      open(s);
      host::frames(2);
      RowKind kinds[kMaxRows];
      const int n = page_rows(s, kinds);
      for (const host::Text& t : st.texts) {
        if (t.y + t.h <= u.header) continue;
        bool inside = false;
        for (int r = 0; r < n; ++r) {
          const Box row = row_box(u, r, n);
          inside = inside || (t.x >= row.x && t.x + t.w <= row.x + row.w && t.y >= row.y - 1 &&
                              t.y + t.h <= row.y + row.h + 1);
        }
        check(inside, std::string(screen_key(s)) + ": \"" + t.s + "\" spills out of its row");
      }
      no_cuts(screen_key(s));
    }
    // The clock face with everything showing
    for (int i = 0; i < 3 && screen() != "clock"; ++i) center_tap(header_back_box(u));
    host::command("alarm 07:50");
    host::command("sound on");
    host::command("time 12:00:00");
    host::frames(2);
    no_cuts("clock");
    no_bad_draws("text fit");
  }
}

void alarm_set_by_touch_rings_and_stops() {
  current = "alarm by touch";
  fresh(320, 240);
  open(Screen::Alarm);
  center_tap(part(Screen::Alarm, 0, Part::Whole));  // on
  center_tap(part(Screen::Alarm, 1, Part::Plus));   // 8
  center_tap(part(Screen::Alarm, 2, Part::Plus));   // 8:01
  const std::string s = host::state();
  check(host::jget(s, "alarm", "at") == "08:01", "alarm set to 08:01, got " + host::jget(s, "alarm", "at"));
  check(host::jget(s, "alarm", "enabled") == "true", "alarm enabled");
  host::set_clock(8, 0, 58);
  host::frames(200);  // ~3 s
  check(host::jget(host::state(), "alarm", "state") == "ringing", "rings at 08:01");
  check(screen() == "clock", "ringing shows the clock");
  size_t tones = st.tones.size();
  check(tones > 0, "beeps");
  for (const host::Tone& t : st.tones) {
    check(t.channel == 0, "alarm beeps on channel 0");
    check(t.freq == 440, "gentle first");
  }
  host::clear_frame_record();
  host::frames(1);
  center_tap(ring_box(ui(), true));
  check(host::jget(host::state(), "alarm", "state") == "silenced", "Stop silences");
  tones = st.tones.size();
  host::frames(100);
  check(st.tones.size() == tones, "no beeps after Stop");
  no_bad_draws("alarm");
}

void alarm_goes_loud_then_snoozes_by_button() {
  current = "snooze by button";
  fresh(616, 284);
  host::command("alarm 06:30");
  host::command("time 06:30:00");
  host::frames(40 * 60, 16);  // ~38 s
  bool loud = false;
  for (const host::Tone& t : st.tones) loud = loud || t.freq == 880 || t.freq == 1760;
  check(loud, "goes loud after the soft seconds");
  host::button();
  const std::string s = host::state();
  check(host::jget(s, "alarm", "state") == "silenced", "button snoozes");
  check(host::jget(s, "alarm", "snooze") == "06:39", "snoozed to 06:39, got " + host::jget(s, "alarm", "snooze"));
  host::set_clock(6, 38, 59);
  host::frames(120);
  check(host::jget(host::state(), "alarm", "state") == "ringing", "rings again after snooze");
  host::command("alarm off");
  check(host::jget(host::state(), "alarm", "state") == "armed", "turning the alarm off stops it");
}

void ringing_pulls_you_out_of_the_menu() {
  current = "ring from menu";
  fresh(320, 240);
  host::command("alarm 07:42");
  open(Screen::Lamp);
  host::set_clock(7, 41, 59);
  host::frames(120);
  check(screen() == "clock", "alarm jumps back to the clock so Stop is on screen");
}

void settings_idle_back_to_clock() {
  current = "idle";
  fresh(320, 240);
  open(Screen::DayScreen);
  host::frames(59000 / 16);
  check(screen() == "day_screen", "stays a minute");
  host::frames(3000 / 16);
  check(screen() == "clock", "goes back to the clock after a minute idle");
}

void skipped_alarm_stays_quiet_and_rearms() {
  current = "skip";
  fresh(320, 240);
  host::command("alarm 07:45");
  center_tap(clock_icon_box(ui(), ClockHit::Skip));
  check(host::jget(host::state(), "alarm", "skip_next") == "true", "skip icon sets skip");
  host::set_clock(7, 44, 59);
  const size_t tones = st.tones.size();
  host::frames(200);
  check(st.tones.size() == tones, "skipped alarm makes no sound");
  const std::string s = host::state();
  check(host::jget(s, "alarm", "state") == "skipped", "state skipped");
  check(host::jget(s, "alarm", "skip_next") == "false", "skip clears after use");
}

void holding_a_stepper_repeats() {
  current = "hold repeat";
  fresh(320, 240);
  open(Screen::Alarm);
  const Box plus = part(Screen::Alarm, 2, Part::Plus);
  host::hold(plus.x + plus.w / 2, plus.y + plus.h / 2, 1500);
  const int minute = std::atoi(host::jget(host::state(), "alarm", "at").substr(3).c_str());
  check(minute >= 8 && minute <= 14, "1.5 s hold moves the minute by about 10, got " + std::to_string(minute));
  // Sliding onto a stepper from elsewhere doesn't start repeating it.
  const Box row = row_box(ui(), 2, 4);
  host::touch(row.x + 4, row.y + row.h / 2, true);
  host::frames(3);
  host::touch(plus.x + plus.w / 2, plus.y + plus.h / 2, true);
  host::frames(60);
  host::touch(0, 0, false);
  host::frames(2);
  const int after = std::atoi(host::jget(host::state(), "alarm", "at").substr(3).c_str());
  check(after == minute, "a drag onto + doesn't count");
}

void sleep_sounds_stream_fade_and_time_out() {
  current = "sleep sounds";
  fresh(320, 240);
  open(Screen::SleepSounds);
  center_tap(part(Screen::SleepSounds, 2, Part::Minus));  // 30 -> 15 min
  center_tap(part(Screen::SleepSounds, 3, Part::Whole));  // play
  std::string s = host::state();
  check(host::jget(s, "sound", "playing") == "true", "Play starts it");
  check(host::jget(s, "sound", "timer") == "15", "timer 15");
  host::frames(300);  // ~5 s
  check(!st.raws.empty(), "streams noise");
  double first_rms = -1, late_rms = 0;
  for (const host::Raw& r : st.raws) {
    check(r.channel == 1, "noise on channel 1");
    check(r.rate == 24000, "24 kHz");
    if (first_rms < 0) first_rms = r.rms;
    late_rms = r.rms;
  }
  check(first_rms < late_rms * 0.5, "fades in");
  check(st.underruns[1] == 0, "never runs dry");
  check(late_rms > 2000, "audible once faded in");
  no_bad_draws("noise buffers");
  // Volume follows the setting
  const int vol_before = st.raws.back().volume;
  center_tap(part(Screen::SleepSounds, 1, Part::Plus));
  host::frames(30);
  check(st.raws.back().volume > vol_before, "volume + makes it louder");
  // Timer: runs out after 15 minutes, fading over the last minute
  host::frames((14 * 60 + 30) * 1000 / 50, 50);
  check(host::jget(host::state(), "sound", "playing") == "true", "still playing at 14.5 min");
  check(st.raws.back().rms < late_rms * 0.8, "fading out in the last minute");
  host::frames(40 * 1000 / 50, 50);
  check(host::jget(host::state(), "sound", "playing") == "false", "timer stops it");
  const size_t n = st.raws.size();
  host::frames(100);
  check(st.raws.size() == n, "nothing more is streamed");
  no_bad_draws("sleep sounds");
}

// A slow loop (a full redraw, a flash write) mustn't starve the speaker or
// make the firmware reuse a buffer it's still playing.
void sleep_sounds_survive_slow_loops() {
  for (int dt : {40, 90, 120, 150}) {
    current = "slow loops " + std::to_string(dt) + " ms";
    fresh(320, 240);
    host::command("sound on");
    host::frames(4000 / dt + 1, dt);  // past the fade in
    const int before = st.underruns[1];
    host::frames(20000 / dt, dt);
    check(st.underruns[1] == before, "no gaps");
    for (int i = 0; i < 40; ++i) {  // screens changing while it plays
      center_tap(clock_icon_box(ui(), ClockHit::Gear));
      center_tap(header_back_box(ui()));
    }
    no_bad_draws("slow loops");
  }
}

void review_regressions() {
  current = "no phantom stepper press";
  fresh(320, 240);
  open(Screen::Alarm);
  center_tap(part(Screen::Alarm, 1, Part::Plus));
  host::frames(61000 / 16);  // idle back to the clock
  check(screen() == "clock", "idled home");
  const std::string before = host::jget(host::state(), "lamp", "brightness");
  open(Screen::Lamp);
  host::frames(60);
  check(host::jget(host::state(), "lamp", "brightness") == before, "opening a page changes nothing");
  // Holding a finger down across a page change doesn't press what's under it.
  const Box tile = tile_box(ui(), 3, kMenuItems);
  center_tap(header_back_box(ui()));
  host::touch(tile.x + tile.w - 3, tile.y + tile.h / 2, true);
  host::frames(80);
  host::touch(0, 0, false);
  host::frames(2);
  check(host::jget(host::state(), "lamp", "brightness") == before, "a held finger doesn't repeat on the new page");

  current = "can't leave a ringing alarm";
  fresh(320, 240);
  host::command("go alarm");
  host::frames(12000 / 16);
  center_tap(clock_icon_box(ui(), ClockHit::Gear));
  check(screen() == "clock", "the gear does nothing while ringing");

  current = "dismissed sunrise stays dark";
  fresh(320, 240);
  host::command("alarm 07:00");
  host::command("go sunrise");
  host::frames(5 * 60 * 1000 / 250, 250);
  center_tap(clock_icon_box(ui(), ClockHit::Lamp));
  host::set_clock(6, 59, 55);
  host::frames(600);
  check(host::jget(host::state(), "alarm", "state") == "ringing", "rings");
  check(host::lamp_duty() == 0, "no wake light after the sunrise was put out");

  current = "unanswered snooze ends";
  fresh(320, 240);
  host::command("alarm 06:30");
  host::command("time 06:30:00");
  host::frames(60);
  host::button();  // snooze to 06:39
  host::set_clock(6, 31, 0);
  host::frames(3);
  host::set_clock(6, 39, 0);
  host::frames(60);
  check(host::jget(host::state(), "alarm", "state") == "ringing", "snooze rings");
  host::set_clock(6, 40, 1);  // nobody answered
  host::frames(3);
  std::string s = host::state();
  check(host::jget(s, "alarm", "snooze") == "null", "the snooze is used up: " + s);
  host::set_clock(6, 38, 59);  // the next day, just before 06:39
  st.clock_s += 86400;
  host::frames(120);
  check(host::jget(host::state(), "alarm", "state") == "armed", "no ring at 06:39 the next day");

  current = "saves are batched";
  fresh(320, 240);
  const auto saved = st.prefs["alarmclk/cfg"];
  open(Screen::Alarm);
  const Box plus = part(Screen::Alarm, 2, Part::Plus);
  host::hold(plus.x + plus.w / 2, plus.y + plus.h / 2, 1000);
  check(st.prefs["alarmclk/cfg"] == saved, "nothing written while holding +");
  host::frames(2000 / 16);
  check(st.prefs["alarmclk/cfg"] != saved, "written once it settles");
}

void sleep_sounds_stop_when_the_alarm_rings() {
  current = "noise then alarm";
  fresh(320, 240);
  host::command("sound brown");
  check(host::jget(host::state(), "sound", "kind") == "brown", "sound brown picks brown");
  check(host::jget(host::state(), "sound", "playing") == "true", "and plays");
  host::command("alarm 07:43");
  host::set_clock(7, 42, 59);
  host::frames(120);
  const std::string s = host::state();
  check(host::jget(s, "alarm", "state") == "ringing", "rings");
  check(host::jget(s, "sound", "playing") == "false", "noise stops for the alarm");
}

void sound_icon_toggles_noise() {
  current = "sound icon";
  fresh(600, 450);
  center_tap(clock_icon_box(ui(), ClockHit::Sound));
  check(host::jget(host::state(), "sound", "playing") == "true", "icon starts");
  host::clear_frame_record();
  host::frames(2);
  center_tap(clock_icon_box(ui(), ClockHit::Sound));
  check(host::jget(host::state(), "sound", "playing") == "false", "icon stops");
}

void lamp_icon_button_and_commands() {
  current = "lamp";
  fresh(320, 240);
  center_tap(clock_icon_box(ui(), ClockHit::Lamp));
  check(host::lamp_duty() == lamp_duty(400), "icon turns it on at 40%");
  host::button();
  check(host::lamp_duty() == 0, "button turns it off");
  host::button();
  check(host::lamp_duty() > 0, "button turns it on");
  host::command("lamp 100");
  check(host::lamp_duty() == 255, "lamp 100 is full");
  host::command("lamp 0");
  check(host::lamp_duty() == 0, "lamp 0 is off");
  host::command("lamp toggle");
  check(host::lamp_duty() == 255, "toggle back on at the last level");
  open(Screen::Lamp);
  center_tap(part(Screen::Lamp, 1, Part::Minus));
  check(host::lamp_duty() == lamp_duty(950), "brightness - steps 5%");
  center_tap(part(Screen::Lamp, 0, Part::Whole));
  check(host::lamp_duty() == 0, "switch turns it off");
  no_bad_draws("lamp");
}

void sunrise_ramps_the_lamp_before_the_alarm() {
  current = "sunrise";
  fresh(320, 240);
  host::command("alarm 07:00");
  open(Screen::Lamp);
  for (int i = 0; i < 3; ++i) center_tap(part(Screen::Lamp, 2, Part::Plus));  // 20 min
  check(host::jget(host::state(), "lamp", "sunrise") == "20", "sunrise 20 min");
  center_tap(header_back_box(ui()));
  center_tap(header_back_box(ui()));
  host::set_clock(6, 39, 0);
  host::frames(2);
  check(host::lamp_duty() == 0, "dark before the sunrise starts");
  int last = 0;
  bool rising = true;
  for (int m = 0; m < 21; ++m) {
    host::frames(60000 / 250, 250);
    const int d = host::lamp_duty();
    rising = rising && d >= last;
    last = d;
  }
  check(rising, "only ever gets brighter");
  check(host::jget(host::state(), "alarm", "state") == "ringing", "alarm rings at 07:00");
  check(host::lamp_duty() == 255, "full at the alarm");
  center_tap(ring_box(ui(), true));
  host::frames(60);
  check(host::lamp_duty() == 255, "stays on after Stop, as a wake light");
  host::set_clock(7, 29, 0);
  host::frames(3);
  check(host::lamp_duty() == 255, "still on at 07:29");
  host::set_clock(7, 30, 0);
  host::frames(3);
  check(host::lamp_duty() == 0, "off 30 minutes after the alarm");
}

void sunrise_can_be_dismissed_and_skips_with_the_alarm() {
  current = "sunrise dismiss";
  fresh(320, 240);
  host::command("alarm 07:00");
  host::command("go sunrise");
  host::frames(60 * 5 * 1000 / 250, 250);
  check(host::lamp_duty() > 0, "go sunrise starts the ramp");
  center_tap(clock_icon_box(ui(), ClockHit::Lamp));
  check(host::lamp_duty() == 0, "the lamp icon puts the sunrise out");
  host::frames(60 * 1000 / 250, 250);
  check(host::lamp_duty() == 0, "and it stays out");
  // Next day, skip the alarm: no sunrise either
  host::command("time 06:40");
  host::frames(2);
  center_tap(clock_icon_box(ui(), ClockHit::Skip));
  host::command("time 06:55");
  host::frames(2);
  check(host::lamp_duty() == 0, "no sunrise before a skipped alarm");
}

void go_commands_travel_in_time() {
  current = "go";
  fresh(616, 284);
  host::command("go day");
  host::frames(2);
  std::string s = host::state();
  check(host::jget(s, "time").substr(0, 5) == "12:00", "go day is noon");
  check(host::jget(s, "night") == "false", "daytime look");
  const int day_brightness = st.brightness;
  host::command("go night");
  host::frames(2);
  s = host::state();
  check(host::jget(s, "night") == "true", "go night is night, got " + s);
  check(st.brightness < day_brightness, "dimmer at night");
  host::command("go alarm");
  host::frames(12 * 1000 / 16);
  check(host::jget(host::state(), "alarm", "state") == "ringing", "go alarm rings within 10 s");
}

void serial_commands_answer() {
  current = "serial";
  fresh(320, 240);
  check(host::command("help").find("lamp") != std::string::npos, "help lists commands");
  check(host::command("bogus").find("?") == 0, "unknown command says so");
  check(host::command("time 25:00") .find("?") == 0, "bad time rejected");
  const std::string s = host::command("state");
  check(s.find("{\"time\":") == 0, "state prints JSON");
  check(host::command("time 23:59:30").find("ok") == 0, "time accepted");
  host::frames(2);
  check(host::clock_hour() == 23 && host::clock_minute() == 59, "clock set");
  // A long garbage line is dropped whole and the next command still works.
  host::command(std::string(300, 'x'));
  check(host::command("lamp on").find("ok") == 0, "recovers after an overlong line");
}

void settings_survive_a_reboot() {
  for (auto wh : sizes()) {
    current = "reboot " + std::to_string(wh.first) + "x" + std::to_string(wh.second);
    fresh(wh.first, wh.second);
    host::command("alarm 05:55");
    host::command("lamp 35");
    host::command("sound white");
    open(Screen::DayScreen);
    center_tap(part(Screen::DayScreen, 2, Part::Whole));  // 12-hour
    host::frames(2000 / 16);  // saves settle
    host::boot(wh.first, wh.second);
    host::frames(2);
    const std::string s = host::state();
    check(host::jget(s, "alarm", "at") == "05:55", "alarm kept");
    check(host::jget(s, "alarm", "enabled") == "true", "alarm on kept");
    check(host::jget(s, "lamp", "on") == "true" && host::jget(s, "lamp", "brightness") == "35",
          "lamp kept");
    check(host::jget(s, "sound", "kind") == "white", "sound kept");
    check(host::jget(s, "sound", "playing") == "false", "but not playing after a reboot");
    check(host::lamp_duty() == lamp_duty(350), "lamp comes back on");
    check(host::drew("AM") || host::drew("PM"), "12-hour kept");
  }
}

void version1_settings_are_upgraded() {
  current = "v1 upgrade";
  SettingsV1 old;
  std::memset(&old, 0, sizeof(old));
  old.magic = kSettingsMagic;
  old.version = 1;
  old.alarm_hour = 6;
  old.alarm_minute = 15;
  old.alarm_enabled = 1;
  old.volume = 60;
  old.soft_volume = 20;
  old.gentle_seconds = 10;
  old.day_format = 2;
  old.night_format = 0;
  old.night_start_hour = 22;
  old.night_end_hour = 6;
  old.day_bright = 50;
  old.night_bright = 10;
  old.hour12 = 1;
  st.prefs.clear();
  const uint8_t* b = reinterpret_cast<const uint8_t*>(&old);
  st.prefs["alarmclk/cfg"] = std::vector<uint8_t>(b, b + sizeof(old));
  host::boot(320, 240);
  host::frames(2);
  const std::string s = host::state();
  check(host::jget(s, "alarm", "at") == "06:15", "alarm time carried over");
  check(host::jget(s, "sound", "kind") == "pink", "new settings get defaults");
  check(st.prefs["alarmclk/cfg"].size() == sizeof(Settings), "rewritten as version 2");
}

void corrupt_settings_fall_back_to_defaults() {
  current = "corrupt";
  const std::vector<std::vector<uint8_t>> junk = {
      {}, {1, 2, 3}, std::vector<uint8_t>(32, 0xFF), std::vector<uint8_t>(24, 0xFF),
      std::vector<uint8_t>(33, 0)};
  for (const auto& j : junk) {
    st.prefs.clear();
    st.prefs["alarmclk/cfg"] = j;
    host::boot(320, 240);
    host::frames(2);
    const std::string s = host::state();
    check(host::jget(s, "alarm", "at") == "07:00", "defaults after junk of " + std::to_string(j.size()));
    Settings saved;
    check(load_settings(st.prefs["alarmclk/cfg"].data(), st.prefs["alarmclk/cfg"].size(), &saved),
          "writes good settings back");
  }
}

void night_look_is_red_and_dim() {
  current = "night look";
  fresh(320, 240);
  host::command("time 23:00");
  host::frames(2);
  bool red = false;
  for (const host::Text& t : st.texts) red = red || t.color == 0xF800;
  check(red, "red digits at night");
  const int night = st.brightness;
  host::command("time 12:00");
  host::frames(2);
  bool white = false;
  for (const host::Text& t : st.texts) white = white || t.color == 0xFFFF;
  check(white, "white by day");
  check(st.brightness > night, "brighter by day");
  open(Screen::NightScreen);
  host::frames(2);
  check(st.brightness == night, "the night page previews night brightness");
}

// Thousands of random taps, holds, drags, button presses, commands and time
// jumps; the firmware must never draw off screen or save bad settings.
void random_abuse() {
  const char* cmds[] = {"lamp toggle", "sound toggle", "go day", "go night", "go alarm",
                        "go sunrise", "button", "state", "sound pink", "alarm 07:42",
                        "time 07:41:55", "lamp 3", "alarm off"};
  for (auto wh : sizes()) {
    current = "random " + std::to_string(wh.first) + "x" + std::to_string(wh.second);
    fresh(wh.first, wh.second);
    uint32_t seed = 12345u + static_cast<uint32_t>(wh.first * 7 + wh.second);
    auto rnd = [&seed]() {
      seed = seed * 1103515245u + 12345u;
      return (seed >> 8) / 16777216.0;
    };
    bool down = false;
    int x = 0, y = 0, left = 0;
    for (int i = 0; i < 12000; ++i) {
      const double r = rnd();
      if (r < 0.002) {
        st.clock_s += static_cast<int64_t>(rnd() * 86400);
      } else if (r < 0.004) {
        host::command(cmds[static_cast<int>(rnd() * 13) % 13]);
      } else if (r < 0.005) {
        host::button();
      }
      if (!down && rnd() < 0.08) {
        down = true;
        x = static_cast<int>(rnd() * wh.first);
        y = static_cast<int>(rnd() * wh.second);
        left = static_cast<int>(rnd() * (rnd() < 0.2 ? 90 : 6));
      } else if (down && left-- <= 0) {
        down = false;
      } else if (down && rnd() < 0.1) {
        x = std::min(wh.first - 1, std::max(0, x + static_cast<int>(rnd() * 21) - 10));
      }
      host::touch(x, y, down);
      host::frames(1, rnd() < 0.3 ? 400 : 16);
      if (!st.bad.empty()) break;
    }
    no_bad_draws("random input");
    Settings saved;
    check(load_settings(st.prefs["alarmclk/cfg"].data(), st.prefs["alarmclk/cfg"].size(), &saved),
          "saved settings stay valid");
    const int duty = host::lamp_duty();
    check(duty >= 0 && duty <= 255, "lamp duty in range");
  }
}

}  // namespace

int main() {
  boots_to_a_clock_face_at_every_size();
  every_page_and_control_at_every_size();
  text_fits();
  alarm_set_by_touch_rings_and_stops();
  alarm_goes_loud_then_snoozes_by_button();
  ringing_pulls_you_out_of_the_menu();
  settings_idle_back_to_clock();
  skipped_alarm_stays_quiet_and_rearms();
  holding_a_stepper_repeats();
  sleep_sounds_stream_fade_and_time_out();
  sleep_sounds_stop_when_the_alarm_rings();
  sleep_sounds_survive_slow_loops();
  review_regressions();
  sound_icon_toggles_noise();
  lamp_icon_button_and_commands();
  sunrise_ramps_the_lamp_before_the_alarm();
  sunrise_can_be_dismissed_and_skips_with_the_alarm();
  go_commands_travel_in_time();
  serial_commands_answer();
  settings_survive_a_reboot();
  version1_settings_are_upgraded();
  corrupt_settings_fall_back_to_defaults();
  night_look_is_red_and_dim();
  random_abuse();
  if (failures) {
    std::fprintf(stderr, "%d firmware checks failed\n", failures);
    return 1;
  }
  std::printf("firmware tests passed\n");
  return 0;
}
