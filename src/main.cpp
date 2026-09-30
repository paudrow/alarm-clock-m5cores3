#include <M5Unified.h>
#include <Preferences.h>

#include <cstdio>
#include <cstring>

#include "alarm_face.hpp"

namespace {

// Lamp driver set-point: CoreS3 Port B. On the V2 bench this feeds the RC
// filter in front of the lamp's op-amp (hardware/DESIGN.md, "Red lamp").
constexpr int kLampPin = 9;

// Speaker channels: the alarm's beeps and the sleep sounds mix independently.
constexpr uint8_t kAlarmChannel = 0;
constexpr uint8_t kNoiseChannel = 1;
constexpr uint32_t kNoiseRate = 24000;
// ~170 ms. M5Unified queues at most two buffers per channel, so each must
// outlast the slowest loop (a full redraw, a flash write) or the sound gaps.
constexpr size_t kNoiseChunk = 4096;
constexpr int kNoiseBuffers = 3;

constexpr uint32_t kIdleMs = 60000;  // settings pages go back to the clock
constexpr uint32_t kRepeatDelayMs = 400;
constexpr uint32_t kRepeatMs = 110;
// Settings are written this long after the last change, so holding a stepper
// doesn't write flash ten times a second (and stall the sleep sounds).
constexpr uint32_t kSaveDelayMs = 1500;

struct Palette {
  uint16_t bg;
  uint16_t ink;
  uint16_t dim;
  uint16_t card;
  uint16_t button;
  uint16_t line;
};

// Day is white on black. Night is red only: red subpixels age slowest, and
// red light is the easiest on sleep.
const Palette kDay = {0x0000, 0xFFFF, 0x94B2, 0x18C3, 0x31A6, 0x4208};
const Palette kNight = {0x0000, 0xF800, 0x9000, 0x2000, 0x4000, 0x6000};

const lgfx::IFont* const kRegular[] = {
    &fonts::FreeSans9pt7b, &fonts::FreeSans12pt7b, &fonts::FreeSans18pt7b,
    &fonts::FreeSans24pt7b};
const lgfx::IFont* const kBold[] = {
    &fonts::FreeSansBold9pt7b, &fonts::FreeSansBold12pt7b,
    &fonts::FreeSansBold18pt7b, &fonts::FreeSansBold24pt7b};

struct RowText {
  char label[28];
  char value[28];
  bool on;
};

struct TileText {
  char title[20];
  char sub[36];
};

struct ClockView {
  char digits[16];
  char period[4];
  char status[96];
  bool enabled;
  bool skip;
  bool lamp;
  bool sound;
  bool ringing;
};

struct App {
  Settings cfg;
  Settings stored;
  Screen screen;
  bool enter;
  Occurrence occurrence;
  bool snooze_armed;
  Hm snooze_at;

  bool preview_active;
  Intensity preview_kind;
  uint32_t preview_start;
  Intensity heard;
  uint32_t next_beep_at;
  bool loud_high;

  Hit press;
  uint32_t next_repeat;
  uint32_t last_touch_ms;

  bool noise_on;
  uint32_t noise_start;
  NoiseGen gen;
  int noise_gain;
  int noise_buf;
  int noise_volume_applied;

  bool wake_light;
  bool sunrise_dismissed;
  int lamp_level;  // permille
  int lamp_duty_applied;

  bool night;
  bool night_look;
  int bright_percent;
  int applied_light;

  bool clock_drawn;
  ClockView last_clock;
  Box last_digits;
  bool rows_drawn;
  RowText last_rows[kMaxRows];
  TileText last_tiles[kMenuItems];

  Settings pending;
  uint32_t changed_ms;

  LineReader line;
  int last_logged_second;
  bool pending_snooze;
};

App app;
int16_t noise_bufs[kNoiseBuffers][kNoiseChunk];

const Palette& palette() { return app.night_look ? kNight : kDay; }

Ui screen_ui() { return ui_for(M5.Display.width(), M5.Display.height()); }

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

HourCycle hours() {
  return app.cfg.hour12 ? HourCycle::H12 : HourCycle::H24;
}

Hm night_start() { return Hm{app.cfg.night_start_hour, app.cfg.night_start_minute}; }
Hm night_end() { return Hm{app.cfg.night_end_hour, app.cfg.night_end_minute}; }

// ---------------------------------------------------------------------------
// Drawing helpers

void fill_box(Box b, uint16_t c) {
  if (b.w > 0 && b.h > 0) {
    M5.Display.fillRect(b.x, b.y, b.w, b.h, c);
  }
}

Box clamp_box(const Ui& ui, Box b) {
  const int x0 = b.x < 0 ? 0 : b.x;
  const int y0 = b.y < 0 ? 0 : b.y;
  const int x1 = b.x + b.w > ui.w ? ui.w : b.x + b.w;
  const int y1 = b.y + b.h > ui.h ? ui.h : b.y + b.h;
  return Box{x0, y0, x1 - x0, y1 - y0};
}

Box inset(Box b, int d) { return Box{b.x + d, b.y + d, b.w - 2 * d, b.h - 2 * d}; }

void fill_round(Box b, int r, uint16_t c) {
  if (b.w <= 0 || b.h <= 0) {
    return;
  }
  if (r > b.h / 2) {
    r = b.h / 2;
  }
  if (r > b.w / 2) {
    r = b.w / 2;
  }
  M5.Display.fillRoundRect(b.x, b.y, b.w, b.h, r, c);
}

void stroke_round(Box b, int r, uint16_t c) {
  if (b.w <= 0 || b.h <= 0) {
    return;
  }
  if (r > b.h / 2) {
    r = b.h / 2;
  }
  if (r > b.w / 2) {
    r = b.w / 2;
  }
  M5.Display.drawRoundRect(b.x, b.y, b.w, b.h, r, c);
}

// Picks the largest font no taller than max_h whose rendering of `text` fits
// max_w. On a screen too small for even the smallest font, that one is scaled
// down to fit; if it's still too wide, `text` is cut short and ends "..".
void fit_text(char* text, size_t cap, int max_w, int max_h, bool bold) {
  const lgfx::IFont* const* ladder = bold ? kBold : kRegular;
  M5.Display.setTextSize(1);
  if (max_w <= 0 || max_h <= 0) {
    text[0] = '\0';
    return;
  }
  for (int i = 3; i >= 1; --i) {
    M5.Display.setFont(ladder[i]);
    if (M5.Display.fontHeight() <= max_h && M5.Display.textWidth(text) <= max_w) {
      return;
    }
  }
  M5.Display.setFont(ladder[0]);
  const int fh = M5.Display.fontHeight();
  if (fh > max_h) {
    M5.Display.setTextSize(static_cast<float>(max_h) / static_cast<float>(fh));
  }
  if (M5.Display.textWidth(text) <= max_w) {
    return;
  }
  char trimmed[64];
  size_t n = std::strlen(text);
  while (n > 0) {
    --n;
    std::snprintf(trimmed, sizeof(trimmed), "%.*s..", static_cast<int>(n), text);
    if (M5.Display.textWidth(trimmed) <= max_w) {
      std::snprintf(text, cap, "%s", trimmed);
      return;
    }
  }
  text[0] = '\0';
}

enum class Align { Left, Center, Right };

void draw_text(Box area, const char* s, Align align, uint16_t color, bool bold,
               int max_h) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%s", s);
  if (max_h > area.h) {
    max_h = area.h;
  }
  fit_text(buf, sizeof(buf), area.w, max_h, bold);
  if (buf[0] == '\0') {
    return;
  }
  M5.Display.setTextColor(color);
  const int y = area.y + area.h / 2;
  if (align == Align::Left) {
    M5.Display.setTextDatum(middle_left);
    M5.Display.drawString(buf, area.x, y);
  } else if (align == Align::Right) {
    M5.Display.setTextDatum(middle_right);
    M5.Display.drawString(buf, area.x + area.w, y);
  } else {
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString(buf, area.x + area.w / 2, y);
  }
}

// A thick line, for icons that must stay legible when scaled up.
void thick_line(int x0, int y0, int x1, int y1, int t, uint16_t c) {
  for (int i = 0; i < t; ++i) {
    const int o = i - t / 2;
    const bool steep = (y1 - y0) * (y1 - y0) > (x1 - x0) * (x1 - x0);
    if (steep) {
      M5.Display.drawLine(x0 + o, y0, x1 + o, y1, c);
    } else {
      M5.Display.drawLine(x0, y0 + o, x1, y1 + o, c);
    }
  }
}

void draw_plus_minus(Box b, bool plus, uint16_t c) {
  const int arm = b.h / 5 < 3 ? 3 : b.h / 5;
  const int t = b.h / 14 < 2 ? 2 : b.h / 14;
  const int cx = b.x + b.w / 2;
  const int cy = b.y + b.h / 2;
  M5.Display.fillRect(cx - arm, cy - t / 2, 2 * arm, t, c);
  if (plus) {
    M5.Display.fillRect(cx - t / 2, cy - arm, t, 2 * arm, c);
  }
}

void draw_chevron(int cx, int cy, int size, bool left, uint16_t c) {
  const int t = size / 5 < 2 ? 2 : size / 5;
  const int d = left ? -size / 2 : size / 2;
  thick_line(cx - d, cy - size, cx + d, cy, t, c);
  thick_line(cx + d, cy, cx - d, cy + size, t, c);
}

// ---------------------------------------------------------------------------
// Settings pages

void draw_header(const Ui& ui, const char* title) {
  const Palette& p = palette();
  const Box back = header_back_box(ui);
  fill_round(back, ui.radius, p.card);
  const int ch = back.h / 5;
  draw_chevron(back.x + back.h / 2, back.y + back.h / 2, ch, true, p.ink);
  draw_text(Box{back.x + back.h * 3 / 4 + ui.gap, back.y,
                back.w - back.h * 3 / 4 - 2 * ui.gap, back.h},
            "Back", Align::Left, p.ink, true, text_px(ui));
  draw_text(header_title_box(ui), title, Align::Left, p.ink, true,
            title_px(ui));
  M5.Display.fillRect(0, ui.header - 1, ui.w, 1, p.line);
}

void draw_switch(Box area, bool on, const Palette& p) {
  const int h = area.h * 11 / 20;
  const int w = h * 9 / 5;
  const Box pill = {area.x + area.w - w, area.y + (area.h - h) / 2, w, h};
  const int knob = h / 2 - (h / 8 < 2 ? 2 : h / 8);
  if (on) {
    fill_round(pill, h / 2, p.ink);
    M5.Display.fillCircle(pill.x + pill.w - h / 2, pill.y + h / 2, knob, p.bg);
  } else {
    fill_round(pill, h / 2, p.button);
    stroke_round(pill, h / 2, p.line);
    M5.Display.fillCircle(pill.x + h / 2, pill.y + h / 2, knob, p.dim);
  }
}

void draw_row(const Ui& ui, int i, int n, RowKind kind, const RowText& t) {
  const Palette& p = palette();
  const Box row = row_box(ui, i, n);
  fill_box(row, p.bg);
  const int tp = text_px(ui);
  switch (kind) {
    case RowKind::Stepper: {
      fill_round(row, ui.radius, p.card);
      draw_text(row_label_box(ui, row, kind), t.label, Align::Left, p.ink,
                false, tp);
      const Box minus = inset(row_part_box(ui, row, Part::Minus), ui.gap / 2 + 1);
      const Box plus = inset(row_part_box(ui, row, Part::Plus), ui.gap / 2 + 1);
      fill_round(minus, ui.radius, p.button);
      fill_round(plus, ui.radius, p.button);
      draw_plus_minus(minus, false, p.ink);
      draw_plus_minus(plus, true, p.ink);
      draw_text(inset(stepper_value_box(ui, row), 1), t.value, Align::Center,
                p.ink, true, tp);
      break;
    }
    case RowKind::Cycle:
    case RowKind::Link: {
      fill_round(row, ui.radius, p.card);
      const Box label = row_label_box(ui, row, kind);
      const int inset_x = ui.pad + ui.gap;
      int right = row.x + row.w - inset_x;
      if (kind == RowKind::Link) {
        const int size = row.h / 7 < 3 ? 3 : row.h / 7;
        draw_chevron(right - size / 2, row.y + row.h / 2, size, false, p.dim);
        right -= size + ui.gap * 2;
      }
      const int half = label.w * 11 / 20;
      draw_text(Box{label.x, row.y, half - ui.gap, row.h}, t.label, Align::Left,
                p.ink, false, tp);
      draw_text(Box{label.x + half, row.y, right - (label.x + half), row.h},
                t.value, Align::Right, kind == RowKind::Link ? p.dim : p.ink,
                true, tp);
      break;
    }
    case RowKind::Toggle: {
      fill_round(row, ui.radius, p.card);
      draw_text(row_label_box(ui, row, kind), t.label, Align::Left, p.ink,
                false, tp);
      const int inset_x = ui.pad + ui.gap;
      draw_switch(Box{row.x + row.w - inset_x - 2 * row.h, row.y + 2,
                      2 * row.h, row.h - 4},
                  t.on, p);
      break;
    }
    case RowKind::Pair: {
      const Box parts[2] = {row_part_box(ui, row, Part::Left),
                            row_part_box(ui, row, Part::Right)};
      const char* labels[2] = {t.label, t.value};
      for (int k = 0; k < 2; ++k) {
        fill_round(parts[k], ui.radius, p.card);
        stroke_round(parts[k], ui.radius, p.line);
        draw_text(inset(parts[k], ui.gap), labels[k], Align::Center, p.ink,
                  true, tp);
      }
      break;
    }
    case RowKind::Action:
      fill_round(row, ui.radius, t.on ? p.button : p.ink);
      draw_text(inset(row, ui.gap), t.label, Align::Center, t.on ? p.ink : p.bg,
                true, tp);
      break;
  }
}

void set_row(RowText* r, const char* label, const char* value, bool on) {
  std::memset(r, 0, sizeof(*r));
  std::snprintf(r->label, sizeof(r->label), "%s", label);
  std::snprintf(r->value, sizeof(r->value), "%s", value);
  r->on = on;
}

void percent(char* buf, size_t n, int v) { std::snprintf(buf, n, "%d%%", v); }

// "9PM-7AM" or "21:00-07:00"
void night_window_text(char* buf, size_t n) {
  if (app.cfg.color_mode == static_cast<uint8_t>(ColorMode::Day)) {
    std::snprintf(buf, n, "Never");
    return;
  }
  if (app.cfg.color_mode == static_cast<uint8_t>(ColorMode::Night)) {
    std::snprintf(buf, n, "Always");
    return;
  }
  char a[12];
  char b[12];
  format_hm_short(a, sizeof(a), night_start(), hours());
  format_hm_short(b, sizeof(b), night_end(), hours());
  std::snprintf(buf, n, "%s-%s", a, b);
}

bool lamp_lit() { return app.lamp_level > 0; }

uint32_t noise_elapsed() { return millis() - app.noise_start; }

int page_texts(Screen s, RowText* rows) {
  RowKind kinds[kMaxRows];
  const int n = page_rows(s, kinds);
  const Settings& c = app.cfg;
  char v[28];
  switch (s) {
    case Screen::Alarm: {
      set_row(&rows[0], "Alarm", "", c.alarm_enabled != 0);
      if (hours() == HourCycle::H12) {
        std::snprintf(v, sizeof(v), "%d %s", clock_hour(c.alarm_hour, hours()),
                      day_period(c.alarm_hour));
      } else {
        std::snprintf(v, sizeof(v), "%02d", c.alarm_hour);
      }
      set_row(&rows[1], "Hour", v, false);
      std::snprintf(v, sizeof(v), "%02d", c.alarm_minute);
      set_row(&rows[2], "Minute", v, false);
      set_row(&rows[3], "Skip next", "", c.skip_next != 0);
      break;
    }
    case Screen::AlarmSound: {
      percent(v, sizeof(v), c.volume);
      set_row(&rows[0], "Loud", v, false);
      percent(v, sizeof(v), c.soft_volume);
      set_row(&rows[1], "Soft", v, false);
      std::snprintf(v, sizeof(v), "%d s", c.gentle_seconds);
      set_row(&rows[2], "Soft for", v, false);
      const bool soft = app.preview_active && app.preview_kind == Intensity::Gentle;
      const bool loud = app.preview_active && app.preview_kind == Intensity::Loud;
      set_row(&rows[3], soft ? "Playing" : "Try soft", loud ? "Playing" : "Try loud",
              false);
      break;
    }
    case Screen::SleepSounds: {
      set_row(&rows[0], "Sound", noise_label(static_cast<NoiseKind>(c.noise_kind)),
              false);
      percent(v, sizeof(v), c.noise_volume);
      set_row(&rows[1], "Volume", v, false);
      if (c.noise_timer == 0) {
        std::snprintf(v, sizeof(v), "No timer");
      } else {
        std::snprintf(v, sizeof(v), "%d min", c.noise_timer);
      }
      set_row(&rows[2], "Timer", v, false);
      if (app.noise_on && c.noise_timer > 0) {
        std::snprintf(v, sizeof(v), "Stop (%d min left)",
                      noise_minutes_left(noise_elapsed(), c.noise_timer));
      } else {
        std::snprintf(v, sizeof(v), "%s", app.noise_on ? "Stop" : "Play");
      }
      set_row(&rows[3], v, "", app.noise_on);
      break;
    }
    case Screen::Lamp: {
      set_row(&rows[0], "Lamp", "", lamp_lit());
      percent(v, sizeof(v), c.lamp_bright);
      set_row(&rows[1], "Brightness", v, false);
      if (c.sunrise_minutes == 0) {
        std::snprintf(v, sizeof(v), "Off");
      } else {
        std::snprintf(v, sizeof(v), "%d min", c.sunrise_minutes);
      }
      set_row(&rows[2], "Sunrise", v, false);
      break;
    }
    case Screen::DayScreen:
      percent(v, sizeof(v), c.day_bright);
      set_row(&rows[0], "Brightness", v, false);
      set_row(&rows[1], "Clock face",
              format_label(static_cast<TimeFormat>(c.day_format)), false);
      set_row(&rows[2], "Hours", hour_cycle_label(hours()), false);
      break;
    case Screen::NightScreen:
      percent(v, sizeof(v), c.night_bright);
      set_row(&rows[0], "Brightness", v, false);
      set_row(&rows[1], "Clock face",
              format_label(static_cast<TimeFormat>(c.night_format)), false);
      night_window_text(v, sizeof(v));
      set_row(&rows[2], "Night hours", v, false);
      break;
    case Screen::NightHours:
      set_row(&rows[0], "Night screen",
              color_mode_label(static_cast<ColorMode>(c.color_mode)), false);
      format_hm(v, sizeof(v), night_start(), hours());
      set_row(&rows[1], "Starts", v, false);
      format_hm(v, sizeof(v), night_end(), hours());
      set_row(&rows[2], "Ends", v, false);
      break;
    case Screen::Clock:
    case Screen::Settings:
      break;
  }
  return n;
}

void draw_page(const Ui& ui) {
  RowKind kinds[kMaxRows];
  const int n = page_rows(app.screen, kinds);
  RowText rows[kMaxRows];
  std::memset(rows, 0, sizeof(rows));
  page_texts(app.screen, rows);
  if (!app.rows_drawn) {
    draw_header(ui, screen_title(app.screen));
  }
  for (int i = 0; i < n; ++i) {
    if (!app.rows_drawn ||
        std::memcmp(&rows[i], &app.last_rows[i], sizeof(RowText)) != 0) {
      draw_row(ui, i, n, kinds[i], rows[i]);
      app.last_rows[i] = rows[i];
    }
  }
  app.rows_drawn = true;
}

// ---------------------------------------------------------------------------
// The settings menu

void tile_text(Screen s, TileText* t) {
  std::memset(t, 0, sizeof(*t));
  std::snprintf(t->title, sizeof(t->title), "%s", screen_title(s));
  const Settings& c = app.cfg;
  char a[16];
  switch (s) {
    case Screen::Alarm:
      format_hm(a, sizeof(a), Hm{c.alarm_hour, c.alarm_minute}, hours());
      std::snprintf(t->sub, sizeof(t->sub), "%s, %s", a,
                    !c.alarm_enabled ? "off" : (c.skip_next ? "skip next" : "on"));
      break;
    case Screen::AlarmSound:
      std::snprintf(t->sub, sizeof(t->sub), "%d%%, soft %d%%", c.volume,
                    c.soft_volume);
      break;
    case Screen::SleepSounds:
      if (app.noise_on && c.noise_timer > 0) {
        std::snprintf(t->sub, sizeof(t->sub), "On, %d min left",
                      noise_minutes_left(noise_elapsed(), c.noise_timer));
      } else if (app.noise_on) {
        std::snprintf(t->sub, sizeof(t->sub), "On");
      } else {
        std::snprintf(t->sub, sizeof(t->sub), "%s",
                      noise_label(static_cast<NoiseKind>(c.noise_kind)));
      }
      break;
    case Screen::Lamp:
      if (lamp_lit()) {
        std::snprintf(t->sub, sizeof(t->sub), "On, %d%%", (app.lamp_level + 5) / 10);
      } else if (c.sunrise_minutes > 0) {
        std::snprintf(t->sub, sizeof(t->sub), "Off, sunrise %d min",
                      c.sunrise_minutes);
      } else {
        std::snprintf(t->sub, sizeof(t->sub), "Off");
      }
      break;
    case Screen::DayScreen:
      std::snprintf(t->sub, sizeof(t->sub), "%d%%, %s", c.day_bright,
                    format_label(static_cast<TimeFormat>(c.day_format)));
      break;
    case Screen::NightScreen: {
      night_window_text(t->sub, sizeof(t->sub));
      break;
    }
    case Screen::Clock:
    case Screen::Settings:
    case Screen::NightHours:
      break;
  }
}

void draw_tile(const Ui& ui, int i, const TileText& t) {
  const Palette& p = palette();
  const Box b = tile_box(ui, i, kMenuItems);
  fill_box(b, p.bg);
  fill_round(b, ui.radius, p.card);
  const int in = ui.pad + 1;
  const Box text = {b.x + in, b.y + ui.gap, b.w - 2 * in, b.h - 2 * ui.gap};
  // Title over the current setting; on a tiny screen, just the title.
  M5.Display.setFont(kRegular[0]);
  M5.Display.setTextSize(1);
  if (text.h < 2 * M5.Display.fontHeight() * 4 / 5) {
    draw_text(text, t.title, Align::Left, p.ink, true, text_px(ui));
    return;
  }
  draw_text(Box{text.x, text.y, text.w, text.h * 11 / 20}, t.title, Align::Left,
            p.ink, true, text_px(ui));
  draw_text(Box{text.x, text.y + text.h * 11 / 20, text.w, text.h * 9 / 20},
            t.sub, Align::Left, p.dim, false, small_px(ui));
}

void draw_menu(const Ui& ui) {
  if (!app.rows_drawn) {
    draw_header(ui, screen_title(Screen::Settings));
  }
  for (int i = 0; i < kMenuItems; ++i) {
    TileText t;
    tile_text(menu_item(i), &t);
    if (!app.rows_drawn ||
        std::memcmp(&t, &app.last_tiles[i], sizeof(TileText)) != 0) {
      draw_tile(ui, i, t);
      app.last_tiles[i] = t;
    }
  }
  app.rows_drawn = true;
}

// ---------------------------------------------------------------------------
// The clock face

void draw_bell(Box b, bool enabled, const Palette& p) {
  fill_box(b, p.bg);
  const uint16_t c = enabled ? p.ink : p.dim;
  const int s = b.w;
  auto S = [s](int v) { return v * s / 40; };
  const int cx = b.x + s / 2;
  const int cy = b.y + s / 2;
  const int r = S(9);
  M5.Display.fillCircle(cx, cy - S(4), r, c);
  M5.Display.fillRect(cx - r, cy - S(4), 2 * r + 1, S(8), c);
  M5.Display.fillTriangle(cx - r, cy + S(4), cx - r - S(3), cy + S(9), cx - r, cy + S(9), c);
  M5.Display.fillTriangle(cx + r, cy + S(4), cx + r + S(3), cy + S(9), cx + r, cy + S(9), c);
  M5.Display.fillRect(cx - r - S(3), cy + S(8), 2 * r + S(6) + 1, S(2) < 1 ? 1 : S(2), c);
  M5.Display.fillCircle(cx, cy + S(12), S(2) < 1 ? 1 : S(2), c);
  if (!enabled) {
    thick_line(b.x + S(8), b.y + S(8), b.x + s - S(8), b.y + s - S(8), S(3) + 1, p.bg);
    thick_line(b.x + S(8), b.y + S(8), b.x + s - S(8), b.y + s - S(8), S(2) < 1 ? 1 : S(2), c);
  }
}

void draw_skip(Box b, bool skip_next, const Palette& p) {
  fill_box(b, p.bg);
  const uint16_t c = skip_next ? p.ink : p.dim;
  const int s = b.w;
  auto S = [s](int v) { return v * s / 40; };
  const int cx = b.x + s / 2;
  const int cy = b.y + s / 2;
  M5.Display.fillTriangle(cx - S(11), cy - S(8), cx - S(11), cy + S(8), cx - S(1), cy, c);
  M5.Display.fillTriangle(cx - S(1), cy - S(8), cx - S(1), cy + S(8), cx + S(9), cy, c);
  M5.Display.fillRect(cx + S(9), cy - S(8), S(3) < 1 ? 1 : S(3), S(16), c);
  if (!skip_next) {
    thick_line(b.x + S(8), b.y + S(8), b.x + s - S(8), b.y + s - S(8), S(3) + 1, p.bg);
    thick_line(b.x + S(8), b.y + S(8), b.x + s - S(8), b.y + s - S(8), S(2) < 1 ? 1 : S(2), c);
  }
}

void draw_gear(Box b, const Palette& p) {
  fill_box(b, p.bg);
  const int s = b.w;
  auto S = [s](int v) { return v * s / 40; };
  const int cx = b.x + s / 2;
  const int cy = b.y + s / 2;
  static const int dx[] = {0, 7, 10, 7, 0, -7, -10, -7};
  static const int dy[] = {-10, -7, 0, 7, 10, 7, 0, -7};
  for (int i = 0; i < 8; ++i) {
    thick_line(cx, cy, cx + S(dx[i]) * 13 / 10, cy + S(dy[i]) * 13 / 10,
               S(5) < 2 ? 2 : S(5), p.ink);
  }
  M5.Display.fillCircle(cx, cy, S(10), p.ink);
  M5.Display.fillCircle(cx, cy, S(4) < 1 ? 1 : S(4), p.bg);
}

void draw_lamp_icon(Box b, bool lit, const Palette& p) {
  fill_box(b, p.bg);
  const uint16_t c = lit ? p.ink : p.dim;
  const int s = b.w;
  auto S = [s](int v) { return v * s / 40; };
  const int cx = b.x + s / 2;
  const int cy = b.y + s / 2;
  if (lit) {
    M5.Display.fillCircle(cx, cy - S(4), S(9), c);
  } else {
    M5.Display.drawCircle(cx, cy - S(4), S(9), c);
    M5.Display.drawCircle(cx, cy - S(4), S(8), c);
  }
  M5.Display.fillRect(cx - S(5), cy + S(6), S(10), S(3) < 1 ? 1 : S(3), c);
  M5.Display.fillRect(cx - S(4), cy + S(10), S(8), S(3) < 1 ? 1 : S(3), c);
}

void draw_sound_icon(Box b, bool on, const Palette& p) {
  fill_box(b, p.bg);
  const uint16_t c = on ? p.ink : p.dim;
  const int s = b.w;
  auto S = [s](int v) { return v * s / 40; };
  const int cx = b.x + s / 2;
  const int cy = b.y + s / 2;
  M5.Display.fillRect(cx - S(12), cy - S(4), S(6), S(8), c);
  M5.Display.fillTriangle(cx - S(7), cy - S(4), cx + S(1), cy - S(11), cx + S(1), cy + S(11), c);
  M5.Display.fillTriangle(cx - S(7), cy - S(4), cx + S(1), cy + S(11), cx - S(7), cy + S(4), c);
  const int t = S(2) < 1 ? 1 : S(2);
  if (on) {
    thick_line(cx + S(5), cy - S(5), cx + S(7), cy, t, c);
    thick_line(cx + S(7), cy, cx + S(5), cy + S(5), t, c);
    thick_line(cx + S(10), cy - S(9), cx + S(13), cy, t, c);
    thick_line(cx + S(13), cy, cx + S(10), cy + S(9), t, c);
  } else {
    thick_line(cx + S(5), cy - S(5), cx + S(13), cy + S(5), t, c);
    thick_line(cx + S(5), cy + S(5), cx + S(13), cy - S(5), t, c);
  }
}

void draw_ring_buttons(const Ui& ui, bool ringing) {
  const Palette& p = palette();
  const Box snooze = ring_box(ui, false);
  const Box stop = ring_box(ui, true);
  fill_box(snooze, p.bg);
  fill_box(stop, p.bg);
  if (!ringing) {
    return;
  }
  fill_round(snooze, ui.radius, p.card);
  stroke_round(snooze, ui.radius, p.ink);
  draw_text(inset(snooze, ui.gap), "Snooze", Align::Center, p.ink, true,
            snooze.h * 3 / 5);
  fill_round(stop, ui.radius, p.ink);
  draw_text(inset(stop, ui.gap), "Stop", Align::Center, p.bg, true, stop.h * 3 / 5);
}

void clock_view(const m5::rtc_datetime_t& dt, ClockView* v) {
  std::memset(v, 0, sizeof(*v));
  const TimeFormat face = static_cast<TimeFormat>(
      app.night ? app.cfg.night_format : app.cfg.day_format);
  format_clock_face(v->digits, sizeof(v->digits), v->period, sizeof(v->period),
                    face, dt.time.hours, dt.time.minutes, dt.time.seconds,
                    hours());
  v->enabled = app.cfg.alarm_enabled != 0;
  v->skip = app.cfg.skip_next != 0;
  v->lamp = lamp_lit();
  v->sound = app.noise_on;
  v->ringing = app.occurrence == Occurrence::Ringing;
  char alarm_part[40] = "";
  char when[16];
  if (app.snooze_armed && v->enabled) {
    format_hm(when, sizeof(when), app.snooze_at, hours());
    std::snprintf(alarm_part, sizeof(alarm_part), "Snoozed until %s", when);
  } else if (v->enabled) {
    format_hm(when, sizeof(when), Hm{app.cfg.alarm_hour, app.cfg.alarm_minute},
              hours());
    std::snprintf(alarm_part, sizeof(alarm_part), v->skip ? "Skipping %s" : "Alarm %s",
                  when);
  }
  char sound_part[40] = "";
  if (app.noise_on) {
    const int left = noise_minutes_left(noise_elapsed(), app.cfg.noise_timer);
    if (left > 0) {
      std::snprintf(sound_part, sizeof(sound_part), "%s, %d min",
                    noise_label(static_cast<NoiseKind>(app.cfg.noise_kind)), left);
    } else {
      std::snprintf(sound_part, sizeof(sound_part), "%s",
                    noise_label(static_cast<NoiseKind>(app.cfg.noise_kind)));
    }
  }
  if (alarm_part[0] && sound_part[0]) {
    std::snprintf(v->status, sizeof(v->status), "%s   %s", alarm_part, sound_part);
  } else {
    std::snprintf(v->status, sizeof(v->status), "%s%s", alarm_part, sound_part);
  }
}

// Fits the digits (7-segment fonts) and the AM/PM mark into the digits box.
void draw_digits(const Ui& ui, const ClockView& v, bool force) {
  const Palette& p = palette();
  const Box area = clock_digits_box(ui, v.ringing);
  const bool changed = force || std::strcmp(v.digits, app.last_clock.digits) != 0 ||
                       std::strcmp(v.period, app.last_clock.period) != 0 ||
                       v.ringing != app.last_clock.ringing;
  if (!changed) {
    return;
  }
  if (v.digits[0] == '\0') {
    fill_box(app.last_digits, p.bg);
    app.last_digits = Box{0, 0, 0, 0};
    return;
  }
  // Period (AM/PM) size and room
  int pw = 0;
  int gap = 0;
  const int period_px = title_px(ui);
  char period[8];
  std::snprintf(period, sizeof(period), "%s", v.period);
  if (period[0]) {
    fit_text(period, sizeof(period), area.w / 4, period_px, true);
    pw = M5.Display.textWidth(period);
    gap = ui.gap * 2;
  }
  M5.Display.setTextSize(1);
  bool font8 = true;
  M5.Display.setFont(&fonts::Font8);
  int tw = M5.Display.textWidth(v.digits);
  int th = M5.Display.fontHeight();
  const int room_w = area.w - gap - pw;
  if (tw > room_w) {
    font8 = false;
    M5.Display.setFont(&fonts::Font7);
    tw = M5.Display.textWidth(v.digits);
    th = M5.Display.fontHeight();
  }
  float scale = 1.f;
  if (tw > 0 && th > 0) {
    const float sx = static_cast<float>(room_w) / static_cast<float>(tw);
    const float sy = static_cast<float>(area.h) / static_cast<float>(th);
    scale = sx < sy ? sx : sy;
    if (scale > 6.f) {
      scale = 6.f;
    }
    // Tenths, so the size doesn't flicker between near-equal values.
    scale = static_cast<float>(static_cast<int>(scale * 10.f)) / 10.f;
    if (scale < 0.3f) {
      scale = 0.3f;
    }
  }
  const int dw = static_cast<int>(tw * scale);
  const int dh = static_cast<int>(th * scale);
  const int total_w = dw + gap + pw;
  const int cx = area.x + area.w / 2;
  const int cy = area.y + area.h / 2;
  // Padded, so a pixel of rounding in the scaled font's width is still cleared.
  const Box now = clamp_box(ui, Box{cx - total_w / 2 - 2, cy - dh / 2 - 1, total_w + 4, dh + 2});
  const bool moved = now.x != app.last_digits.x || now.y != app.last_digits.y ||
                     now.w != app.last_digits.w || now.h != app.last_digits.h;
  if (moved || force) {
    fill_box(app.last_digits, p.bg);
  }
  M5.Display.setTextColor(p.ink, p.bg);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(font8 ? &fonts::Font8 : &fonts::Font7);
  M5.Display.setTextSize(scale);
  const int left = cx - total_w / 2;
  M5.Display.drawString(v.digits, left + dw / 2, cy);
  M5.Display.setTextSize(1);
  if (period[0]) {
    fit_text(period, sizeof(period), area.w / 4, period_px, true);
    M5.Display.setTextColor(p.ink, p.bg);
    M5.Display.setTextDatum(middle_left);
    M5.Display.drawString(period, left + dw + gap, cy);
  }
  app.last_digits = now;
}

void draw_clock(const Ui& ui, const m5::rtc_datetime_t& dt) {
  const Palette& p = palette();
  ClockView v;
  clock_view(dt, &v);
  const bool all = !app.clock_drawn;
  if (all) {
    app.last_digits = Box{0, 0, 0, 0};
  }
  M5.Display.startWrite();
  if (all || v.ringing != app.last_clock.ringing) {
    // The digits' box changes size when the ring buttons come and go.
    fill_box(app.last_digits, p.bg);
    app.last_digits = Box{0, 0, 0, 0};
    fill_box(clock_status_box(ui), p.bg);
    draw_ring_buttons(ui, v.ringing);
  }
  draw_digits(ui, v, all || v.ringing != app.last_clock.ringing);
  if (all) {
    draw_gear(clock_icon_box(ui, ClockHit::Gear), p);
  }
  if (all || v.enabled != app.last_clock.enabled) {
    draw_bell(clock_icon_box(ui, ClockHit::Bell), v.enabled, p);
  }
  if (all || v.skip != app.last_clock.skip) {
    draw_skip(clock_icon_box(ui, ClockHit::Skip), v.skip, p);
  }
  if (all || v.lamp != app.last_clock.lamp) {
    draw_lamp_icon(clock_icon_box(ui, ClockHit::Lamp), v.lamp, p);
  }
  if (all || v.sound != app.last_clock.sound) {
    draw_sound_icon(clock_icon_box(ui, ClockHit::Sound), v.sound, p);
  }
  if (!v.ringing && (all || app.last_clock.ringing ||
                     std::strcmp(v.status, app.last_clock.status) != 0)) {
    const Box st = clock_status_box(ui);
    fill_box(st, p.bg);
    draw_text(st, v.status, Align::Center, p.dim, false, small_px(ui));
  }
  M5.Display.endWrite();
  app.last_clock = v;
  app.clock_drawn = true;
}

// ---------------------------------------------------------------------------
// Sound

void drive_speaker(Intensity heard, int loud_percent, int soft_percent) {
  if (heard != app.heard) {
    M5.Speaker.stop(kAlarmChannel);
    app.heard = heard;
    app.next_beep_at = millis();  // beep now; not 0, which reads as the future past 2^31 ms
    app.loud_high = false;
  }
  if (heard == Intensity::Silent) {
    return;
  }
  const uint32_t now = millis();
  if (static_cast<int32_t>(now - app.next_beep_at) < 0) {
    return;
  }
  if (heard == Intensity::Gentle) {
    M5.Speaker.setChannelVolume(kAlarmChannel, speaker_level(soft_percent));
    M5.Speaker.tone(440, 280, kAlarmChannel);
    app.next_beep_at = now + 700;
  } else {
    M5.Speaker.setChannelVolume(kAlarmChannel, speaker_level(loud_percent));
    const int freq = app.loud_high ? 1760 : 880;
    app.loud_high = !app.loud_high;
    M5.Speaker.tone(freq, 90, kAlarmChannel);
    app.next_beep_at = now + 140;
  }
}

void start_noise() {
  if (app.noise_on) {
    return;
  }
  app.noise_on = true;
  app.noise_start = millis();
  app.noise_gain = 0;
  app.noise_volume_applied = -1;
  app.gen = noise_gen(0x5EEDu + app.noise_start);
}

void stop_noise() {
  if (!app.noise_on) {
    return;
  }
  app.noise_on = false;
  M5.Speaker.stop(kNoiseChannel);
}

// Keeps two buffers of noise queued on the speaker.
void pump_noise() {
  if (!app.noise_on) {
    return;
  }
  const uint32_t elapsed = noise_elapsed();
  if (noise_timer_done(elapsed, app.cfg.noise_timer)) {
    stop_noise();
    return;
  }
  if (app.noise_volume_applied != app.cfg.noise_volume) {
    M5.Speaker.setChannelVolume(kNoiseChannel, speaker_level(app.cfg.noise_volume));
    app.noise_volume_applied = app.cfg.noise_volume;
  }
  const uint32_t chunk_ms = static_cast<uint32_t>(kNoiseChunk * 1000 / kNoiseRate);
  for (int guard = 0; guard < 2 && M5.Speaker.isPlaying(kNoiseChannel) < 2; ++guard) {
    const int to = noise_gain_permille(elapsed + chunk_ms, app.cfg.noise_timer);
    int16_t* buf = noise_bufs[app.noise_buf];
    fill_noise(app.gen, static_cast<NoiseKind>(app.cfg.noise_kind), buf,
               kNoiseChunk, app.noise_gain, to);
    app.noise_gain = to;
    M5.Speaker.playRaw(buf, kNoiseChunk, kNoiseRate, false, 1, kNoiseChannel, false);
    app.noise_buf = (app.noise_buf + 1) % kNoiseBuffers;
  }
}

// ---------------------------------------------------------------------------
// Lamp

void lamp_off() {
  app.cfg.lamp_on = 0;
  app.wake_light = false;
  app.sunrise_dismissed = true;
}

void lamp_toggle() {
  if (button_action(false, lamp_lit()) == ButtonAction::LampOff) {
    lamp_off();
  } else {
    app.cfg.lamp_on = 1;
  }
}

void update_lamp(const m5::rtc_datetime_t& dt) {
  const Hm now = {dt.time.hours, dt.time.minutes};
  const Alarm alarm = alarm_of(app.cfg);
  int sunrise = sunrise_permille(now, dt.time.seconds, alarm, app.cfg.sunrise_minutes);
  if (sunrise == 0) {
    app.sunrise_dismissed = false;
  }
  if (app.sunrise_dismissed || app.occurrence == Occurrence::Skipped) {
    sunrise = 0;
  }
  if (app.wake_light && wake_light_expired(now, alarm.at)) {
    app.wake_light = false;
  }
  app.lamp_level = lamp_permille(app.cfg.lamp_on != 0, app.cfg.lamp_bright,
                                 sunrise, app.wake_light);
  const int duty = lamp_duty(app.lamp_level);
  if (duty != app.lamp_duty_applied) {
    analogWrite(kLampPin, duty);
    app.lamp_duty_applied = duty;
  }
}

// ---------------------------------------------------------------------------
// Navigation and touch

void go(Screen s) {
  app.screen = s;
  app.enter = true;
  app.press = Hit{Part::None, -1};  // a finger still down from the last screen isn't a press here
}

void set_clock_time(int h, int m, int s) {
  m5::rtc_datetime_t dt;
  M5.Rtc.getDateTime(&dt);
  dt.time.hours = static_cast<int8_t>(h);
  dt.time.minutes = static_cast<int8_t>(m);
  dt.time.seconds = static_cast<int8_t>(s);
  M5.Rtc.setDateTime(dt);
}

void do_button() {
  switch (button_action(app.occurrence == Occurrence::Ringing, lamp_lit())) {
    case ButtonAction::Snooze:
      break;  // handled with the alarm step
    case ButtonAction::LampOff:
    case ButtonAction::LampOn:
      lamp_toggle();
      break;
  }
}

void on_row(Screen s, int row, Part part, Intensity* preview) {
  Settings& c = app.cfg;
  const int dir = part == Part::Minus ? -1 : 1;
  switch (s) {
    case Screen::Alarm:
      if (row == 0) {
        c.alarm_enabled = c.alarm_enabled ? 0 : 1;
      } else if (row == 1) {
        c.alarm_hour = static_cast<uint8_t>((c.alarm_hour + 24 + dir) % 24);
      } else if (row == 2) {
        c.alarm_minute = static_cast<uint8_t>((c.alarm_minute + 60 + dir) % 60);
      } else if (row == 3) {
        c.skip_next = c.skip_next ? 0 : 1;
      }
      break;
    case Screen::AlarmSound:
      if (row == 0) {
        c.volume = static_cast<uint8_t>(adjust_volume(c.volume, 5 * dir));
        c.soft_volume = static_cast<uint8_t>(clamp_soft(c.soft_volume, c.volume));
      } else if (row == 1) {
        c.soft_volume = static_cast<uint8_t>(
            clamp_soft(adjust_volume(c.soft_volume, 5 * dir), c.volume));
      } else if (row == 2) {
        c.gentle_seconds = static_cast<uint8_t>(adjust_gentle(c.gentle_seconds, 5 * dir));
      } else if (row == 3) {
        *preview = part == Part::Left ? Intensity::Gentle : Intensity::Loud;
      }
      break;
    case Screen::SleepSounds:
      if (row == 0) {
        c.noise_kind = static_cast<uint8_t>(next_noise(static_cast<NoiseKind>(c.noise_kind)));
      } else if (row == 1) {
        c.noise_volume = static_cast<uint8_t>(adjust_level(c.noise_volume, 5 * dir));
      } else if (row == 2) {
        c.noise_timer = static_cast<uint8_t>(step_timer(c.noise_timer, dir));
        if (app.noise_on) {
          app.noise_start = millis() - kNoiseFadeInMs;  // the timer restarts from now
        }
      } else if (row == 3) {
        if (app.noise_on) {
          stop_noise();
        } else {
          start_noise();
        }
      }
      break;
    case Screen::Lamp:
      if (row == 0) {
        lamp_toggle();
      } else if (row == 1) {
        c.lamp_bright = static_cast<uint8_t>(adjust_level(c.lamp_bright, 5 * dir));
      } else if (row == 2) {
        c.sunrise_minutes = static_cast<uint8_t>(step_sunrise(c.sunrise_minutes, dir));
      }
      break;
    case Screen::DayScreen:
      if (row == 0) {
        c.day_bright = static_cast<uint8_t>(adjust_brightness(c.day_bright, 10 * dir));
      } else if (row == 1) {
        c.day_format = static_cast<uint8_t>(next_format(static_cast<TimeFormat>(c.day_format)));
      } else if (row == 2) {
        c.hour12 = c.hour12 ? 0 : 1;
      }
      break;
    case Screen::NightScreen:
      if (row == 0) {
        c.night_bright = static_cast<uint8_t>(adjust_brightness(c.night_bright, 10 * dir));
      } else if (row == 1) {
        c.night_format = static_cast<uint8_t>(next_format(static_cast<TimeFormat>(c.night_format)));
      } else if (row == 2) {
        go(Screen::NightHours);
      }
      break;
    case Screen::NightHours:
      if (row == 0) {
        c.color_mode = static_cast<uint8_t>(next_color_mode(static_cast<ColorMode>(c.color_mode)));
      } else if (row == 1) {
        const Hm t = step_minutes(night_start(), 30 * dir);
        c.night_start_hour = static_cast<uint8_t>(t.hour);
        c.night_start_minute = static_cast<uint8_t>(t.minute);
      } else if (row == 2) {
        const Hm t = step_minutes(night_end(), 30 * dir);
        c.night_end_hour = static_cast<uint8_t>(t.hour);
        c.night_end_minute = static_cast<uint8_t>(t.minute);
      }
      break;
    case Screen::Clock:
    case Screen::Settings:
      break;
  }
}

void handle_touch(const Ui& ui, bool* stop, bool* snooze, Intensity* preview) {
  if (!M5.Touch.getCount()) {
    return;
  }
  const auto t = M5.Touch.getDetail();
  const bool edge = t.wasPressed();
  if (edge || t.isPressed()) {
    app.last_touch_ms = millis();
  }
  if (app.screen == Screen::Clock) {
    if (!edge) {
      return;
    }
    const RingHit action = app.occurrence == Occurrence::Ringing
                               ? ring_hit(ui, t.x, t.y)
                               : RingHit::None;
    if (action == RingHit::Stop) {
      *stop = true;
    } else if (action == RingHit::Snooze) {
      *snooze = true;
    } else {
      switch (clock_hit(ui, t.x, t.y)) {
        case ClockHit::Gear:
          if (app.occurrence != Occurrence::Ringing) {
            go(Screen::Settings);  // not while ringing: Snooze and Stop stay on screen
          }
          break;
        case ClockHit::Bell:
          app.cfg.alarm_enabled = app.cfg.alarm_enabled ? 0 : 1;
          break;
        case ClockHit::Skip:
          app.cfg.skip_next = app.cfg.skip_next ? 0 : 1;
          break;
        case ClockHit::Lamp:
          lamp_toggle();
          break;
        case ClockHit::Sound:
          if (app.noise_on) {
            stop_noise();
          } else {
            start_noise();
          }
          break;
        case ClockHit::Face:
          break;
      }
    }
    return;
  }
  if (app.screen == Screen::Settings) {
    if (!edge) {
      return;
    }
    if (in_box(header_back_box(ui), t.x, t.y)) {
      go(Screen::Clock);
      return;
    }
    const int i = tile_hit(ui, kMenuItems, t.x, t.y);
    if (i >= 0) {
      go(menu_item(i));
    }
    return;
  }
  RowKind kinds[kMaxRows];
  const int n = page_rows(app.screen, kinds);
  const Hit hit = page_hit(ui, kinds, n, t.x, t.y);
  if (edge) {
    app.press = hit;
  }
  const bool stepper = hit.part == Part::Minus || hit.part == Part::Plus;
  const bool same = hit.part == app.press.part && hit.row == app.press.row;
  if (stepper && same &&
      (edge || (t.isPressed() &&
                static_cast<int32_t>(millis() - app.next_repeat) >= 0))) {
    on_row(app.screen, hit.row, hit.part, preview);
    app.next_repeat = millis() + (edge ? kRepeatDelayMs : kRepeatMs);
  } else if (edge) {
    if (hit.part == Part::Back) {
      go(parent_screen(app.screen));
    } else if (hit.part != Part::None) {
      on_row(app.screen, hit.row, hit.part, preview);
    }
  }
}

// ---------------------------------------------------------------------------
// Serial commands

void run_command(const Command& c);

void read_serial() {
  int budget = 256;
  while (budget-- > 0 && Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch < 0) {
      break;
    }
    if (line_push(app.line, static_cast<char>(ch))) {
      run_command(parse_command(app.line.buf));
    }
  }
}

}  // namespace

// The clock's whole state as one line of JSON, for the serial "state" command
// and the simulator.
int write_state_json(char* out, size_t n) {
  m5::rtc_datetime_t dt;
  M5.Rtc.getDateTime(&dt);
  const Settings& c = app.cfg;
  char snooze[12] = "null";
  if (app.snooze_armed) {
    std::snprintf(snooze, sizeof(snooze), "\"%02d:%02d\"", app.snooze_at.hour,
                  app.snooze_at.minute);
  }
  const int w = std::snprintf(
      out, n,
      "{\"time\":\"%02d:%02d:%02d\",\"screen\":\"%s\",\"night\":%s,"
      "\"brightness\":%d,"
      "\"alarm\":{\"at\":\"%02d:%02d\",\"enabled\":%s,\"skip_next\":%s,"
      "\"state\":\"%s\",\"snooze\":%s},"
      "\"lamp\":{\"on\":%s,\"brightness\":%d,\"level\":%d,\"duty\":%d,"
      "\"sunrise\":%d,\"wake\":%s},"
      "\"sound\":{\"playing\":%s,\"kind\":\"%s\",\"volume\":%d,\"timer\":%d,"
      "\"minutes_left\":%d},"
      "\"night_hours\":{\"mode\":\"%s\",\"start\":\"%02d:%02d\",\"end\":\"%02d:%02d\"}}",
      dt.time.hours, dt.time.minutes, dt.time.seconds, screen_key(app.screen),
      app.night ? "true" : "false", app.bright_percent, c.alarm_hour,
      c.alarm_minute, c.alarm_enabled ? "true" : "false",
      c.skip_next ? "true" : "false", occurrence_name(app.occurrence), snooze,
      c.lamp_on ? "true" : "false", c.lamp_bright,
      app.lamp_level < 0 ? 0 : app.lamp_level, app.lamp_duty_applied < 0 ? 0 : app.lamp_duty_applied,
      c.sunrise_minutes, app.wake_light ? "true" : "false",
      app.noise_on ? "true" : "false",
      noise_key(static_cast<NoiseKind>(c.noise_kind)), c.noise_volume,
      c.noise_timer,
      app.noise_on ? noise_minutes_left(noise_elapsed(), c.noise_timer) : 0,
      c.color_mode == 1 ? "day" : (c.color_mode == 2 ? "night" : "scheduled"),
      c.night_start_hour, c.night_start_minute, c.night_end_hour,
      c.night_end_minute);
  return w;
}

namespace {

void run_command(const Command& c) {
  Settings& s = app.cfg;
  switch (c.kind) {
    case CmdKind::None:
      return;
    case CmdKind::Unknown:
      Serial.printf("? unknown command; try: help\n");
      return;
    case CmdKind::Help:
      Serial.printf(
          "lamp on|off|toggle|1-100  sound on|off|toggle|white|pink|brown  "
          "time HH:MM[:SS]  alarm HH:MM|on|off  go day|night|alarm|sunrise  "
          "button  state\n");
      return;
    case CmdKind::State: {
      char buf[640];
      write_state_json(buf, sizeof(buf));
      Serial.printf("%s\n", buf);
      return;
    }
    case CmdKind::LampOn:
      s.lamp_on = 1;
      break;
    case CmdKind::LampOff:
      lamp_off();
      break;
    case CmdKind::LampToggle:
      lamp_toggle();
      break;
    case CmdKind::LampLevel:
      s.lamp_bright = static_cast<uint8_t>(clamp_int(c.a, 5, 100));
      s.lamp_on = 1;
      break;
    case CmdKind::SoundOn:
      start_noise();
      break;
    case CmdKind::SoundOff:
      stop_noise();
      break;
    case CmdKind::SoundToggle:
      if (app.noise_on) {
        stop_noise();
      } else {
        start_noise();
      }
      break;
    case CmdKind::SoundKind:
      s.noise_kind = static_cast<uint8_t>(clamp_int(c.a, 0, 2));
      start_noise();
      break;
    case CmdKind::Time:
      set_clock_time(c.a, c.b, c.c);
      break;
    case CmdKind::AlarmAt:
      s.alarm_hour = static_cast<uint8_t>(c.a);
      s.alarm_minute = static_cast<uint8_t>(c.b);
      s.alarm_enabled = 1;
      s.skip_next = 0;
      break;
    case CmdKind::AlarmOn:
      s.alarm_enabled = 1;
      break;
    case CmdKind::AlarmOff:
      s.alarm_enabled = 0;
      break;
    case CmdKind::GoDay:
      set_clock_time(12, 0, 0);
      break;
    case CmdKind::GoNight: {
      const int span = minutes_until(night_start(), night_end());
      const Hm t = step_minutes(night_start(), span / 2);
      if (s.color_mode == static_cast<uint8_t>(ColorMode::Day)) {
        s.color_mode = static_cast<uint8_t>(ColorMode::Auto);
      }
      set_clock_time(t.hour, t.minute, 0);
      break;
    }
    case CmdKind::GoAlarm: {
      s.alarm_enabled = 1;
      s.skip_next = 0;
      const Hm t = step_minutes(Hm{s.alarm_hour, s.alarm_minute}, -1);
      set_clock_time(t.hour, t.minute, 50);
      break;
    }
    case CmdKind::GoSunrise: {
      s.alarm_enabled = 1;
      s.skip_next = 0;
      if (s.sunrise_minutes == 0) {
        s.sunrise_minutes = 10;
      }
      const Hm t = step_minutes(Hm{s.alarm_hour, s.alarm_minute}, -s.sunrise_minutes);
      set_clock_time(t.hour, t.minute, 0);
      break;
    }
    case CmdKind::Button:
      do_button();
      if (app.occurrence == Occurrence::Ringing) {
        app.pending_snooze = true;
      }
      break;
  }
  Serial.printf("ok\n");
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);

  app = App();
  app.cfg = default_settings();
  app.screen = Screen::Clock;
  app.enter = true;
  app.occurrence = Occurrence::Armed;
  app.heard = Intensity::Silent;
  app.lamp_duty_applied = -1;
  app.noise_volume_applied = -1;
  app.applied_light = -1;
  app.last_logged_second = -1;
  app.press = Hit{Part::None, -1};

  // app.stored is what's in flash; the first loop writes app.cfg if they
  // differ (first boot, or settings upgraded from version 1).
  {
    Preferences prefs;
    if (prefs.begin("alarmclk", true)) {
      uint8_t raw[sizeof(Settings)] = {};
      const size_t n = prefs.getBytes("cfg", raw, sizeof(raw));
      prefs.end();
      Settings loaded;
      if (load_settings(raw, n, &loaded)) {
        app.cfg = loaded;
        if (n == sizeof(Settings)) {
          app.stored = loaded;
        }
      }
    }
  }

  m5::rtc_datetime_t dt;
  if (!M5.Rtc.getDateTime(&dt) || rtc_needs_set(dt.date.year)) {
    YmdHms stamp = parse_compile_stamp(__DATE__, __TIME__);
    M5.Rtc.setDateTime(
        {{static_cast<int16_t>(stamp.year), static_cast<int8_t>(stamp.month),
          static_cast<int8_t>(stamp.date), -1},
         {static_cast<int8_t>(stamp.hour), static_cast<int8_t>(stamp.minute),
          static_cast<int8_t>(stamp.second)}});
  }

  if (M5.Speaker.isEnabled()) {
    M5.Speaker.setVolume(255);
  }
  analogWrite(kLampPin, 0);
  app.lamp_duty_applied = 0;
}

void loop() {
  M5.update();
  read_serial();

  m5::rtc_datetime_t dt;
  M5.Rtc.getDateTime(&dt);
  const Ui ui = screen_ui();

  bool stop_alarm = false;
  bool snooze_press = app.pending_snooze;
  app.pending_snooze = false;
  Intensity request_preview = Intensity::Silent;

  if (M5.BtnPWR.wasClicked()) {
    if (app.occurrence == Occurrence::Ringing) {
      snooze_press = true;
    } else {
      do_button();
    }
  }
  handle_touch(ui, &stop_alarm, &snooze_press, &request_preview);

  // Alarm
  const Occurrence before = app.occurrence;
  Inputs in = {{dt.time.hours, dt.time.minutes},
               dt.time.seconds,
               stop_alarm,
               app.cfg.gentle_seconds,
               snooze_press,
               app.snooze_armed,
               app.snooze_at};
  const Step step = step_alarm(app.occurrence, alarm_of(app.cfg), in);
  app.occurrence = step.occurrence;
  app.cfg.skip_next = step.skip_next ? 1 : 0;
  app.snooze_armed = step.snooze_armed;
  app.snooze_at = step.snooze_at;
  if (app.occurrence == Occurrence::Ringing && before != Occurrence::Ringing) {
    stop_noise();
    if (app.cfg.sunrise_minutes > 0 && !app.sunrise_dismissed) {
      app.wake_light = true;
    }
    if (app.screen != Screen::Clock) {
      go(Screen::Clock);
    }
  }

  if (app.screen != Screen::Clock && millis() - app.last_touch_ms > kIdleMs) {
    go(Screen::Clock);
  }

  // Sound
  if (request_preview != Intensity::Silent && step.intensity == Intensity::Silent) {
    app.preview_active = true;
    app.preview_kind = request_preview;
    app.preview_start = millis();
  }
  Intensity heard = step.intensity;
  if (step.intensity != Intensity::Silent) {
    app.preview_active = false;
  } else if (app.preview_active) {
    if (preview_playing(millis() - app.preview_start)) {
      heard = app.preview_kind;
    } else {
      app.preview_active = false;
    }
  }
  drive_speaker(heard, app.cfg.volume, app.cfg.soft_volume);
  pump_noise();

  update_lamp(dt);

  // Day or night look
  const Hm now_hm = {dt.time.hours, dt.time.minutes};
  const ColorMode mode = static_cast<ColorMode>(app.cfg.color_mode);
  app.night = mode == ColorMode::Night ||
              (mode == ColorMode::Auto && is_night(now_hm, night_start(), night_end()));
  const int preview = screen_preview(app.screen);
  const bool show_night = preview == 2 ? true : (preview == 1 ? false : app.night);
  app.bright_percent = show_night ? app.cfg.night_bright : app.cfg.day_bright;
  if (show_night != app.night_look) {
    app.night_look = show_night;
    app.enter = true;
  }
  const int light = backlight_level(app.bright_percent);
  if (light != app.applied_light) {
    M5.Display.setBrightness(static_cast<uint8_t>(light));
    app.applied_light = light;
  }

  // Draw
  if (app.enter) {
    M5.Display.fillScreen(palette().bg);
    app.clock_drawn = false;
    app.rows_drawn = false;
    app.enter = false;
  }
  if (app.screen == Screen::Clock) {
    draw_clock(ui, dt);
  } else if (app.screen == Screen::Settings) {
    draw_menu(ui);
  } else {
    draw_page(ui);
  }

  if (dt.time.seconds != app.last_logged_second) {
    app.last_logged_second = dt.time.seconds;
    Serial.printf(
        "%02d:%02d:%02d %s alarm=%02d:%02d %s skip=%d vol=%d svol=%d soft=%d "
        "tone=%s bright=%d lamp=%d sound=%s\n",
        dt.time.hours, dt.time.minutes, dt.time.seconds,
        occurrence_name(app.occurrence), app.cfg.alarm_hour, app.cfg.alarm_minute,
        app.cfg.alarm_enabled ? "on" : "off", app.cfg.skip_next, app.cfg.volume,
        app.cfg.soft_volume, app.cfg.gentle_seconds, app.night ? "night" : "day",
        app.bright_percent, app.lamp_level,
        app.noise_on ? noise_key(static_cast<NoiseKind>(app.cfg.noise_kind)) : "off");
  }

  if (std::memcmp(&app.cfg, &app.pending, sizeof(Settings)) != 0) {
    app.pending = app.cfg;
    app.changed_ms = millis();
  }
  const bool never_saved = app.stored.magic != kSettingsMagic;
  if (std::memcmp(&app.cfg, &app.stored, sizeof(Settings)) != 0 &&
      settings_valid(app.cfg) &&
      (never_saved || millis() - app.changed_ms >= kSaveDelayMs)) {
    Preferences prefs;
    if (prefs.begin("alarmclk", false)) {
      prefs.putBytes("cfg", &app.cfg, sizeof(Settings));
      prefs.end();
      app.stored = app.cfg;
    }
  }

  M5.delay(10);
}
