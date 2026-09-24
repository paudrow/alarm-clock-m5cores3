#include <M5Unified.h>
#include <Preferences.h>

#include <cstdio>
#include <cstring>

#include "alarm_face.hpp"

namespace {

enum class Screen {
  Clock,
  Menu,
  AlarmPage,
  SoundPage,
  DayPage,
  NightPage,
  WhenPage,
};

uint16_t ink = TFT_RED;

Alarm kDefaultAlarm = {{7, 0}, false, false};

const char* occurrence_name(Occurrence o) {
  switch (o) {
    case Occurrence::Armed:
      return "armed";
    case Occurrence::Ringing:
      return "ringing";
    case Occurrence::Silenced:
      return "silenced";
    case Occurrence::Skipped:
      return "skipped";
  }
  return "armed";
}

const char* format_name(TimeFormat f) {
  switch (f) {
    case TimeFormat::Hidden:
      return "hidden";
    case TimeFormat::Hours:
      return "h";
    case TimeFormat::HoursMinutes:
      return "hm";
    case TimeFormat::HoursMinutesSeconds:
      return "hms";
  }
  return "hms";
}

int row_mid_y(int row, int rows, int h) {
  return (row * h / rows + (row + 1) * h / rows) / 2;
}

void fill_row(int row, int rows) {
  const int h = M5.Display.height();
  const int w = M5.Display.width();
  const int y0 = row * h / rows;
  const int y1 = (row + 1) * h / rows;
  M5.Display.fillRect(0, y0, w, y1 - y0, TFT_BLACK);
}

void draw_button(Box b, const char* label) {
  M5.Display.drawRect(b.x, b.y, b.w, b.h, ink);
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(label, b.x + b.w / 2, b.y + b.h / 2);
}

void draw_gear() {
  const int w = M5.Display.width();
  const Box b = gear_box(w);
  const int cx = b.x + b.w / 2;
  const int cy = b.y + b.h / 2;
  M5.Display.drawCircle(cx, cy, 10, ink);
  M5.Display.drawCircle(cx, cy, 4, ink);
  static const int dx[] = {0, 7, 10, 7, 0, -7, -10, -7};
  static const int dy[] = {-10, -7, 0, 7, 10, 7, 0, -7};
  for (int i = 0; i < 8; ++i) {
    const int ix = cx + dx[i];
    const int iy = cy + dy[i];
    const int ox = cx + (dx[i] * 14) / 10;
    const int oy = cy + (dy[i] * 14) / 10;
    M5.Display.drawLine(ix, iy, ox, oy, ink);
  }
}

void draw_bell(bool enabled, bool clear) {
  const int w = M5.Display.width();
  const Box b = bell_box(w);
  if (clear) {
    M5.Display.fillRect(b.x, b.y, b.w, b.h, TFT_BLACK);
  }
  const int cx = b.x + b.w / 2;
  const int cy = b.y + b.h / 2;
  M5.Display.drawCircle(cx, cy - 6, 9, ink);
  M5.Display.fillRect(cx - 9, cy - 6, 19, 10, TFT_BLACK);
  M5.Display.drawLine(cx - 9, cy - 6, cx - 11, cy + 8, ink);
  M5.Display.drawLine(cx + 9, cy - 6, cx + 11, cy + 8, ink);
  M5.Display.drawLine(cx - 11, cy + 8, cx + 11, cy + 8, ink);
  M5.Display.fillCircle(cx, cy + 12, 2, ink);
  if (!enabled) {
    M5.Display.drawLine(b.x + 6, b.y + 6, b.x + 34, b.y + 34, ink);
  }
}

void draw_skip(bool skip_next, bool clear) {
  const int w = M5.Display.width();
  const Box b = skip_box(w);
  if (clear) {
    M5.Display.fillRect(b.x, b.y, b.w, b.h, TFT_BLACK);
  }
  const int cx = b.x + b.w / 2;
  const int cy = b.y + b.h / 2;
  M5.Display.drawLine(cx - 10, cy - 8, cx - 2, cy, ink);
  M5.Display.drawLine(cx - 2, cy, cx - 10, cy + 8, ink);
  M5.Display.drawLine(cx - 2, cy - 8, cx + 6, cy, ink);
  M5.Display.drawLine(cx + 6, cy, cx - 2, cy + 8, ink);
  if (!skip_next) {
    M5.Display.drawLine(b.x + 6, b.y + 6, b.x + 34, b.y + 34, ink);
  }
}

struct TimePaint {
  int x;
  int y;
  int w;
  int h;
  float scale;
  bool use_font8;
  char text[24];
};

void draw_ring_actions(bool ringing, bool erase) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  const Box snooze = ring_action_box(w, h, false);
  const Box stop = ring_action_box(w, h, true);
  if (erase) {
    M5.Display.fillRect(snooze.x, snooze.y, snooze.w, snooze.h, TFT_BLACK);
    M5.Display.fillRect(stop.x, stop.y, stop.w, stop.h, TFT_BLACK);
  }
  if (!ringing) {
    return;
  }
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(ink, TFT_BLACK);
  draw_button(snooze, "Snooze");
  draw_button(stop, "Stop");
}

void draw_clock_time(const m5::rtc_datetime_t& dt, TimeFormat fmt,
                     HourCycle hours, bool entering, bool enabled,
                     bool skip_next, bool ringing, bool* last_enabled,
                     bool* last_skip_next, bool* last_ringing, TimePaint* last) {
  char digits[16];
  char period[4];
  format_clock_face(digits, sizeof(digits), period, sizeof(period), fmt,
                    dt.time.hours, dt.time.minutes, dt.time.seconds, hours);
  char buf[24];
  if (period[0] == '\0') {
    std::snprintf(buf, sizeof(buf), "%s", digits);
  } else {
    std::snprintf(buf, sizeof(buf), "%s %s", digits, period);
  }
  const bool enabled_changed = enabled != *last_enabled;
  const bool skip_changed = skip_next != *last_skip_next;
  const bool ringing_changed = ringing != *last_ringing;
  if (!entering && std::strcmp(buf, last->text) == 0 && !enabled_changed &&
      !skip_changed && !ringing_changed) {
    return;
  }

  const int w = M5.Display.width();
  const int h = M5.Display.height();

  bool use_font8 = digits[0] != '\0';
  M5.Display.setTextSize(1);
  M5.Display.setFont(&fonts::Font8);
  int tw = M5.Display.textWidth(digits);
  int th = M5.Display.fontHeight();
  if (digits[0] == '\0' || tw > w - 8) {
    use_font8 = false;
    M5.Display.setFont(&fonts::Font7);
    tw = M5.Display.textWidth(digits);
    th = M5.Display.fontHeight();
  }
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);
  const int pw = period[0] == '\0' ? 0 : M5.Display.textWidth(period);
  const int ph = period[0] == '\0' ? 0 : M5.Display.fontHeight();
  const int gap = period[0] == '\0' ? 0 : 8;
  float scale = 1;
  if (tw > 0 && th > 0) {
    const float room = static_cast<float>(w - 8 - gap - pw);
    const float sx = room / static_cast<float>(tw);
    const float sy = 150.f / static_cast<float>(th);
    scale = sx < sy ? sx : sy;
    if (scale < 1.f) {
      scale = 1.f;
    }
    if (scale > 2.2f) {
      scale = 2.2f;
    }
    if (ringing) {
      const int limit = ring_action_box(w, h, false).y - 6 - 52;
      const float sy_ring = static_cast<float>(limit) / static_cast<float>(th);
      if (sy_ring < scale) {
        scale = sy_ring;
      }
      if (scale < 1.f) {
        scale = 1.f;
      }
    }
  }
  const int floor_y = ringing ? ring_action_box(w, h, false).y - 6 : h - 4;
  const int scaled_w = static_cast<int>(tw * scale);
  const int scaled_h = static_cast<int>(th * scale);
  const int total_w = scaled_w + gap + pw;
  const int total_h = scaled_h > ph ? scaled_h : ph;
  int cy = h / 2 + 8;
  if (cy - total_h / 2 < 48) {
    cy = 48 + total_h / 2;
  }
  if (cy + total_h / 2 > floor_y) {
    cy = floor_y - total_h / 2;
  }
  const int cx = w / 2;
  const int box_x = cx - total_w / 2;
  const int box_y = cy - total_h / 2;
  const int digit_cx = box_x + scaled_w / 2;
  const int period_cx = box_x + scaled_w + gap + pw / 2;

  const bool font_changed = use_font8 != last->use_font8 || scale != last->scale;
  const bool shorter = std::strlen(buf) < std::strlen(last->text);

  M5.Display.startWrite();
  if (!entering && (shorter || font_changed) && last->w > 0) {
    M5.Display.fillRect(last->x, last->y, last->w, last->h, TFT_BLACK);
  }
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  if (digits[0] != '\0') {
    M5.Display.setFont(use_font8 ? &fonts::Font8 : &fonts::Font7);
    M5.Display.setTextSize(scale);
    M5.Display.drawString(digits, digit_cx, cy);
  }
  if (period[0] != '\0') {
    M5.Display.setFont(&fonts::Font4);
    M5.Display.setTextSize(1);
    M5.Display.drawString(period, period_cx, cy);
  }
  draw_gear();
  draw_bell(enabled, entering || enabled_changed);
  draw_skip(skip_next, entering || skip_changed);
  if (ringing_changed && !ringing) {
    draw_ring_actions(false, true);
  }
  if (ringing) {
    draw_ring_actions(true, false);
  }
  M5.Display.endWrite();

  last->x = box_x;
  last->y = box_y;
  last->w = total_w;
  last->h = total_h;
  last->scale = scale;
  last->use_font8 = use_font8;
  std::strcpy(last->text, buf);
  *last_enabled = enabled;
  *last_skip_next = skip_next;
  *last_ringing = ringing;
}

void draw_menu() {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);
  draw_button(back_button(5, w, h), "Back");
  M5.Display.setTextDatum(middle_right);
  M5.Display.drawString("Menu", w - 8, row_mid_y(0, 5, h));
  draw_button(centered_button(1, 5, w, h, 180), "Alarm");
  draw_button(centered_button(2, 5, w, h, 180), "Sound");
  draw_button(centered_button(3, 5, w, h, 180), "Day");
  draw_button(centered_button(4, 5, w, h, 180), "Night");
}

void draw_alarm_page(const Alarm& alarm, HourCycle hours, int dirty_row) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);

  if (dirty_row < 0 || dirty_row == 0) {
    if (dirty_row == 0) {
      fill_row(0, 3);
    }
    draw_button(back_button(3, w, h), "Back");
  }
  if (dirty_row < 0 || dirty_row == 1) {
    if (dirty_row == 1) {
      fill_row(1, 3);
    }
    char when[16];
    format_time(when, sizeof(when), TimeFormat::HoursMinutes, alarm.at.hour,
                alarm.at.minute, 0, hours);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s  %s", when, alarm.enabled ? "ON" : "OFF");
    draw_button(centered_button(1, 3, w, h, 220), buf);
  }
  if (dirty_row < 0 || dirty_row == 2) {
    if (dirty_row == 2) {
      fill_row(2, 3);
    }
    const char* steppers[] = {"H-", "H+", "M-", "M+"};
    for (int i = 0; i < 4; ++i) {
      draw_button(column_button(2, 3, i, 4, w, h), steppers[i]);
    }
  }
}

void draw_level_row(int row, int rows, const char* label);

void draw_sound_page(int volume, int soft_volume, int gentle_seconds,
                     int dirty_row) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextSize(1);

  if (dirty_row < 0 || dirty_row == 0) {
    if (dirty_row == 0) {
      fill_row(0, 5);
    }
    draw_button(back_button(5, w, h), "Back");
  }
  if (dirty_row < 0 || dirty_row == 1) {
    if (dirty_row == 1) {
      fill_row(1, 5);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Loud %d", volume);
    draw_level_row(1, 5, buf);
  }
  if (dirty_row < 0 || dirty_row == 2) {
    if (dirty_row == 2) {
      fill_row(2, 5);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Soft %d", soft_volume);
    draw_level_row(2, 5, buf);
  }
  if (dirty_row < 0 || dirty_row == 3) {
    if (dirty_row == 3) {
      fill_row(3, 5);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%ds then loud", gentle_seconds);
    draw_level_row(3, 5, buf);
  }
  if (dirty_row < 0 || dirty_row == 4) {
    if (dirty_row == 4) {
      fill_row(4, 5);
    }
    draw_button(column_button(4, 5, 0, 2, w, h), "Preview soft");
    draw_button(column_button(4, 5, 1, 2, w, h), "Preview loud");
  }
}

void draw_day_page(int day_bright, TimeFormat day_fmt, HourCycle hours,
                   int dirty_row) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);

  if (dirty_row < 0 || dirty_row == 0) {
    if (dirty_row == 0) {
      fill_row(0, 4);
    }
    draw_button(back_button(4, w, h), "Back");
  }
  if (dirty_row < 0 || dirty_row == 1) {
    if (dirty_row == 1) {
      fill_row(1, 4);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Bright %d", day_bright);
    draw_level_row(1, 4, buf);
  }
  if (dirty_row < 0 || dirty_row == 2) {
    if (dirty_row == 2) {
      fill_row(2, 4);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Face %s", format_label(day_fmt));
    draw_button(centered_button(2, 4, w, h, 220), buf);
  }
  if (dirty_row < 0 || dirty_row == 3) {
    if (dirty_row == 3) {
      fill_row(3, 4);
    }
    draw_button(centered_button(3, 4, w, h, 220), hour_cycle_label(hours));
  }
}

void draw_night_page(int night_bright, TimeFormat night_fmt, HourCycle hours,
                     Hm night_start, Hm night_end, int dirty_row) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);

  if (dirty_row < 0 || dirty_row == 0) {
    if (dirty_row == 0) {
      fill_row(0, 5);
    }
    draw_button(back_button(5, w, h), "Back");
  }
  if (dirty_row < 0 || dirty_row == 1) {
    if (dirty_row == 1) {
      fill_row(1, 5);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Bright %d", night_bright);
    draw_level_row(1, 5, buf);
  }
  if (dirty_row < 0 || dirty_row == 2) {
    if (dirty_row == 2) {
      fill_row(2, 5);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Face %s", format_label(night_fmt));
    draw_button(centered_button(2, 5, w, h, 220), buf);
  }
  if (dirty_row < 0 || dirty_row == 3) {
    if (dirty_row == 3) {
      fill_row(3, 5);
    }
    draw_button(centered_button(3, 5, w, h, 220), hour_cycle_label(hours));
  }
  if (dirty_row < 0 || dirty_row == 4) {
    if (dirty_row == 4) {
      fill_row(4, 5);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%02d:%02d-%02d:%02d", night_start.hour,
                  night_start.minute, night_end.hour, night_end.minute);
    draw_button(centered_button(4, 5, w, h, 220), buf);
  }
}

void draw_when_page(ColorMode mode, Hm night_start, Hm night_end, int dirty_row) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  M5.Display.setTextColor(ink, TFT_BLACK);
  M5.Display.setFont(&fonts::Font4);
  M5.Display.setTextSize(1);

  if (dirty_row < 0 || dirty_row == 0) {
    if (dirty_row == 0) {
      fill_row(0, 4);
    }
    draw_button(back_button(4, w, h), "Back");
  }
  if (dirty_row < 0 || dirty_row == 1) {
    if (dirty_row == 1) {
      fill_row(1, 4);
    }
    draw_button(centered_button(1, 4, w, h, 220), color_mode_label(mode));
  }
  if (dirty_row < 0 || dirty_row == 2) {
    if (dirty_row == 2) {
      fill_row(2, 4);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Start %02d:%02d", night_start.hour,
                  night_start.minute);
    draw_level_row(2, 4, buf);
  }
  if (dirty_row < 0 || dirty_row == 3) {
    if (dirty_row == 3) {
      fill_row(3, 4);
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "End %02d:%02d", night_end.hour,
                  night_end.minute);
    draw_level_row(3, 4, buf);
  }
}

void draw_level_row(int row, int rows, const char* label) {
  const int w = M5.Display.width();
  const int h = M5.Display.height();
  draw_button(side_button(row, rows, w, h, false), "-");
  draw_button(side_button(row, rows, w, h, true), "+");
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString(label, w / 2, row_mid_y(row, rows, h));
}

void drive_speaker(Intensity heard, int loud_percent, int soft_percent) {
  static Intensity prev = Intensity::Silent;
  static uint32_t next_beep_at = 0;
  static bool loud_high = false;

  if (heard != prev) {
    M5.Speaker.stop();
    prev = heard;
    next_beep_at = 0;
    loud_high = false;
  }

  if (heard == Intensity::Silent) {
    return;
  }

  const uint32_t now = millis();
  if (now < next_beep_at) {
    return;
  }

  if (heard == Intensity::Gentle) {
    M5.Speaker.setVolume(speaker_level(soft_percent));
    M5.Speaker.tone(440, 280);
    next_beep_at = now + 700;
  } else {
    M5.Speaker.setVolume(speaker_level(loud_percent));
    const int freq = loud_high ? 1760 : 880;
    loud_high = !loud_high;
    M5.Speaker.tone(freq, 90);
    next_beep_at = now + 140;
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);

  m5::rtc_datetime_t dt;
  if (!M5.Rtc.getDateTime(&dt) || rtc_needs_set(dt.date.year)) {
    YmdHms stamp = parse_compile_stamp(__DATE__, __TIME__);
    M5.Rtc.setDateTime(
        {{static_cast<int16_t>(stamp.year), static_cast<int8_t>(stamp.month),
          static_cast<int8_t>(stamp.date)},
         {static_cast<int8_t>(stamp.hour), static_cast<int8_t>(stamp.minute),
          static_cast<int8_t>(stamp.second)}});
  }

  if (M5.Speaker.isEnabled()) {
    M5.Speaker.setVolume(speaker_level(80));
  }
}

Settings capture_settings(const Alarm& alarm, int volume, int soft_volume,
                          int gentle_seconds, TimeFormat day_format,
                          TimeFormat night_format, ColorMode color_mode,
                          Hm night_start, Hm night_end, int day_bright,
                          int night_bright, HourCycle hours) {
  Settings s = {};
  s.magic = kSettingsMagic;
  s.version = kSettingsVersion;
  s.alarm_hour = static_cast<uint8_t>(alarm.at.hour);
  s.alarm_minute = static_cast<uint8_t>(alarm.at.minute);
  s.alarm_enabled = alarm.enabled ? 1 : 0;
  s.skip_next = alarm.skip_next ? 1 : 0;
  s.volume = static_cast<uint8_t>(volume);
  s.soft_volume = static_cast<uint8_t>(soft_volume);
  s.gentle_seconds = static_cast<uint8_t>(gentle_seconds);
  s.day_format = static_cast<uint8_t>(day_format);
  s.night_format = static_cast<uint8_t>(night_format);
  s.color_mode = static_cast<uint8_t>(color_mode);
  s.night_start_hour = static_cast<uint8_t>(night_start.hour);
  s.night_start_minute = static_cast<uint8_t>(night_start.minute);
  s.night_end_hour = static_cast<uint8_t>(night_end.hour);
  s.night_end_minute = static_cast<uint8_t>(night_end.minute);
  s.day_bright = static_cast<uint8_t>(day_bright);
  s.night_bright = static_cast<uint8_t>(night_bright);
  s.hour12 = hours == HourCycle::H12 ? 1 : 0;
  return s;
}

bool read_settings(Settings* out) {
  Preferences prefs;
  if (!prefs.begin("alarmclk", true)) {
    return false;
  }
  Settings s = {};
  const size_t n = prefs.getBytes("cfg", &s, sizeof(s));
  prefs.end();
  if (n != sizeof(s) || !settings_valid(s)) {
    return false;
  }
  *out = s;
  return true;
}

void write_settings(const Settings& s) {
  Preferences prefs;
  if (!prefs.begin("alarmclk", false)) {
    return;
  }
  prefs.putBytes("cfg", &s, sizeof(s));
  prefs.end();
}

void apply_settings(const Settings& s, Alarm* alarm, int* volume,
                    int* soft_volume, int* gentle_seconds,
                    TimeFormat* day_format, TimeFormat* night_format,
                    ColorMode* color_mode, Hm* night_start, Hm* night_end,
                    int* day_bright, int* night_bright, HourCycle* hours) {
  alarm->at.hour = s.alarm_hour;
  alarm->at.minute = s.alarm_minute;
  alarm->enabled = s.alarm_enabled != 0;
  alarm->skip_next = s.skip_next != 0;
  *volume = s.volume;
  *soft_volume = s.soft_volume;
  *gentle_seconds = s.gentle_seconds;
  *day_format = static_cast<TimeFormat>(s.day_format);
  *night_format = static_cast<TimeFormat>(s.night_format);
  *color_mode = static_cast<ColorMode>(s.color_mode);
  night_start->hour = s.night_start_hour;
  night_start->minute = s.night_start_minute;
  night_end->hour = s.night_end_hour;
  night_end->minute = s.night_end_minute;
  *day_bright = snap_brightness(s.day_bright);
  *night_bright = snap_brightness(s.night_bright);
  *hours = s.hour12 ? HourCycle::H12 : HourCycle::H24;
}

void loop() {
  static Screen screen = Screen::Clock;
  static Occurrence occurrence = Occurrence::Armed;
  static Alarm alarm = kDefaultAlarm;
  static int volume = 80;
  static int soft_volume = 40;
  static int gentle_seconds = kGentleSeconds;
  static TimeFormat day_format = TimeFormat::HoursMinutesSeconds;
  static TimeFormat night_format = TimeFormat::HoursMinutesSeconds;
  static bool snooze_armed = false;
  static Hm snooze_at = {0, 0};
  static bool settings_ready = false;
  static Settings stored = {};
  static bool preview_active = false;
  static Intensity preview_kind = Intensity::Gentle;
  static uint32_t preview_start = 0;
  static bool enter_screen = true;
  static int dirty_row = -1;
  static TimePaint last_time = {0, 0, 0, 0, 1.f, false, ""};
  static bool last_enabled = false;
  static bool last_skip_next = false;
  static bool last_ringing = false;
  static int last_logged_second = -1;
  static uint32_t next_repeat = 0;
  static ColorMode color_mode = ColorMode::Auto;
  static Hm night_start = {21, 0};
  static Hm night_end = {7, 0};
  static int day_bright = 70;
  static int night_bright = 20;
  static HourCycle hours = HourCycle::H24;
  static bool was_night = false;
  static int applied_light = -1;

  if (!settings_ready) {
    Settings loaded = {};
    if (read_settings(&loaded)) {
      apply_settings(loaded, &alarm, &volume, &soft_volume, &gentle_seconds,
                     &day_format, &night_format, &color_mode, &night_start,
                     &night_end, &day_bright, &night_bright, &hours);
      stored = loaded;
    }
    settings_ready = true;
  }

  M5.update();

  m5::rtc_datetime_t dt;
  M5.Rtc.getDateTime(&dt);

  bool stop_alarm = false;
  bool snooze_press = false;
  Intensity request_preview = Intensity::Silent;
  const int w = M5.Display.width();
  const int h = M5.Display.height();

  if (M5.Touch.getCount()) {
    auto t = M5.Touch.getDetail();
    const bool edge = t.wasPressed();

    if (screen == Screen::Clock) {
      if (edge) {
        const RingHit action =
            occurrence == Occurrence::Ringing ? ring_hit(t.x, t.y, w, h)
                                              : RingHit::None;
        if (action == RingHit::Stop) {
          stop_alarm = true;
        } else if (action == RingHit::Snooze) {
          snooze_press = true;
        } else {
          const ClockHit hit = clock_hit(t.x, t.y, w, h);
          if (hit == ClockHit::Gear) {
            screen = Screen::Menu;
            enter_screen = true;
          } else if (hit == ClockHit::Bell) {
            alarm = apply_zone(alarm, Zone::Toggle);
          } else if (hit == ClockHit::Skip) {
            alarm.skip_next = !alarm.skip_next;
          }
        }
      }
    } else if (screen == Screen::Menu) {
      if (edge) {
        switch (menu_hit(t.x, t.y, w, h)) {
          case MenuHit::Back:
            screen = Screen::Clock;
            enter_screen = true;
            break;
          case MenuHit::Alarm:
            screen = Screen::AlarmPage;
            enter_screen = true;
            break;
          case MenuHit::Sound:
            screen = Screen::SoundPage;
            enter_screen = true;
            break;
          case MenuHit::Day:
            screen = Screen::DayPage;
            enter_screen = true;
            break;
          case MenuHit::Night:
            screen = Screen::NightPage;
            enter_screen = true;
            break;
          default:
            break;
        }
      }
    } else if (screen == Screen::AlarmPage) {
      const AlarmPageHit hit = alarm_page_hit(t.x, t.y, w, h);
      const bool is_step = hit == AlarmPageHit::HourDown ||
                           hit == AlarmPageHit::HourUp ||
                           hit == AlarmPageHit::MinuteDown ||
                           hit == AlarmPageHit::MinuteUp;
      if (is_step && (edge || (t.isPressed() && millis() >= next_repeat))) {
        switch (hit) {
          case AlarmPageHit::HourDown:
            alarm = apply_zone(alarm, Zone::HourDown);
            break;
          case AlarmPageHit::HourUp:
            alarm = apply_zone(alarm, Zone::Hour);
            break;
          case AlarmPageHit::MinuteDown:
            alarm = apply_zone(alarm, Zone::MinuteDown);
            break;
          case AlarmPageHit::MinuteUp:
            alarm = apply_zone(alarm, Zone::Minute);
            break;
          default:
            break;
        }
        next_repeat = millis() + (edge ? 350 : 120);
        dirty_row = 1;
      } else if (edge) {
        switch (hit) {
          case AlarmPageHit::Back:
            screen = Screen::Menu;
            enter_screen = true;
            break;
          case AlarmPageHit::Toggle:
            alarm = apply_zone(alarm, Zone::Toggle);
            dirty_row = 1;
            break;
          default:
            break;
        }
      }
    } else if (screen == Screen::SoundPage) {
      const SoundPageHit hit = sound_page_hit(t.x, t.y, w, h);
      const bool is_step = hit == SoundPageHit::VolumeDown ||
                           hit == SoundPageHit::VolumeUp ||
                           hit == SoundPageHit::SoftDown ||
                           hit == SoundPageHit::SoftUp ||
                           hit == SoundPageHit::GentleDown ||
                           hit == SoundPageHit::GentleUp;
      if (is_step && (edge || (t.isPressed() && millis() >= next_repeat))) {
        switch (hit) {
          case SoundPageHit::VolumeDown: {
            volume = adjust_volume(volume, -5);
            const int capped = clamp_soft(soft_volume, volume);
            dirty_row = capped == soft_volume ? 1 : -3;
            soft_volume = capped;
            break;
          }
          case SoundPageHit::VolumeUp:
            volume = adjust_volume(volume, 5);
            dirty_row = 1;
            break;
          case SoundPageHit::SoftDown:
            soft_volume = clamp_soft(adjust_volume(soft_volume, -5), volume);
            dirty_row = 2;
            break;
          case SoundPageHit::SoftUp:
            soft_volume = clamp_soft(adjust_volume(soft_volume, 5), volume);
            dirty_row = 2;
            break;
          case SoundPageHit::GentleDown:
            gentle_seconds = adjust_gentle(gentle_seconds, -5);
            dirty_row = 3;
            break;
          case SoundPageHit::GentleUp:
            gentle_seconds = adjust_gentle(gentle_seconds, 5);
            dirty_row = 3;
            break;
          default:
            break;
        }
        next_repeat = millis() + (edge ? 350 : 120);
      } else if (edge) {
        switch (hit) {
          case SoundPageHit::Back:
            screen = Screen::Menu;
            enter_screen = true;
            break;
          case SoundPageHit::PreviewSoft:
            request_preview = Intensity::Gentle;
            break;
          case SoundPageHit::PreviewLoud:
            request_preview = Intensity::Loud;
            break;
          default:
            break;
        }
      }
    } else if (screen == Screen::DayPage) {
      const DayPageHit hit = day_page_hit(t.x, t.y, w, h);
      const bool is_step =
          hit == DayPageHit::BrightDown || hit == DayPageHit::BrightUp;
      if (is_step && (edge || (t.isPressed() && millis() >= next_repeat))) {
        if (hit == DayPageHit::BrightDown) {
          day_bright = adjust_brightness(day_bright, -10);
        } else {
          day_bright = adjust_brightness(day_bright, 10);
        }
        dirty_row = 1;
        next_repeat = millis() + (edge ? 350 : 120);
      } else if (edge) {
        if (hit == DayPageHit::Back) {
          screen = Screen::Menu;
          enter_screen = true;
        } else if (hit == DayPageHit::Face) {
          day_format = next_format(day_format);
          dirty_row = 2;
        } else if (hit == DayPageHit::Hour) {
          hours = next_hour_cycle(hours);
          dirty_row = 3;
        }
      }
    } else if (screen == Screen::NightPage) {
      const NightPageHit hit = night_page_hit(t.x, t.y, w, h);
      const bool is_step =
          hit == NightPageHit::BrightDown || hit == NightPageHit::BrightUp;
      if (is_step && (edge || (t.isPressed() && millis() >= next_repeat))) {
        if (hit == NightPageHit::BrightDown) {
          night_bright = adjust_brightness(night_bright, -10);
        } else {
          night_bright = adjust_brightness(night_bright, 10);
        }
        dirty_row = 1;
        next_repeat = millis() + (edge ? 350 : 120);
      } else if (edge) {
        if (hit == NightPageHit::Back) {
          screen = Screen::Menu;
          enter_screen = true;
        } else if (hit == NightPageHit::Face) {
          night_format = next_format(night_format);
          dirty_row = 2;
        } else if (hit == NightPageHit::Hour) {
          hours = next_hour_cycle(hours);
          dirty_row = 3;
        } else if (hit == NightPageHit::When) {
          screen = Screen::WhenPage;
          enter_screen = true;
        }
      }
    } else if (screen == Screen::WhenPage) {
      const WhenPageHit hit = when_page_hit(t.x, t.y, w, h);
      const bool is_step = hit == WhenPageHit::StartDown ||
                           hit == WhenPageHit::StartUp ||
                           hit == WhenPageHit::EndDown ||
                           hit == WhenPageHit::EndUp;
      if (is_step && (edge || (t.isPressed() && millis() >= next_repeat))) {
        switch (hit) {
          case WhenPageHit::StartDown:
            night_start = step_minutes(night_start, -30);
            dirty_row = 2;
            break;
          case WhenPageHit::StartUp:
            night_start = step_minutes(night_start, 30);
            dirty_row = 2;
            break;
          case WhenPageHit::EndDown:
            night_end = step_minutes(night_end, -30);
            dirty_row = 3;
            break;
          case WhenPageHit::EndUp:
            night_end = step_minutes(night_end, 30);
            dirty_row = 3;
            break;
          default:
            break;
        }
        next_repeat = millis() + (edge ? 350 : 120);
      } else if (edge) {
        if (hit == WhenPageHit::Back) {
          screen = Screen::NightPage;
          enter_screen = true;
        } else if (hit == WhenPageHit::Mode) {
          color_mode = next_color_mode(color_mode);
          dirty_row = 1;
        }
      }
    }
  }

  Inputs in = {{dt.time.hours, dt.time.minutes},
               dt.time.seconds,
               stop_alarm,
               gentle_seconds,
               snooze_press,
               snooze_armed,
               snooze_at};
  Step step = step_alarm(occurrence, alarm, in);
  occurrence = step.occurrence;
  alarm.skip_next = step.skip_next;
  snooze_armed = step.snooze_armed;
  snooze_at = step.snooze_at;

  if (request_preview != Intensity::Silent &&
      step.intensity == Intensity::Silent) {
    preview_active = true;
    preview_kind = request_preview;
    preview_start = millis();
  }

  Intensity heard = step.intensity;
  if (step.intensity == Intensity::Gentle ||
      step.intensity == Intensity::Loud) {
    preview_active = false;
    heard = step.intensity;
  } else if (preview_active) {
    if (preview_playing(millis() - preview_start)) {
      heard = preview_kind;
    } else {
      preview_active = false;
      heard = Intensity::Silent;
    }
  }

  drive_speaker(heard, volume, soft_volume);

  const Hm now_hm = {dt.time.hours, dt.time.minutes};
  const bool night = color_mode == ColorMode::Night ||
                     (color_mode == ColorMode::Auto &&
                      is_night(now_hm, night_start, night_end));
  bool show_night = night;
  int bright_percent = night ? night_bright : day_bright;
  if (screen == Screen::DayPage) {
    show_night = false;
    bright_percent = day_bright;
  } else if (screen == Screen::NightPage) {
    show_night = true;
    bright_percent = night_bright;
  }
  ink = show_night ? TFT_RED : TFT_WHITE;
  if (show_night != was_night) {
    was_night = show_night;
    enter_screen = true;
  }
  const int light = backlight_level(bright_percent);
  if (light != applied_light) {
    M5.Display.setBrightness(static_cast<uint8_t>(light));
    applied_light = light;
  }

  const TimeFormat face = night ? night_format : day_format;
  const bool ringing = occurrence == Occurrence::Ringing;

  if (screen == Screen::Clock) {
    if (enter_screen) {
      M5.Display.fillScreen(TFT_BLACK);
      last_time.text[0] = '\0';
      last_time.w = 0;
      last_enabled = !alarm.enabled;
      last_skip_next = !alarm.skip_next;
      last_ringing = !ringing;
      draw_clock_time(dt, face, hours, true, alarm.enabled, alarm.skip_next, ringing,
                      &last_enabled, &last_skip_next, &last_ringing, &last_time);
      enter_screen = false;
    } else {
      draw_clock_time(dt, face, hours, false, alarm.enabled, alarm.skip_next, ringing,
                      &last_enabled, &last_skip_next, &last_ringing, &last_time);
    }
  } else if (screen == Screen::Menu) {
    if (enter_screen) {
      M5.Display.fillScreen(TFT_BLACK);
      draw_menu();
      enter_screen = false;
      dirty_row = -1;
    }
  } else if (screen == Screen::AlarmPage) {
    if (enter_screen) {
      M5.Display.fillScreen(TFT_BLACK);
      draw_alarm_page(alarm, hours, -1);
      enter_screen = false;
      dirty_row = -1;
    } else if (dirty_row >= 0) {
      draw_alarm_page(alarm, hours, dirty_row);
      dirty_row = -1;
    }
  } else if (screen == Screen::SoundPage) {
    if (enter_screen) {
      M5.Display.fillScreen(TFT_BLACK);
      draw_sound_page(volume, soft_volume, gentle_seconds, -1);
      enter_screen = false;
      dirty_row = -1;
    } else if (dirty_row == -3) {
      draw_sound_page(volume, soft_volume, gentle_seconds, 1);
      draw_sound_page(volume, soft_volume, gentle_seconds, 2);
      dirty_row = -1;
    } else if (dirty_row >= 0) {
      draw_sound_page(volume, soft_volume, gentle_seconds, dirty_row);
      dirty_row = -1;
    }
  } else if (screen == Screen::DayPage) {
    if (enter_screen) {
      M5.Display.fillScreen(TFT_BLACK);
      draw_day_page(day_bright, day_format, hours, -1);
      enter_screen = false;
      dirty_row = -1;
    } else if (dirty_row >= 0) {
      draw_day_page(day_bright, day_format, hours, dirty_row);
      dirty_row = -1;
    }
  } else if (screen == Screen::NightPage) {
    if (enter_screen) {
      M5.Display.fillScreen(TFT_BLACK);
      draw_night_page(night_bright, night_format, hours, night_start, night_end,
                      -1);
      enter_screen = false;
      dirty_row = -1;
    } else if (dirty_row >= 0) {
      draw_night_page(night_bright, night_format, hours, night_start, night_end,
                      dirty_row);
      dirty_row = -1;
    }
  } else if (screen == Screen::WhenPage) {
    if (enter_screen) {
      M5.Display.fillScreen(TFT_BLACK);
      draw_when_page(color_mode, night_start, night_end, -1);
      enter_screen = false;
      dirty_row = -1;
    } else if (dirty_row >= 0) {
      draw_when_page(color_mode, night_start, night_end, dirty_row);
      dirty_row = -1;
    }
  }

  if (dt.time.seconds != last_logged_second) {
    last_logged_second = dt.time.seconds;
    Serial.printf(
        "%02d:%02d:%02d %s alarm=%02d:%02d %s skip=%d vol=%d svol=%d soft=%d "
        "dfmt=%s nfmt=%s tone=%s bright=%d\n",
        dt.time.hours, dt.time.minutes, dt.time.seconds,
        occurrence_name(occurrence), alarm.at.hour, alarm.at.minute,
        alarm.enabled ? "on" : "off", alarm.skip_next ? 1 : 0, volume,
        soft_volume, gentle_seconds, format_name(day_format),
        format_name(night_format), night ? "night" : "day", bright_percent);
  }

  const Settings current =
      capture_settings(alarm, volume, soft_volume, gentle_seconds, day_format,
                       night_format, color_mode, night_start, night_end,
                       day_bright, night_bright, hours);
  if (std::memcmp(&current, &stored, sizeof(current)) != 0) {
    if (settings_valid(current)) {
      write_settings(current);
      stored = current;
    }
  }

  M5.delay(10);
}
