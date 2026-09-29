// Stand-in for M5Unified, just enough for src/main.cpp to run in a browser.
//
// Built to WebAssembly by sim/build.py. Every drawing, touch, sound, clock and
// storage call is forwarded to JavaScript (the js_* imports below), which draws
// on a canvas and feeds back touches and the time. Only the parts of the API
// the firmware uses are here; a new call in main.cpp that isn't covered fails
// the sim build, which is the signal to add it.

#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define SIM_IMPORT(name) extern "C" __attribute__((import_module("env"), import_name(#name)))

SIM_IMPORT(js_fill_rect) void js_fill_rect(int x, int y, int w, int h, int color);
SIM_IMPORT(js_draw_rect) void js_draw_rect(int x, int y, int w, int h, int color);
SIM_IMPORT(js_draw_line) void js_draw_line(int x0, int y0, int x1, int y1, int color);
SIM_IMPORT(js_draw_circle) void js_draw_circle(int x, int y, int r, int color);
SIM_IMPORT(js_fill_circle) void js_fill_circle(int x, int y, int r, int color);
SIM_IMPORT(js_draw_string) void js_draw_string(const char* s, int len, int x, int y, int datum,
                                               int fg, int bg, int font, float size);
SIM_IMPORT(js_text_width) int js_text_width(const char* s, int len, int font, float size);
SIM_IMPORT(js_font_height) int js_font_height(int font, float size);
SIM_IMPORT(js_set_brightness) void js_set_brightness(int level);
SIM_IMPORT(js_tone) void js_tone(int freq, int ms, int volume);
SIM_IMPORT(js_stop_tone) void js_stop_tone();
SIM_IMPORT(js_millis) double js_millis();
SIM_IMPORT(js_get_time) void js_get_time(int* ymdhms);
SIM_IMPORT(js_set_time) void js_set_time(int y, int mo, int d, int h, int mi, int s);
SIM_IMPORT(js_log) void js_log(const char* s, int len);
SIM_IMPORT(js_pref_get) int js_pref_get(const char* key, int key_len, void* buf, int cap);
SIM_IMPORT(js_pref_set) void js_pref_set(const char* key, int key_len, const void* buf, int len);

#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define TFT_RED 0xF800

// Text datums, same values as LovyanGFX/M5GFX.
enum textdatum_t : uint8_t {
  top_left = 0, top_center = 1, top_right = 2,
  middle_left = 4, middle_center = 5, middle_right = 6,
  bottom_left = 8, bottom_center = 9, bottom_right = 10,
  baseline_left = 16, baseline_center = 17, baseline_right = 18,
};

namespace lgfx {
struct IFont {
  int id;
};
}  // namespace lgfx

namespace fonts {
// TFT_eSPI-style built-in fonts: 2 = 16 px, 4 = 26 px, 7 = 48 px 7-segment, 8 = 75 px digits.
static const lgfx::IFont Font0 = {0};
static const lgfx::IFont Font2 = {2};
static const lgfx::IFont Font4 = {4};
static const lgfx::IFont Font6 = {6};
static const lgfx::IFont Font7 = {7};
static const lgfx::IFont Font8 = {8};
}  // namespace fonts

inline uint32_t millis() { return static_cast<uint32_t>(js_millis()); }

class SimDisplay {
 public:
  int width() const { return w_; }
  int height() const { return h_; }
  void setResolution(int w, int h) { w_ = w; h_ = h; }

  void startWrite() {}
  void endWrite() {}
  void setBrightness(uint8_t level) { js_set_brightness(level); }

  void fillScreen(uint32_t c) { js_fill_rect(0, 0, w_, h_, static_cast<int>(c)); }
  void fillRect(int x, int y, int w, int h, uint32_t c) { js_fill_rect(x, y, w, h, static_cast<int>(c)); }
  void drawRect(int x, int y, int w, int h, uint32_t c) { js_draw_rect(x, y, w, h, static_cast<int>(c)); }
  void drawLine(int x0, int y0, int x1, int y1, uint32_t c) { js_draw_line(x0, y0, x1, y1, static_cast<int>(c)); }
  void drawCircle(int x, int y, int r, uint32_t c) { js_draw_circle(x, y, r, static_cast<int>(c)); }
  void fillCircle(int x, int y, int r, uint32_t c) { js_fill_circle(x, y, r, static_cast<int>(c)); }

  void setTextColor(uint32_t fg) { fg_ = static_cast<int>(fg); bg_ = -1; }
  void setTextColor(uint32_t fg, uint32_t bg) { fg_ = static_cast<int>(fg); bg_ = static_cast<int>(bg); }
  void setTextDatum(textdatum_t d) { datum_ = d; }
  void setTextSize(float s) { size_ = s; }
  void setFont(const lgfx::IFont* f) { font_ = f ? f->id : 0; }

  size_t drawString(const char* s, int x, int y) {
    const int n = static_cast<int>(strlen(s));
    js_draw_string(s, n, x, y, datum_, fg_, bg_, font_, size_);
    return static_cast<size_t>(textWidth(s));
  }
  int32_t textWidth(const char* s) { return js_text_width(s, static_cast<int>(strlen(s)), font_, size_); }
  int32_t fontHeight() { return js_font_height(font_, size_); }

 private:
  int w_ = 320;
  int h_ = 240;
  int fg_ = TFT_WHITE;
  int bg_ = -1;
  int datum_ = top_left;
  int font_ = 0;
  float size_ = 1.f;
};

namespace m5 {

struct rtc_date_t {
  int16_t year;
  int8_t month;
  int8_t date;
  int8_t weekDay;
};

struct rtc_time_t {
  int8_t hours;
  int8_t minutes;
  int8_t seconds;
};

struct rtc_datetime_t {
  rtc_date_t date;
  rtc_time_t time;
};

struct touch_detail_t {
  int16_t x = 0;
  int16_t y = 0;
  bool pressed = false;
  bool pressed_edge = false;
  bool released_edge = false;
  bool isPressed() const { return pressed; }
  bool wasPressed() const { return pressed_edge; }
  bool wasReleased() const { return released_edge; }
  bool wasClicked() const { return released_edge; }
  bool isHolding() const { return pressed && !pressed_edge; }
};

class SimTouch {
 public:
  int getCount() const { return (detail_.pressed || detail_.released_edge) ? 1 : 0; }
  touch_detail_t getDetail(int = 0) const { return detail_; }
  // Called once per M5.update() with the latest pointer state from JavaScript.
  void latch(int x, int y, bool down) {
    const bool was = detail_.pressed;
    detail_.x = static_cast<int16_t>(x);
    detail_.y = static_cast<int16_t>(y);
    detail_.pressed = down;
    detail_.pressed_edge = down && !was;
    detail_.released_edge = !down && was;
  }

 private:
  touch_detail_t detail_;
};

class SimSpeaker {
 public:
  bool isEnabled() const { return true; }
  void setVolume(uint8_t v) { volume_ = v; }
  bool tone(float freq, uint32_t ms = UINT32_MAX) {
    js_tone(static_cast<int>(freq), static_cast<int>(ms > 600000 ? 600000 : ms), volume_);
    return true;
  }
  void stop() { js_stop_tone(); }

 private:
  uint8_t volume_ = 64;
};

class SimRtc {
 public:
  bool getDateTime(rtc_datetime_t* dt) const {
    int v[6];
    js_get_time(v);
    dt->date = {static_cast<int16_t>(v[0]), static_cast<int8_t>(v[1]), static_cast<int8_t>(v[2]), 0};
    dt->time = {static_cast<int8_t>(v[3]), static_cast<int8_t>(v[4]), static_cast<int8_t>(v[5])};
    return true;
  }
  void setDateTime(const rtc_datetime_t& dt) {
    js_set_time(dt.date.year, dt.date.month, dt.date.date, dt.time.hours, dt.time.minutes, dt.time.seconds);
  }
};

struct config_t {};

class M5Unified {
 public:
  config_t config() const { return {}; }
  void begin(const config_t&) {}
  void update();
  void delay(uint32_t) {}

  SimDisplay Display;
  SimTouch Touch;
  SimSpeaker Speaker;
  SimRtc Rtc;
};

}  // namespace m5

extern m5::M5Unified M5;

class SimSerial {
 public:
  void begin(unsigned long) {}
  int printf(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    const int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    js_log(buf, n < 0 ? 0 : (n < static_cast<int>(sizeof buf) ? n : static_cast<int>(sizeof buf) - 1));
    return n;
  }
};

extern SimSerial Serial;
