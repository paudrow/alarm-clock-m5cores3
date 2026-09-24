#ifndef ALARM_FACE_HPP
#define ALARM_FACE_HPP

#include <cstddef>
#include <cstdio>
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

struct Box {
  int x;
  int y;
  int w;
  int h;
};

inline bool in_box(Box b, int px, int py) {
  return px >= b.x && py >= b.y && px < b.x + b.w && py < b.y + b.h;
}

inline Box inset_button(int row, int rows, int screen_w, int screen_h, int x,
                        int bw) {
  int y0 = row * screen_h / rows;
  int y1 = (row + 1) * screen_h / rows;
  int row_h = y1 - y0;
  int margin = 10;
  int bh = row_h - 2 * margin;
  if (bh < 20) {
    bh = 20;
  }
  int by = y0 + (row_h - bh) / 2;
  if (x < 0) {
    x = 0;
  }
  if (x + bw > screen_w) {
    bw = screen_w - x;
  }
  return Box{x, by, bw, bh};
}

inline Box centered_button(int row, int rows, int screen_w, int screen_h,
                           int bw) {
  if (bw > screen_w - 24) {
    bw = screen_w - 24;
  }
  return inset_button(row, rows, screen_w, screen_h, (screen_w - bw) / 2, bw);
}

inline Box back_button(int rows, int screen_w, int screen_h) {
  return inset_button(0, rows, screen_w, screen_h, 8, 88);
}

inline Box column_button(int row, int rows, int col, int cols, int screen_w,
                         int screen_h) {
  int col_w = screen_w / cols;
  int gap = 8;
  int bw = col_w - 2 * gap;
  return inset_button(row, rows, screen_w, screen_h, col * col_w + gap, bw);
}

inline Box side_button(int row, int rows, int screen_w, int screen_h,
                       bool right) {
  int bw = 64;
  int x = right ? screen_w - 8 - bw : 8;
  return inset_button(row, rows, screen_w, screen_h, x, bw);
}

inline Box skip_box(int screen_w) { return Box{screen_w - 132, 2, 40, 40}; }
inline Box bell_box(int screen_w) { return Box{screen_w - 88, 2, 40, 40}; }
inline Box gear_box(int screen_w) { return Box{screen_w - 44, 2, 40, 40}; }

enum class ClockHit {
  Face,
  Gear,
  Bell,
  Skip,
};

enum class MenuHit { None, Back, Alarm, Sound, Day, Night };

enum class ColorMode { Auto, Day, Night };

enum class DayPageHit { None, Back, BrightDown, BrightUp, Face, Hour };

enum class NightPageHit { None, Back, BrightDown, BrightUp, Face, Hour, When };

enum class WhenPageHit {
  None,
  Back,
  Mode,
  StartDown,
  StartUp,
  EndDown,
  EndUp,
};

enum class AlarmPageHit {
  None,
  Back,
  Toggle,
  HourDown,
  HourUp,
  MinuteDown,
  MinuteUp,
};

enum class SoundPageHit {
  None,
  Back,
  VolumeDown,
  VolumeUp,
  SoftDown,
  SoftUp,
  GentleDown,
  GentleUp,
  PreviewSoft,
  PreviewLoud,
};

enum class RingHit { None, Snooze, Stop };

inline const char* color_mode_label(ColorMode mode) {
  switch (mode) {
    case ColorMode::Day:
      return "Always day";
    case ColorMode::Night:
      return "Always night";
    case ColorMode::Auto:
      return "Follow clock";
  }
  return "Follow clock";
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
    return finish_step(Occurrence::Armed, Intensity::Silent, alarm.skip_next,
                       in.snooze_armed, in.snooze_at);
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
  return cycle == HourCycle::H12 ? "AM/PM" : "24 hour";
}

inline const char* format_label(TimeFormat fmt) {
  switch (fmt) {
    case TimeFormat::Hidden:
      return "None";
    case TimeFormat::Hours:
      return "H";
    case TimeFormat::HoursMinutes:
      return "H:M";
    case TimeFormat::HoursMinutesSeconds:
      return "H:M:S";
  }
  return "H:M:S";
}

inline int adjust_volume(int volume, int delta) {
  int v = volume + delta;
  if (v < 0) {
    return 0;
  }
  if (v > 100) {
    return 100;
  }
  return v;
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
  int v = seconds + delta;
  if (v < 0) {
    return 0;
  }
  if (v > 60) {
    return 60;
  }
  return v;
}

inline int speaker_level(int volume_percent) {
  int v = volume_percent;
  if (v < 0) {
    v = 0;
  }
  if (v > 100) {
    v = 100;
  }
  return v * 255 / 100;
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
  int v = snap_brightness(value) + delta;
  if (v < 10) {
    return 10;
  }
  if (v > 100) {
    return 100;
  }
  return v;
}

inline int backlight_level(int percent) {
  percent = snap_brightness(percent);
  return 1 + (percent - 10) * 254 / 90;
}

inline ClockHit clock_hit(int x, int y, int w, int h) {
  (void)h;
  if (in_box(gear_box(w), x, y)) {
    return ClockHit::Gear;
  }
  if (in_box(bell_box(w), x, y)) {
    return ClockHit::Bell;
  }
  if (in_box(skip_box(w), x, y)) {
    return ClockHit::Skip;
  }
  return ClockHit::Face;
}

inline bool in_bounds(int x, int y, int w, int h) {
  return x >= 0 && y >= 0 && x < w && y < h;
}

inline MenuHit menu_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return MenuHit::None;
  }
  if (in_box(back_button(5, w, h), x, y)) {
    return MenuHit::Back;
  }
  if (in_box(centered_button(1, 5, w, h, 180), x, y)) {
    return MenuHit::Alarm;
  }
  if (in_box(centered_button(2, 5, w, h, 180), x, y)) {
    return MenuHit::Sound;
  }
  if (in_box(centered_button(3, 5, w, h, 180), x, y)) {
    return MenuHit::Day;
  }
  if (in_box(centered_button(4, 5, w, h, 180), x, y)) {
    return MenuHit::Night;
  }
  return MenuHit::None;
}

inline AlarmPageHit alarm_page_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return AlarmPageHit::None;
  }
  if (in_box(back_button(3, w, h), x, y)) {
    return AlarmPageHit::Back;
  }
  if (in_box(centered_button(1, 3, w, h, 220), x, y)) {
    return AlarmPageHit::Toggle;
  }
  const AlarmPageHit steppers[] = {
      AlarmPageHit::HourDown, AlarmPageHit::HourUp, AlarmPageHit::MinuteDown,
      AlarmPageHit::MinuteUp};
  for (int col = 0; col < 4; ++col) {
    if (in_box(column_button(2, 3, col, 4, w, h), x, y)) {
      return steppers[col];
    }
  }
  return AlarmPageHit::None;
}

inline SoundPageHit sound_page_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return SoundPageHit::None;
  }
  if (in_box(back_button(5, w, h), x, y)) {
    return SoundPageHit::Back;
  }
  if (in_box(side_button(1, 5, w, h, false), x, y)) {
    return SoundPageHit::VolumeDown;
  }
  if (in_box(side_button(1, 5, w, h, true), x, y)) {
    return SoundPageHit::VolumeUp;
  }
  if (in_box(side_button(2, 5, w, h, false), x, y)) {
    return SoundPageHit::SoftDown;
  }
  if (in_box(side_button(2, 5, w, h, true), x, y)) {
    return SoundPageHit::SoftUp;
  }
  if (in_box(side_button(3, 5, w, h, false), x, y)) {
    return SoundPageHit::GentleDown;
  }
  if (in_box(side_button(3, 5, w, h, true), x, y)) {
    return SoundPageHit::GentleUp;
  }
  if (in_box(column_button(4, 5, 0, 2, w, h), x, y)) {
    return SoundPageHit::PreviewSoft;
  }
  if (in_box(column_button(4, 5, 1, 2, w, h), x, y)) {
    return SoundPageHit::PreviewLoud;
  }
  return SoundPageHit::None;
}

inline DayPageHit day_page_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return DayPageHit::None;
  }
  if (in_box(back_button(4, w, h), x, y)) {
    return DayPageHit::Back;
  }
  if (in_box(side_button(1, 4, w, h, false), x, y)) {
    return DayPageHit::BrightDown;
  }
  if (in_box(side_button(1, 4, w, h, true), x, y)) {
    return DayPageHit::BrightUp;
  }
  if (in_box(centered_button(2, 4, w, h, 220), x, y)) {
    return DayPageHit::Face;
  }
  if (in_box(centered_button(3, 4, w, h, 220), x, y)) {
    return DayPageHit::Hour;
  }
  return DayPageHit::None;
}

inline NightPageHit night_page_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return NightPageHit::None;
  }
  if (in_box(back_button(5, w, h), x, y)) {
    return NightPageHit::Back;
  }
  if (in_box(side_button(1, 5, w, h, false), x, y)) {
    return NightPageHit::BrightDown;
  }
  if (in_box(side_button(1, 5, w, h, true), x, y)) {
    return NightPageHit::BrightUp;
  }
  if (in_box(centered_button(2, 5, w, h, 220), x, y)) {
    return NightPageHit::Face;
  }
  if (in_box(centered_button(3, 5, w, h, 220), x, y)) {
    return NightPageHit::Hour;
  }
  if (in_box(centered_button(4, 5, w, h, 220), x, y)) {
    return NightPageHit::When;
  }
  return NightPageHit::None;
}

inline WhenPageHit when_page_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return WhenPageHit::None;
  }
  if (in_box(back_button(4, w, h), x, y)) {
    return WhenPageHit::Back;
  }
  if (in_box(centered_button(1, 4, w, h, 220), x, y)) {
    return WhenPageHit::Mode;
  }
  if (in_box(side_button(2, 4, w, h, false), x, y)) {
    return WhenPageHit::StartDown;
  }
  if (in_box(side_button(2, 4, w, h, true), x, y)) {
    return WhenPageHit::StartUp;
  }
  if (in_box(side_button(3, 4, w, h, false), x, y)) {
    return WhenPageHit::EndDown;
  }
  if (in_box(side_button(3, 4, w, h, true), x, y)) {
    return WhenPageHit::EndUp;
  }
  return WhenPageHit::None;
}

inline Box ring_action_box(int screen_w, int screen_h, bool stop) {
  const int bw = 136;
  const int bh = 44;
  const int y = screen_h - bh - 8;
  const int x = stop ? screen_w - 12 - bw : 12;
  return Box{x, y, bw, bh};
}

inline RingHit ring_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return RingHit::None;
  }
  if (in_box(ring_action_box(w, h, false), x, y)) {
    return RingHit::Snooze;
  }
  if (in_box(ring_action_box(w, h, true), x, y)) {
    return RingHit::Stop;
  }
  return RingHit::None;
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

constexpr uint32_t kSettingsMagic = 0x41434B31u;
constexpr uint16_t kSettingsVersion = 1;

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
};

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
  return true;
}

static_assert(sizeof(Settings) == 24, "settings blob stays one NVS record");

#endif
