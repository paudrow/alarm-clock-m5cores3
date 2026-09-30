#ifndef ALARM_FACE_HPP
#define ALARM_FACE_HPP

// Everything the clock decides, with no hardware: the alarm's state machine,
// time formatting, the screen layout and what a touch hits, the sleep-sound
// generator, the lamp and sunrise, serial commands, and the saved settings.
// src/main.cpp draws and drives the hardware from these; the tests in test/
// check them on a PC.

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdint.h>

struct Hm {
  int hour;
  int minute;
};

struct Alarm {
  Hm at;
  bool enabled;
  bool skip_next;
};

enum class Occurrence {
  Armed,
  Ringing,
  Silenced,
  Skipped,
};

enum class Intensity {
  Silent,
  Gentle,
  Loud,
};

enum class TimeFormat {
  Hidden,
  Hours,
  HoursMinutes,
  HoursMinutesSeconds,
};

enum class HourCycle { H24, H12 };

struct Inputs {
  Hm now;
  int second;
  bool touched;
  int gentle_seconds;
  bool snooze;
  bool snooze_armed;
  Hm snooze_at;
};

struct Step {
  Occurrence occurrence;
  Intensity intensity;
  bool skip_next;
  bool snooze_armed;
  Hm snooze_at;
};

constexpr int kGentleSeconds = 30;
constexpr int kSnoozeMinutes = 9;

enum class Zone {
  Clock,
  Hour,
  HourDown,
  Minute,
  MinuteDown,
  Toggle,
};

struct YmdHms {
  int year;
  int month;
  int date;
  int hour;
  int minute;
  int second;
};

enum class ColorMode { Auto, Day, Night };

inline const char* color_mode_label(ColorMode mode) {
  switch (mode) {
    case ColorMode::Day:
      return "Never";
    case ColorMode::Night:
      return "Always";
    case ColorMode::Auto:
      return "Scheduled";
  }
  return "Scheduled";
}

inline ColorMode next_color_mode(ColorMode mode) {
  switch (mode) {
    case ColorMode::Auto:
      return ColorMode::Day;
    case ColorMode::Day:
      return ColorMode::Night;
    case ColorMode::Night:
      return ColorMode::Auto;
  }
  return ColorMode::Auto;
}

inline Hm step_minutes(Hm t, int delta);
inline int minutes_until(Hm from, Hm to);

inline bool hm_equal(Hm a, Hm b) {
  return a.hour == b.hour && a.minute == b.minute;
}

inline Intensity ring_intensity(int second, int gentle_seconds) {
  return second < gentle_seconds ? Intensity::Gentle : Intensity::Loud;
}

inline Step finish_step(Occurrence occurrence, Intensity intensity, bool skip_next,
                        bool snooze_armed, Hm snooze_at) {
  return Step{occurrence, intensity, skip_next, snooze_armed, snooze_at};
}

inline Step step_alarm(Occurrence occurrence, Alarm alarm, Inputs in) {
  const bool on_alarm = hm_equal(in.now, alarm.at);
  const bool on_snooze = in.snooze_armed && hm_equal(in.now, in.snooze_at);
  if ((occurrence == Occurrence::Ringing || occurrence == Occurrence::Silenced ||
       occurrence == Occurrence::Skipped) &&
      !on_alarm && !on_snooze) {
    // A snooze whose minute has just passed is used up, answered or not.
    const bool snooze_spent =
        in.snooze_armed && minutes_until(in.snooze_at, in.now) >= 1 &&
        minutes_until(in.snooze_at, in.now) <= kSnoozeMinutes;
    return finish_step(Occurrence::Armed, Intensity::Silent, alarm.skip_next,
                       in.snooze_armed && !snooze_spent,
                       snooze_spent ? Hm{0, 0} : in.snooze_at);
  }
  if (!alarm.enabled &&
      (occurrence == Occurrence::Ringing || in.snooze_armed)) {
    return finish_step(Occurrence::Armed, Intensity::Silent, alarm.skip_next,
                       false, Hm{0, 0});
  }
  if (occurrence == Occurrence::Ringing && in.touched) {
    return finish_step(Occurrence::Silenced, Intensity::Silent, alarm.skip_next,
                       false, Hm{0, 0});
  }
  if (occurrence == Occurrence::Ringing && in.snooze) {
    return finish_step(Occurrence::Silenced, Intensity::Silent, alarm.skip_next,
                       true, step_minutes(in.now, kSnoozeMinutes));
  }
  if (occurrence == Occurrence::Ringing) {
    return finish_step(Occurrence::Ringing,
                       ring_intensity(in.second, in.gentle_seconds),
                       alarm.skip_next, in.snooze_armed, in.snooze_at);
  }
  if (occurrence == Occurrence::Silenced && (on_alarm || on_snooze)) {
    return finish_step(Occurrence::Silenced, Intensity::Silent, alarm.skip_next,
                       in.snooze_armed, in.snooze_at);
  }
  if (occurrence == Occurrence::Skipped && on_alarm) {
    return finish_step(Occurrence::Skipped, Intensity::Silent, alarm.skip_next,
                       in.snooze_armed, in.snooze_at);
  }
  if (occurrence == Occurrence::Armed && alarm.enabled && on_alarm &&
      alarm.skip_next) {
    return finish_step(Occurrence::Skipped, Intensity::Silent, false,
                       in.snooze_armed, in.snooze_at);
  }
  if (occurrence == Occurrence::Armed && alarm.enabled &&
      (on_alarm || on_snooze)) {
    return finish_step(Occurrence::Ringing,
                       ring_intensity(in.second, in.gentle_seconds),
                       alarm.skip_next, in.snooze_armed, in.snooze_at);
  }
  if (occurrence == Occurrence::Silenced) {
    return finish_step(Occurrence::Silenced, Intensity::Silent, alarm.skip_next,
                       in.snooze_armed, in.snooze_at);
  }
  if (occurrence == Occurrence::Skipped) {
    return finish_step(Occurrence::Skipped, Intensity::Silent, alarm.skip_next,
                       in.snooze_armed, in.snooze_at);
  }
  return finish_step(Occurrence::Armed, Intensity::Silent, alarm.skip_next,
                     in.snooze_armed, in.snooze_at);
}

inline TimeFormat next_format(TimeFormat fmt) {
  switch (fmt) {
    case TimeFormat::Hidden:
      return TimeFormat::Hours;
    case TimeFormat::Hours:
      return TimeFormat::HoursMinutes;
    case TimeFormat::HoursMinutes:
      return TimeFormat::HoursMinutesSeconds;
    case TimeFormat::HoursMinutesSeconds:
      return TimeFormat::Hidden;
  }
  return TimeFormat::HoursMinutesSeconds;
}

inline HourCycle next_hour_cycle(HourCycle cycle) {
  return cycle == HourCycle::H12 ? HourCycle::H24 : HourCycle::H12;
}

inline const char* hour_cycle_label(HourCycle cycle) {
  return cycle == HourCycle::H12 ? "12-hour" : "24-hour";
}

// What the clock face shows, named by example so the choice reads at a glance.
inline const char* format_label(TimeFormat fmt) {
  switch (fmt) {
    case TimeFormat::Hidden:
      return "Off";
    case TimeFormat::Hours:
      return "Hour only";
    case TimeFormat::HoursMinutes:
      return "12:34";
    case TimeFormat::HoursMinutesSeconds:
      return "12:34:56";
  }
  return "12:34:56";
}

inline int clamp_int(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

inline int adjust_volume(int volume, int delta) {
  return clamp_int(volume + delta, 0, 100);
}

inline int clamp_soft(int soft, int loud) {
  if (soft < 0) {
    return 0;
  }
  if (soft > loud) {
    return loud;
  }
  return soft;
}

inline int adjust_gentle(int seconds, int delta) {
  return clamp_int(seconds + delta, 0, 60);
}

inline int speaker_level(int volume_percent) {
  return clamp_int(volume_percent, 0, 100) * 255 / 100;
}

inline int clock_hour(int hour, HourCycle cycle) {
  if (cycle == HourCycle::H24) {
    return hour;
  }
  int h = hour % 12;
  if (h < 0) {
    h += 12;
  }
  return h == 0 ? 12 : h;
}

inline const char* day_period(int hour) { return hour < 12 ? "AM" : "PM"; }

inline void format_time(char* buf, size_t n, TimeFormat fmt, int hour,
                        int minute, int second,
                        HourCycle cycle = HourCycle::H24) {
  if (n == 0) {
    return;
  }
  if (fmt == TimeFormat::Hidden) {
    buf[0] = '\0';
    return;
  }
  if (cycle == HourCycle::H12) {
    const int h = clock_hour(hour, cycle);
    const char* period = day_period(hour);
    switch (fmt) {
      case TimeFormat::Hours:
        std::snprintf(buf, n, "%d %s", h, period);
        return;
      case TimeFormat::HoursMinutes:
        std::snprintf(buf, n, "%d:%02d %s", h, minute, period);
        return;
      case TimeFormat::HoursMinutesSeconds:
        std::snprintf(buf, n, "%d:%02d:%02d %s", h, minute, second, period);
        return;
      case TimeFormat::Hidden:
        break;
    }
    buf[0] = '\0';
    return;
  }
  switch (fmt) {
    case TimeFormat::Hidden:
      buf[0] = '\0';
      return;
    case TimeFormat::Hours:
      std::snprintf(buf, n, "%02d", hour);
      return;
    case TimeFormat::HoursMinutes:
      std::snprintf(buf, n, "%02d:%02d", hour, minute);
      return;
    case TimeFormat::HoursMinutesSeconds:
      std::snprintf(buf, n, "%02d:%02d:%02d", hour, minute, second);
      return;
  }
  buf[0] = '\0';
}

inline void format_hm(char* buf, size_t n, Hm t, HourCycle cycle) {
  format_time(buf, n, TimeFormat::HoursMinutes, t.hour, t.minute, 0, cycle);
}

// Compact, for tight spaces: "9PM", "9:30PM" or "21:00".
inline void format_hm_short(char* buf, size_t n, Hm t, HourCycle cycle) {
  if (cycle == HourCycle::H24) {
    std::snprintf(buf, n, "%02d:%02d", t.hour, t.minute);
    return;
  }
  const int h = clock_hour(t.hour, cycle);
  if (t.minute == 0) {
    std::snprintf(buf, n, "%d%s", h, day_period(t.hour));
  } else {
    std::snprintf(buf, n, "%d:%02d%s", h, t.minute, day_period(t.hour));
  }
}

inline void format_clock_face(char* digits, size_t nd, char* period, size_t np,
                              TimeFormat fmt, int hour, int minute, int second,
                              HourCycle cycle) {
  if (nd == 0 || np == 0) {
    return;
  }
  period[0] = '\0';
  if (fmt == TimeFormat::Hidden) {
    digits[0] = '\0';
    return;
  }
  if (cycle == HourCycle::H24) {
    format_time(digits, nd, fmt, hour, minute, second, HourCycle::H24);
    return;
  }
  const int h = clock_hour(hour, cycle);
  std::snprintf(period, np, "%s", day_period(hour));
  switch (fmt) {
    case TimeFormat::Hours:
      std::snprintf(digits, nd, "%d", h);
      return;
    case TimeFormat::HoursMinutes:
      std::snprintf(digits, nd, "%d:%02d", h, minute);
      return;
    case TimeFormat::HoursMinutesSeconds:
      std::snprintf(digits, nd, "%d:%02d:%02d", h, minute, second);
      return;
    case TimeFormat::Hidden:
      break;
  }
  digits[0] = '\0';
}

inline bool preview_playing(uint32_t elapsed_ms) { return elapsed_ms < 2000; }

inline int hm_minutes(Hm t) { return t.hour * 60 + t.minute; }

inline bool is_night(Hm now, Hm start, Hm end) {
  const int n = hm_minutes(now);
  const int s = hm_minutes(start);
  const int e = hm_minutes(end);
  if (s == e) {
    return false;
  }
  if (s < e) {
    return n >= s && n < e;
  }
  return n >= s || n < e;
}

inline Hm step_minutes(Hm t, int delta) {
  int m = hm_minutes(t) + delta;
  const int day = 24 * 60;
  m %= day;
  if (m < 0) {
    m += day;
  }
  return Hm{m / 60, m % 60};
}

// Minutes from a to b going forward, 0..1439.
inline int minutes_until(Hm from, Hm to) {
  int d = (hm_minutes(to) - hm_minutes(from)) % (24 * 60);
  return d < 0 ? d + 24 * 60 : d;
}

inline int snap_brightness(int value) {
  if (value < 10) {
    return 10;
  }
  if (value > 100) {
    return 100;
  }
  int snapped = ((value + 5) / 10) * 10;
  if (snapped > 100) {
    return 100;
  }
  return snapped;
}

inline int adjust_brightness(int value, int delta) {
  return clamp_int(snap_brightness(value) + delta, 10, 100);
}

inline int backlight_level(int percent) {
  percent = snap_brightness(percent);
  return 1 + (percent - 10) * 254 / 90;
}

inline Alarm apply_zone(Alarm alarm, Zone zone) {
  switch (zone) {
    case Zone::Clock:
      return alarm;
    case Zone::Hour:
      alarm.at.hour = (alarm.at.hour + 1) % 24;
      return alarm;
    case Zone::HourDown:
      alarm.at.hour = (alarm.at.hour + 23) % 24;
      return alarm;
    case Zone::Minute:
      alarm.at.minute = (alarm.at.minute + 1) % 60;
      return alarm;
    case Zone::MinuteDown:
      alarm.at.minute = (alarm.at.minute + 59) % 60;
      return alarm;
    case Zone::Toggle:
      alarm.enabled = !alarm.enabled;
      return alarm;
  }
  return alarm;
}

// ---------------------------------------------------------------------------
// Screen layout
//
// Every size comes from the screen's own width and height, so the same
// firmware lays itself out on the CoreS3 (320 x 240), the 4.1" AMOLED
// (616 x 284 in the simulator) and the 2.41" (600 x 450), or anything from
// about 160 x 120 up. `scale` is 100 on the CoreS3; everything else follows.

struct Box {
  int x;
  int y;
  int w;
  int h;
};

inline bool in_box(Box b, int px, int py) {
  return px >= b.x && py >= b.y && px < b.x + b.w && py < b.y + b.h;
}

inline bool in_bounds(int x, int y, int w, int h) {
  return x >= 0 && y >= 0 && x < w && y < h;
}

inline bool boxes_overlap(Box a, Box b) {
  return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h &&
         b.y < a.y + a.h;
}

struct Ui {
  int w;
  int h;
  int scale;   // percent of the CoreS3's layout
  int pad;     // margin at the screen's edges
  int gap;     // between rows and tiles
  int header;  // height of the header bar and of the clock's icon row
  int radius;  // corner radius of buttons and cards
};

inline int scaled(int px, int scale) { return (px * scale + 50) / 100; }

inline Ui ui_for(int w, int h) {
  Ui ui;
  ui.w = w < 1 ? 1 : w;
  ui.h = h < 1 ? 1 : h;
  const int sx = ui.w * 100 / 320;
  const int sy = ui.h * 100 / 240;
  ui.scale = clamp_int(sx < sy ? sx : sy, 40, 400);
  ui.pad = clamp_int(scaled(6, ui.scale), 3, 40);
  ui.gap = clamp_int(scaled(5, ui.scale), 2, 30);
  ui.header = clamp_int(scaled(40, ui.scale), 24, 160);
  if (ui.header > ui.h / 4) {
    ui.header = ui.h / 4;
  }
  ui.radius = clamp_int(scaled(8, ui.scale), 3, 32);
  return ui;
}

// Nominal text heights (px) for a role; main.cpp picks the largest font that fits.
inline int text_px(const Ui& ui) { return scaled(22, ui.scale); }
inline int title_px(const Ui& ui) { return scaled(26, ui.scale); }
inline int small_px(const Ui& ui) { return scaled(18, ui.scale); }

// ---- header bar (every settings page) ----

inline Box header_back_box(const Ui& ui) {
  const int bw = clamp_int(scaled(80, ui.scale), ui.header, ui.w / 3);
  return Box{ui.pad, ui.gap, bw, ui.header - 2 * ui.gap};
}

// The page title sits right of Back, left-aligned, with the rest of the bar.
inline Box header_title_box(const Ui& ui) {
  const Box back = header_back_box(ui);
  const int left = back.x + back.w + 2 * ui.gap;
  return Box{left, 0, ui.w - ui.pad - left, ui.header};
}

// ---- rows of a settings page ----

enum class RowKind : uint8_t {
  Stepper,  // label, then  [-] value [+]
  Cycle,    // label and value; a tap moves to the next choice
  Toggle,   // label and an on/off switch
  Link,     // label, value and >; a tap opens another page
  Pair,     // two buttons side by side
  Action,   // one wide button
};

enum class Part : uint8_t { None, Back, Whole, Minus, Plus, Left, Right };

struct Hit {
  Part part;
  int row;
};

constexpr int kMaxRows = 5;

inline int body_top(const Ui& ui) { return ui.header + ui.gap; }

inline int row_pitch(const Ui& ui, int rows) {
  if (rows < 1) {
    rows = 1;
  }
  const int room = ui.h - ui.pad - body_top(ui) + ui.gap;
  int pitch = room / rows;
  const int most = scaled(72, ui.scale);
  if (pitch > most) {
    pitch = most;
  }
  return pitch;
}

inline Box row_box(const Ui& ui, int row, int rows) {
  const int pitch = row_pitch(ui, rows);
  return Box{ui.pad, body_top(ui) + row * pitch, ui.w - 2 * ui.pad,
             pitch - ui.gap};
}

// Width of a stepper's - and + buttons: square, unless the row is tall.
inline int stepper_button_w(const Ui& ui, Box row) {
  const int most = scaled(48, ui.scale);
  return row.h < most ? row.h : most;
}

// Width of a stepper's value field, between its - and + buttons.
inline int stepper_value_w(const Ui& ui, Box row) {
  int vw = scaled(84, ui.scale);
  const int most = row.w - 2 * stepper_button_w(ui, row) - row.w / 3;
  if (vw > most) {
    vw = most;
  }
  return vw < 0 ? 0 : vw;
}

inline Box row_part_box(const Ui& ui, Box row, Part part) {
  const int bw = stepper_button_w(ui, row);
  switch (part) {
    case Part::Plus:
      return Box{row.x + row.w - bw, row.y, bw, row.h};
    case Part::Minus: {
      const int vw = stepper_value_w(ui, row);
      return Box{row.x + row.w - 2 * bw - vw, row.y, bw, row.h};
    }
    case Part::Left:
      return Box{row.x, row.y, (row.w - ui.gap) / 2, row.h};
    case Part::Right: {
      const int half = (row.w - ui.gap) / 2;
      return Box{row.x + row.w - half, row.y, half, row.h};
    }
    case Part::Whole:
    case Part::Back:
    case Part::None:
      break;
  }
  return row;
}

// Where a stepper's value is drawn: between - and +.
inline Box stepper_value_box(const Ui& ui, Box row) {
  const Box minus = row_part_box(ui, row, Part::Minus);
  const Box plus = row_part_box(ui, row, Part::Plus);
  return Box{minus.x + minus.w, row.y, plus.x - (minus.x + minus.w), row.h};
}

// Where a row's label is drawn, left of whatever controls the row has.
inline Box row_label_box(const Ui& ui, Box row, RowKind kind) {
  const int inset = ui.pad + ui.gap;
  int right = row.x + row.w - inset;
  if (kind == RowKind::Stepper) {
    right = row_part_box(ui, row, Part::Minus).x - ui.gap;
  } else if (kind == RowKind::Toggle) {
    right = row.x + row.w - inset - 2 * row.h;
  }
  return Box{row.x + inset, row.y, right - (row.x + inset), row.h};
}

inline Hit page_hit(const Ui& ui, const RowKind* kinds, int rows, int x, int y) {
  if (!in_bounds(x, y, ui.w, ui.h)) {
    return Hit{Part::None, -1};
  }
  if (in_box(header_back_box(ui), x, y)) {
    return Hit{Part::Back, -1};
  }
  for (int i = 0; i < rows && i < kMaxRows; ++i) {
    const Box row = row_box(ui, i, rows);
    if (!in_box(row, x, y)) {
      continue;
    }
    switch (kinds[i]) {
      case RowKind::Stepper:
        if (in_box(row_part_box(ui, row, Part::Minus), x, y)) {
          return Hit{Part::Minus, i};
        }
        if (in_box(row_part_box(ui, row, Part::Plus), x, y)) {
          return Hit{Part::Plus, i};
        }
        return Hit{Part::None, i};
      case RowKind::Pair:
        if (in_box(row_part_box(ui, row, Part::Left), x, y)) {
          return Hit{Part::Left, i};
        }
        if (in_box(row_part_box(ui, row, Part::Right), x, y)) {
          return Hit{Part::Right, i};
        }
        return Hit{Part::None, i};
      case RowKind::Cycle:
      case RowKind::Toggle:
      case RowKind::Link:
      case RowKind::Action:
        return Hit{Part::Whole, i};
    }
  }
  return Hit{Part::None, -1};
}

// ---- the settings menu: a grid of tiles ----

inline int tile_columns(const Ui& ui) { return ui.w * 100 >= ui.h * 180 ? 3 : 2; }

inline Box tile_box(const Ui& ui, int i, int n) {
  const int cols = tile_columns(ui);
  const int rows = (n + cols - 1) / cols;
  const int top = body_top(ui);
  const int tw = (ui.w - 2 * ui.pad - (cols - 1) * ui.gap) / cols;
  const int th = (ui.h - ui.pad - top - (rows - 1) * ui.gap) / (rows < 1 ? 1 : rows);
  const int col = i % cols;
  const int row = i / cols;
  return Box{ui.pad + col * (tw + ui.gap), top + row * (th + ui.gap), tw, th};
}

inline int tile_hit(const Ui& ui, int n, int x, int y) {
  for (int i = 0; i < n; ++i) {
    if (in_box(tile_box(ui, i, n), x, y)) {
      return i;
    }
  }
  return -1;
}

// ---- the clock face ----

enum class ClockHit { Face, Gear, Bell, Skip, Lamp, Sound };

inline int clock_icon_size(const Ui& ui) { return ui.header; }

inline Box clock_icon_box(const Ui& ui, ClockHit which) {
  const int s = clock_icon_size(ui);
  switch (which) {
    case ClockHit::Gear:
      return Box{ui.w - ui.pad - s, 0, s, s};
    case ClockHit::Bell:
      return Box{ui.w - ui.pad - 2 * s, 0, s, s};
    case ClockHit::Skip:
      return Box{ui.w - ui.pad - 3 * s, 0, s, s};
    case ClockHit::Lamp:
      return Box{ui.pad, 0, s, s};
    case ClockHit::Sound:
      return Box{ui.pad + s, 0, s, s};
    case ClockHit::Face:
      break;
  }
  return Box{0, 0, 0, 0};
}

inline ClockHit clock_hit(const Ui& ui, int x, int y) {
  const ClockHit icons[] = {ClockHit::Gear, ClockHit::Bell, ClockHit::Skip,
                            ClockHit::Lamp, ClockHit::Sound};
  for (ClockHit c : icons) {
    if (in_box(clock_icon_box(ui, c), x, y)) {
      return c;
    }
  }
  return ClockHit::Face;
}

// The line under the digits: next alarm, sleep sounds.
inline Box clock_status_box(const Ui& ui) {
  const int h = clamp_int(scaled(30, ui.scale), 16, ui.h / 5);
  return Box{ui.pad, ui.h - ui.pad - h, ui.w - 2 * ui.pad, h};
}

enum class RingHit { None, Snooze, Stop };

inline Box ring_box(const Ui& ui, bool stop) {
  const int bh = clamp_int(scaled(56, ui.scale), 24, ui.h / 4);
  const int bw = (ui.w - 2 * ui.pad - ui.gap) / 2;
  const int y = ui.h - ui.pad - bh;
  return Box{stop ? ui.w - ui.pad - bw : ui.pad, y, bw, bh};
}

inline RingHit ring_hit(const Ui& ui, int x, int y) {
  if (!in_bounds(x, y, ui.w, ui.h)) {
    return RingHit::None;
  }
  if (in_box(ring_box(ui, false), x, y)) {
    return RingHit::Snooze;
  }
  if (in_box(ring_box(ui, true), x, y)) {
    return RingHit::Stop;
  }
  return RingHit::None;
}

// The space the digits may use.
inline Box clock_digits_box(const Ui& ui, bool ringing) {
  const int top = ui.header + ui.gap;
  const int bottom = ringing ? ring_box(ui, false).y - ui.gap
                             : clock_status_box(ui).y - ui.gap;
  return Box{ui.pad, top, ui.w - 2 * ui.pad, bottom - top};
}

// ---------------------------------------------------------------------------
// Screens and what's on each

enum class Screen : uint8_t {
  Clock,
  Settings,
  Alarm,
  AlarmSound,
  SleepSounds,
  Lamp,
  DayScreen,
  NightScreen,
  NightHours,
};

constexpr int kMenuItems = 6;

inline Screen menu_item(int i) {
  static const Screen items[kMenuItems] = {
      Screen::Alarm,  Screen::AlarmSound, Screen::SleepSounds,
      Screen::Lamp,   Screen::DayScreen,  Screen::NightScreen};
  return items[clamp_int(i, 0, kMenuItems - 1)];
}

inline const char* screen_title(Screen s) {
  switch (s) {
    case Screen::Clock:
      return "Clock";
    case Screen::Settings:
      return "Settings";
    case Screen::Alarm:
      return "Alarm";
    case Screen::AlarmSound:
      return "Alarm sound";
    case Screen::SleepSounds:
      return "Sleep sounds";
    case Screen::Lamp:
      return "Lamp";
    case Screen::DayScreen:
      return "Day screen";
    case Screen::NightScreen:
      return "Night screen";
    case Screen::NightHours:
      return "Night hours";
  }
  return "";
}

// Short machine name, used by the serial "state" line and the simulator.
inline const char* screen_key(Screen s) {
  switch (s) {
    case Screen::Clock:
      return "clock";
    case Screen::Settings:
      return "settings";
    case Screen::Alarm:
      return "alarm";
    case Screen::AlarmSound:
      return "alarm_sound";
    case Screen::SleepSounds:
      return "sleep_sounds";
    case Screen::Lamp:
      return "lamp";
    case Screen::DayScreen:
      return "day_screen";
    case Screen::NightScreen:
      return "night_screen";
    case Screen::NightHours:
      return "night_hours";
  }
  return "";
}

// Where Back goes.
inline Screen parent_screen(Screen s) {
  switch (s) {
    case Screen::Clock:
    case Screen::Settings:
      return Screen::Clock;
    case Screen::NightHours:
      return Screen::NightScreen;
    default:
      return Screen::Settings;
  }
}

// The rows of a settings page. Returns how many.
inline int page_rows(Screen s, RowKind* kinds) {
  int n = 0;
  auto add = [&](RowKind k) { kinds[n++] = k; };
  switch (s) {
    case Screen::Alarm:
      add(RowKind::Toggle);   // Alarm on/off
      add(RowKind::Stepper);  // Hour
      add(RowKind::Stepper);  // Minute
      add(RowKind::Toggle);   // Skip next
      break;
    case Screen::AlarmSound:
      add(RowKind::Stepper);  // Loud volume
      add(RowKind::Stepper);  // Soft volume
      add(RowKind::Stepper);  // Soft for
      add(RowKind::Pair);     // Try soft | Try loud
      break;
    case Screen::SleepSounds:
      add(RowKind::Cycle);    // Sound
      add(RowKind::Stepper);  // Volume
      add(RowKind::Stepper);  // Timer
      add(RowKind::Action);   // Play / Stop
      break;
    case Screen::Lamp:
      add(RowKind::Toggle);   // Lamp on/off
      add(RowKind::Stepper);  // Brightness
      add(RowKind::Stepper);  // Sunrise
      break;
    case Screen::DayScreen:
      add(RowKind::Stepper);  // Brightness
      add(RowKind::Cycle);    // Clock face
      add(RowKind::Cycle);    // Hours
      break;
    case Screen::NightScreen:
      add(RowKind::Stepper);  // Brightness
      add(RowKind::Cycle);    // Clock face
      add(RowKind::Link);     // Night hours >
      break;
    case Screen::NightHours:
      add(RowKind::Cycle);    // Night mode
      add(RowKind::Stepper);  // Starts
      add(RowKind::Stepper);  // Ends
      break;
    case Screen::Clock:
    case Screen::Settings:
      break;
  }
  return n;
}

// Settings the screen is previewing: 1 = day look, 2 = night look, 0 = follow the clock.
inline int screen_preview(Screen s) {
  if (s == Screen::DayScreen) {
    return 1;
  }
  if (s == Screen::NightScreen || s == Screen::NightHours) {
    return 2;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// Sleep sounds: white, pink and brown noise, generated live

enum class NoiseKind : uint8_t { White, Pink, Brown };

inline const char* noise_label(NoiseKind k) {
  switch (k) {
    case NoiseKind::White:
      return "White noise";
    case NoiseKind::Pink:
      return "Pink noise";
    case NoiseKind::Brown:
      return "Brown noise";
  }
  return "Pink noise";
}

inline const char* noise_key(NoiseKind k) {
  switch (k) {
    case NoiseKind::White:
      return "white";
    case NoiseKind::Pink:
      return "pink";
    case NoiseKind::Brown:
      return "brown";
  }
  return "pink";
}

inline NoiseKind next_noise(NoiseKind k) {
  switch (k) {
    case NoiseKind::White:
      return NoiseKind::Pink;
    case NoiseKind::Pink:
      return NoiseKind::Brown;
    case NoiseKind::Brown:
      return NoiseKind::White;
  }
  return NoiseKind::Pink;
}

struct NoiseGen {
  uint32_t seed;
  float b0, b1, b2, b3, b4, b5, b6;
  float brown;
};

inline NoiseGen noise_gen(uint32_t seed) {
  NoiseGen g = {};
  g.seed = seed == 0 ? 0x9E3779B9u : seed;
  return g;
}

// xorshift32, mapped to [-1, 1).
inline float noise_white(NoiseGen& g) {
  uint32_t x = g.seed;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  g.seed = x;
  return static_cast<float>(static_cast<int32_t>(x)) * (1.0f / 2147483648.0f);
}

// One sample, full scale = 1. Each kind is levelled to about the same loudness.
inline float noise_value(NoiseGen& g, NoiseKind kind) {
  const float white = noise_white(g);
  switch (kind) {
    case NoiseKind::White:
      return white * 0.28f;
    case NoiseKind::Pink: {
      // Paul Kellet's refined pink filter.
      g.b0 = 0.99886f * g.b0 + white * 0.0555179f;
      g.b1 = 0.99332f * g.b1 + white * 0.0750759f;
      g.b2 = 0.96900f * g.b2 + white * 0.1538520f;
      g.b3 = 0.86650f * g.b3 + white * 0.3104856f;
      g.b4 = 0.55000f * g.b4 + white * 0.5329522f;
      g.b5 = -0.7616f * g.b5 - white * 0.0168980f;
      const float pink = g.b0 + g.b1 + g.b2 + g.b3 + g.b4 + g.b5 + g.b6 +
                         white * 0.5362f;
      g.b6 = white * 0.115926f;
      return pink * 0.09f;
    }
    case NoiseKind::Brown:
      // A leaky integrator: the leak keeps it from wandering off-centre.
      g.brown = (g.brown + 0.02f * white) / 1.02f;
      return g.brown * 2.8f;
  }
  return 0.f;
}

inline int16_t noise_sample(NoiseGen& g, NoiseKind kind, int gain_permille) {
  float v = noise_value(g, kind) * static_cast<float>(gain_permille) * 32767.f /
            1000.f;
  if (v > 32767.f) {
    v = 32767.f;
  }
  if (v < -32767.f) {
    v = -32767.f;
  }
  return static_cast<int16_t>(v);
}

// Fill a buffer, ramping the gain from `from` to `to` (permille) so a level
// change never clicks.
inline void fill_noise(NoiseGen& g, NoiseKind kind, int16_t* out, size_t n,
                       int from, int to) {
  for (size_t i = 0; i < n; ++i) {
    const int gain =
        n > 1 ? from + static_cast<int>((to - from) * static_cast<long>(i) /
                                        static_cast<long>(n - 1))
              : to;
    out[i] = noise_sample(g, kind, gain);
  }
}

constexpr uint32_t kNoiseFadeInMs = 3000;
constexpr uint32_t kNoiseFadeOutMs = 60000;

// How loud the sleep sound is, permille, `elapsed_ms` after it started.
// It fades in, and with a timer it fades out over the last minute.
inline int noise_gain_permille(uint32_t elapsed_ms, int timer_minutes) {
  int gain = elapsed_ms >= kNoiseFadeInMs
                 ? 1000
                 : static_cast<int>(elapsed_ms * 1000u / kNoiseFadeInMs);
  if (timer_minutes > 0) {
    const uint32_t total = static_cast<uint32_t>(timer_minutes) * 60000u;
    if (elapsed_ms >= total) {
      return 0;
    }
    const uint32_t left = total - elapsed_ms;
    if (left < kNoiseFadeOutMs) {
      const int out = static_cast<int>(left * 1000u / kNoiseFadeOutMs);
      if (out < gain) {
        gain = out;
      }
    }
  }
  return gain;
}

inline bool noise_timer_done(uint32_t elapsed_ms, int timer_minutes) {
  return timer_minutes > 0 &&
         elapsed_ms >= static_cast<uint32_t>(timer_minutes) * 60000u;
}

// Whole minutes left on the timer, rounded up; 0 with no timer.
inline int noise_minutes_left(uint32_t elapsed_ms, int timer_minutes) {
  if (timer_minutes <= 0) {
    return 0;
  }
  const uint32_t total = static_cast<uint32_t>(timer_minutes) * 60000u;
  if (elapsed_ms >= total) {
    return 0;
  }
  return static_cast<int>((total - elapsed_ms + 59999u) / 60000u);
}

// Steps a value through a fixed list, stopping at the ends.
inline int step_through(const int* steps, int n, int value, int dir) {
  int at = 0;
  for (int i = 0; i < n; ++i) {
    if (steps[i] <= value) {
      at = i;
    }
  }
  at = clamp_int(at + (dir > 0 ? 1 : (dir < 0 ? -1 : 0)), 0, n - 1);
  return steps[at];
}

inline bool in_steps(const int* steps, int n, int value) {
  for (int i = 0; i < n; ++i) {
    if (steps[i] == value) {
      return true;
    }
  }
  return false;
}

constexpr int kTimerSteps[] = {0, 15, 30, 45, 60, 90, 120};
constexpr int kTimerStepCount = 7;
constexpr int kSunriseSteps[] = {0, 10, 15, 20, 30, 45, 60};
constexpr int kSunriseStepCount = 7;

inline int step_timer(int minutes, int dir) {
  return step_through(kTimerSteps, kTimerStepCount, minutes, dir);
}

inline int step_sunrise(int minutes, int dir) {
  return step_through(kSunriseSteps, kSunriseStepCount, minutes, dir);
}

// 5..100 in steps of 5, for the lamp and sleep-sound levels.
inline int adjust_level(int value, int delta) {
  const int snapped = (clamp_int(value, 5, 100) + 2) / 5 * 5;
  return clamp_int(snapped + delta, 5, 100);
}

// ---------------------------------------------------------------------------
// Lamp and sunrise

// How far through the sunrise we are, permille: it ramps from 0 to 1000 over
// `ramp_minutes` before the alarm and holds 1000 through the alarm's minute.
inline int sunrise_permille(Hm now, int second, Alarm alarm, int ramp_minutes) {
  if (!alarm.enabled || alarm.skip_next || ramp_minutes <= 0) {
    return 0;
  }
  if (hm_equal(now, alarm.at)) {
    return 1000;
  }
  const int ramp_s = ramp_minutes * 60;
  const int until_s = minutes_until(now, alarm.at) * 60 - clamp_int(second, 0, 59);
  if (until_s <= 0 || until_s > ramp_s) {
    return 0;
  }
  return (ramp_s - until_s) * 1000 / ramp_s;
}

constexpr int kWakeLightMinutes = 30;

// After a sunrise alarm rings the lamp stays up, until this many minutes past
// the alarm or until someone turns it off.
inline bool wake_light_expired(Hm now, Hm alarm_at) {
  return minutes_until(alarm_at, now) >= kWakeLightMinutes;
}

// The lamp's level, permille of full, from everything that can light it.
inline int lamp_permille(bool manual_on, int bright_percent, int sunrise,
                         bool wake_light) {
  int level = manual_on ? clamp_int(bright_percent, 0, 100) * 10 : 0;
  if (sunrise > level) {
    level = sunrise;
  }
  if (wake_light) {
    level = 1000;
  }
  return clamp_int(level, 0, 1000);
}

// Drive level, 0..255, for a perceptual level. Squared, so the dim end has
// fine steps, and never 0 unless asked for 0.
inline int lamp_duty(int permille) {
  permille = clamp_int(permille, 0, 1000);
  if (permille == 0) {
    return 0;
  }
  const long d = (static_cast<long>(permille) * permille * 255 + 500000) / 1000000;
  return d < 1 ? 1 : static_cast<int>(d);
}

// What the physical button (and the lamp icon) does.
enum class ButtonAction { Snooze, LampOff, LampOn };

inline ButtonAction button_action(bool ringing, bool lamp_lit) {
  if (ringing) {
    return ButtonAction::Snooze;
  }
  return lamp_lit ? ButtonAction::LampOff : ButtonAction::LampOn;
}

// ---------------------------------------------------------------------------
// Serial commands
//
// The same text commands work over USB serial on the clock and from the
// simulator, so tests and the 3D viewer drive the real firmware code path.
//   lamp on|off|toggle|<1-100>     sound on|off|toggle|white|pink|brown
//   time HH:MM[:SS]                alarm HH:MM | on | off
//   go day|night|alarm|sunrise     button     state     help

enum class CmdKind : uint8_t {
  None,
  Unknown,
  Help,
  State,
  LampOn,
  LampOff,
  LampToggle,
  LampLevel,
  SoundOn,
  SoundOff,
  SoundToggle,
  SoundKind,
  Time,
  AlarmAt,
  AlarmOn,
  AlarmOff,
  GoDay,
  GoNight,
  GoAlarm,
  GoSunrise,
  Button,
};

struct Command {
  CmdKind kind;
  int a;
  int b;
  int c;
};

inline bool cmd_word(const char*& p, const char* word) {
  while (*p == ' ') {
    ++p;
  }
  size_t n = std::strlen(word);
  for (size_t i = 0; i < n; ++i) {
    char c = p[i];
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c - 'A' + 'a');
    }
    if (c != word[i]) {
      return false;
    }
  }
  const char end = p[n];
  if (end != '\0' && end != ' ') {
    return false;
  }
  p += n;
  return true;
}

inline bool cmd_end(const char* p) {
  while (*p == ' ') {
    ++p;
  }
  return *p == '\0';
}

// Reads 1-3 digits, 0..999. Returns -1 if there are none or too many.
inline int cmd_number(const char*& p) {
  while (*p == ' ') {
    ++p;
  }
  int v = 0;
  int digits = 0;
  while (*p >= '0' && *p <= '9') {
    v = v * 10 + (*p - '0');
    ++p;
    if (++digits > 3) {
      return -1;
    }
  }
  return digits ? v : -1;
}

// HH:MM or HH:MM:SS. Returns false unless it's a real time of day.
inline bool cmd_clock(const char*& p, int* h, int* m, int* s, bool seconds_ok) {
  *h = cmd_number(p);
  if (*h < 0 || *h > 23 || *p != ':') {
    return false;
  }
  ++p;
  if (!(p[0] >= '0' && p[0] <= '9' && p[1] >= '0' && p[1] <= '9')) {
    return false;
  }
  *m = (p[0] - '0') * 10 + (p[1] - '0');
  p += 2;
  *s = 0;
  if (seconds_ok && *p == ':') {
    ++p;
    if (!(p[0] >= '0' && p[0] <= '9' && p[1] >= '0' && p[1] <= '9')) {
      return false;
    }
    *s = (p[0] - '0') * 10 + (p[1] - '0');
    p += 2;
  }
  return *m <= 59 && *s <= 59 && cmd_end(p);
}

inline Command parse_command(const char* line) {
  Command c = {CmdKind::Unknown, 0, 0, 0};
  const char* p = line;
  if (cmd_end(p)) {
    c.kind = CmdKind::None;
    return c;
  }
  if (cmd_word(p, "help") && cmd_end(p)) {
    c.kind = CmdKind::Help;
  } else if (p = line, cmd_word(p, "state") && cmd_end(p)) {
    c.kind = CmdKind::State;
  } else if (p = line, cmd_word(p, "button") && cmd_end(p)) {
    c.kind = CmdKind::Button;
  } else if (p = line, cmd_word(p, "lamp")) {
    const char* q = p;
    if (cmd_word(q, "on") && cmd_end(q)) {
      c.kind = CmdKind::LampOn;
    } else if (q = p, cmd_word(q, "off") && cmd_end(q)) {
      c.kind = CmdKind::LampOff;
    } else if (q = p, cmd_word(q, "toggle") && cmd_end(q)) {
      c.kind = CmdKind::LampToggle;
    } else {
      q = p;
      const int v = cmd_number(q);
      if (v >= 0 && v <= 100 && cmd_end(q)) {
        c.kind = v == 0 ? CmdKind::LampOff : CmdKind::LampLevel;
        c.a = v;
      }
    }
  } else if (p = line, cmd_word(p, "sound")) {
    const char* q = p;
    if (cmd_word(q, "on") && cmd_end(q)) {
      c.kind = CmdKind::SoundOn;
    } else if (q = p, cmd_word(q, "off") && cmd_end(q)) {
      c.kind = CmdKind::SoundOff;
    } else if (q = p, cmd_word(q, "toggle") && cmd_end(q)) {
      c.kind = CmdKind::SoundToggle;
    } else if (q = p, cmd_word(q, "white") && cmd_end(q)) {
      c.kind = CmdKind::SoundKind;
      c.a = static_cast<int>(NoiseKind::White);
    } else if (q = p, cmd_word(q, "pink") && cmd_end(q)) {
      c.kind = CmdKind::SoundKind;
      c.a = static_cast<int>(NoiseKind::Pink);
    } else if (q = p, cmd_word(q, "brown") && cmd_end(q)) {
      c.kind = CmdKind::SoundKind;
      c.a = static_cast<int>(NoiseKind::Brown);
    }
  } else if (p = line, cmd_word(p, "time")) {
    int h, m, s;
    if (cmd_clock(p, &h, &m, &s, true)) {
      c.kind = CmdKind::Time;
      c.a = h;
      c.b = m;
      c.c = s;
    }
  } else if (p = line, cmd_word(p, "alarm")) {
    const char* q = p;
    int h, m, s;
    if (cmd_word(q, "on") && cmd_end(q)) {
      c.kind = CmdKind::AlarmOn;
    } else if (q = p, cmd_word(q, "off") && cmd_end(q)) {
      c.kind = CmdKind::AlarmOff;
    } else if (q = p, cmd_clock(q, &h, &m, &s, false)) {
      c.kind = CmdKind::AlarmAt;
      c.a = h;
      c.b = m;
    }
  } else if (p = line, cmd_word(p, "go")) {
    const char* q = p;
    if (cmd_word(q, "day") && cmd_end(q)) {
      c.kind = CmdKind::GoDay;
    } else if (q = p, cmd_word(q, "night") && cmd_end(q)) {
      c.kind = CmdKind::GoNight;
    } else if (q = p, cmd_word(q, "alarm") && cmd_end(q)) {
      c.kind = CmdKind::GoAlarm;
    } else if (q = p, cmd_word(q, "sunrise") && cmd_end(q)) {
      c.kind = CmdKind::GoSunrise;
    }
  }
  return c;
}

// Collects serial bytes into lines. Over-long lines are dropped whole.
struct LineReader {
  char buf[64];
  size_t len;
  bool overflow;
};

// Returns true when `c` finishes a line, which is then in r.buf.
inline bool line_push(LineReader& r, char c) {
  if (c == '\r' || c == '\n') {
    const bool ok = !r.overflow && r.len > 0;
    r.buf[ok ? r.len : 0] = '\0';
    r.len = 0;
    r.overflow = false;
    return ok;
  }
  if (r.len + 1 >= sizeof(r.buf)) {
    r.overflow = true;
    return false;
  }
  r.buf[r.len++] = c;
  return false;
}

// ---------------------------------------------------------------------------
// Compile stamp, used to set a blank clock chip

inline int month_number(const char* mmm) {
  static const char* names[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  };
  for (int i = 0; i < 12; ++i) {
    const char* n = names[i];
    if (mmm[0] == n[0] && mmm[1] == n[1] && mmm[2] == n[2]) {
      return i + 1;
    }
  }
  return 0;
}

inline int read_two_digits(const char* p) {
  return (p[0] - '0') * 10 + (p[1] - '0');
}

inline int read_four_digits(const char* p) {
  return (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 +
         (p[3] - '0');
}

inline YmdHms parse_compile_stamp(const char* date, const char* time) {
  YmdHms out;
  out.month = month_number(date);
  const char* p = date + 4;
  if (*p == ' ') {
    ++p;
    out.date = *p - '0';
    ++p;
  } else {
    out.date = read_two_digits(p);
    p += 2;
  }
  ++p;
  out.year = read_four_digits(p);
  out.hour = read_two_digits(time);
  out.minute = read_two_digits(time + 3);
  out.second = read_two_digits(time + 6);
  return out;
}

inline bool rtc_needs_set(int year) { return year < 2026; }

// ---------------------------------------------------------------------------
// Saved settings: one blob in NVS, checked before it's trusted

constexpr uint32_t kSettingsMagic = 0x41434B31u;
constexpr uint16_t kSettingsVersion = 2;

// Version 1, as written by earlier firmware; read once and upgraded.
struct SettingsV1 {
  uint32_t magic;
  uint16_t version;
  uint8_t alarm_hour;
  uint8_t alarm_minute;
  uint8_t alarm_enabled;
  uint8_t skip_next;
  uint8_t volume;
  uint8_t soft_volume;
  uint8_t gentle_seconds;
  uint8_t day_format;
  uint8_t night_format;
  uint8_t color_mode;
  uint8_t night_start_hour;
  uint8_t night_start_minute;
  uint8_t night_end_hour;
  uint8_t night_end_minute;
  uint8_t day_bright;
  uint8_t night_bright;
  uint8_t hour12;
};

struct Settings {
  uint32_t magic;
  uint16_t version;
  uint8_t alarm_hour;
  uint8_t alarm_minute;
  uint8_t alarm_enabled;
  uint8_t skip_next;
  uint8_t volume;
  uint8_t soft_volume;
  uint8_t gentle_seconds;
  uint8_t day_format;
  uint8_t night_format;
  uint8_t color_mode;
  uint8_t night_start_hour;
  uint8_t night_start_minute;
  uint8_t night_end_hour;
  uint8_t night_end_minute;
  uint8_t day_bright;
  uint8_t night_bright;
  uint8_t hour12;
  // Added in version 2
  uint8_t noise_kind;
  uint8_t noise_volume;
  uint8_t noise_timer;
  uint8_t lamp_on;
  uint8_t lamp_bright;
  uint8_t sunrise_minutes;
  uint8_t reserved[3];
};

static_assert(sizeof(SettingsV1) == 24, "version 1 blob is 24 bytes");
static_assert(sizeof(Settings) == 32, "settings blob stays one small NVS record");

inline Settings default_settings() {
  Settings s;
  std::memset(&s, 0, sizeof(s));
  s.magic = kSettingsMagic;
  s.version = kSettingsVersion;
  s.alarm_hour = 7;
  s.alarm_minute = 0;
  s.alarm_enabled = 0;
  s.skip_next = 0;
  s.volume = 80;
  s.soft_volume = 40;
  s.gentle_seconds = kGentleSeconds;
  s.day_format = static_cast<uint8_t>(TimeFormat::HoursMinutesSeconds);
  s.night_format = static_cast<uint8_t>(TimeFormat::HoursMinutesSeconds);
  s.color_mode = static_cast<uint8_t>(ColorMode::Auto);
  s.night_start_hour = 21;
  s.night_start_minute = 0;
  s.night_end_hour = 7;
  s.night_end_minute = 0;
  s.day_bright = 70;
  s.night_bright = 20;
  s.hour12 = 0;
  s.noise_kind = static_cast<uint8_t>(NoiseKind::Pink);
  s.noise_volume = 30;
  s.noise_timer = 30;
  s.lamp_on = 0;
  s.lamp_bright = 40;
  s.sunrise_minutes = 0;
  return s;
}

inline bool settings_valid(const Settings& s) {
  if (s.magic != kSettingsMagic || s.version != kSettingsVersion) {
    return false;
  }
  if (s.alarm_hour > 23 || s.alarm_minute > 59) {
    return false;
  }
  if (s.alarm_enabled > 1 || s.skip_next > 1) {
    return false;
  }
  if (s.volume > 100 || s.soft_volume > s.volume || s.gentle_seconds > 60) {
    return false;
  }
  if (s.day_format > 3 || s.night_format > 3 || s.color_mode > 2) {
    return false;
  }
  if (s.night_start_hour > 23 || s.night_start_minute > 59 ||
      s.night_end_hour > 23 || s.night_end_minute > 59) {
    return false;
  }
  if (s.day_bright < 5 || s.day_bright > 100 || s.night_bright < 5 ||
      s.night_bright > 100) {
    return false;
  }
  if (s.hour12 > 1) {
    return false;
  }
  if (s.noise_kind > 2 || s.noise_volume < 5 || s.noise_volume > 100 ||
      !in_steps(kTimerSteps, kTimerStepCount, s.noise_timer)) {
    return false;
  }
  if (s.lamp_on > 1 || s.lamp_bright < 5 || s.lamp_bright > 100 ||
      !in_steps(kSunriseSteps, kSunriseStepCount, s.sunrise_minutes)) {
    return false;
  }
  return true;
}

// Reads a stored blob of `n` bytes: the current version, or version 1
// upgraded with defaults for what it didn't have.
inline bool load_settings(const void* data, size_t n, Settings* out) {
  if (n == sizeof(Settings)) {
    Settings s;
    std::memcpy(&s, data, sizeof(s));
    if (!settings_valid(s)) {
      return false;
    }
    *out = s;
    return true;
  }
  if (n == sizeof(SettingsV1)) {
    SettingsV1 old;
    std::memcpy(&old, data, sizeof(old));
    if (old.magic != kSettingsMagic || old.version != 1) {
      return false;
    }
    Settings s = default_settings();
    // The first 23 bytes line up field for field.
    std::memcpy(&s.alarm_hour, &old.alarm_hour, 17);
    if (!settings_valid(s)) {
      return false;
    }
    *out = s;
    return true;
  }
  return false;
}

inline Alarm alarm_of(const Settings& s) {
  return Alarm{{s.alarm_hour, s.alarm_minute}, s.alarm_enabled != 0,
               s.skip_next != 0};
}

#endif
