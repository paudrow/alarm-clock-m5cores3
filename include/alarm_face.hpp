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

enum class MenuHit { None, Back, Alarm, Sound, ClockFace, Display };

enum class ColorMode { Auto, Day, Night };

enum class DisplayPageHit {
  None,
  Back,
  Mode,
  DayDown,
  DayUp,
  NightDown,
  NightUp,
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

enum class ClockPageHit { None, Back, DayFormat, NightFormat };

enum class RingHit { None, Snooze, Stop };

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

inline void format_time(char* buf, size_t n, TimeFormat fmt, int hour,
                        int minute, int second) {
  if (n == 0) {
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

inline int adjust_brightness(int value, int delta) {
  int v = value + delta;
  if (v < 5) {
    return 5;
  }
  if (v > 100) {
    return 100;
  }
  return v;
}

inline int backlight_level(int percent) {
  if (percent < 5) {
    percent = 5;
  }
  if (percent > 100) {
    percent = 100;
  }
  return percent * 255 / 100;
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
    return MenuHit::ClockFace;
  }
  if (in_box(centered_button(4, 5, w, h, 180), x, y)) {
    return MenuHit::Display;
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
  if (in_box(back_button(6, w, h), x, y)) {
    return SoundPageHit::Back;
  }
  if (in_box(side_button(1, 6, w, h, false), x, y)) {
    return SoundPageHit::VolumeDown;
  }
  if (in_box(side_button(1, 6, w, h, true), x, y)) {
    return SoundPageHit::VolumeUp;
  }
  if (in_box(side_button(2, 6, w, h, false), x, y)) {
    return SoundPageHit::SoftDown;
  }
  if (in_box(side_button(2, 6, w, h, true), x, y)) {
    return SoundPageHit::SoftUp;
  }
  if (in_box(side_button(3, 6, w, h, false), x, y)) {
    return SoundPageHit::GentleDown;
  }
  if (in_box(side_button(3, 6, w, h, true), x, y)) {
    return SoundPageHit::GentleUp;
  }
  if (in_box(centered_button(4, 6, w, h, 200), x, y)) {
    return SoundPageHit::PreviewSoft;
  }
  if (in_box(centered_button(5, 6, w, h, 200), x, y)) {
    return SoundPageHit::PreviewLoud;
  }
  return SoundPageHit::None;
}

inline ClockPageHit clock_page_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return ClockPageHit::None;
  }
  if (in_box(back_button(3, w, h), x, y)) {
    return ClockPageHit::Back;
  }
  if (in_box(centered_button(1, 3, w, h, 220), x, y)) {
    return ClockPageHit::DayFormat;
  }
  if (in_box(centered_button(2, 3, w, h, 220), x, y)) {
    return ClockPageHit::NightFormat;
  }
  return ClockPageHit::None;
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

inline DisplayPageHit display_page_hit(int x, int y, int w, int h) {
  if (!in_bounds(x, y, w, h)) {
    return DisplayPageHit::None;
  }
  if (in_box(back_button(6, w, h), x, y)) {
    return DisplayPageHit::Back;
  }
  if (in_box(centered_button(1, 6, w, h, 200), x, y)) {
    return DisplayPageHit::Mode;
  }
  if (in_box(side_button(2, 6, w, h, false), x, y)) {
    return DisplayPageHit::DayDown;
  }
  if (in_box(side_button(2, 6, w, h, true), x, y)) {
    return DisplayPageHit::DayUp;
  }
  if (in_box(side_button(3, 6, w, h, false), x, y)) {
    return DisplayPageHit::NightDown;
  }
  if (in_box(side_button(3, 6, w, h, true), x, y)) {
    return DisplayPageHit::NightUp;
  }
  if (in_box(side_button(4, 6, w, h, false), x, y)) {
    return DisplayPageHit::StartDown;
  }
  if (in_box(side_button(4, 6, w, h, true), x, y)) {
    return DisplayPageHit::StartUp;
  }
  if (in_box(side_button(5, 6, w, h, false), x, y)) {
    return DisplayPageHit::EndDown;
  }
  if (in_box(side_button(5, 6, w, h, true), x, y)) {
    return DisplayPageHit::EndUp;
  }
  return DisplayPageHit::None;
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
  return true;
}

#endif
