// Stand-in for M5Unified, just enough for src/main.cpp to run in a browser.
//
// Built to WebAssembly by sim/build.py. Every drawing, touch, sound, clock and
// storage call is forwarded to JavaScript (the js_* imports below), which draws
// on a canvas and feeds back touches and the time. Only the parts of the API
// the firmware uses are here; a new call in main.cpp that isn't covered fails
// the sim build, which is the signal to add it.
//
// The same header also builds natively (test/host/), where the js_* functions
// are C++ that records what the firmware did, for the firmware tests.

#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(__wasm__)
#define SIM_IMPORT(name) extern "C" __attribute__((import_module("env"), import_name(#name)))
#else
#define SIM_IMPORT(name) extern "C"
#endif

SIM_IMPORT(js_fill_rect) void js_fill_rect(int x, int y, int w, int h, int color);
SIM_IMPORT(js_draw_rect) void js_draw_rect(int x, int y, int w, int h, int color);
SIM_IMPORT(js_fill_round_rect) void js_fill_round_rect(int x, int y, int w, int h, int r, int color);
SIM_IMPORT(js_draw_round_rect) void js_draw_round_rect(int x, int y, int w, int h, int r, int color);
SIM_IMPORT(js_draw_line) void js_draw_line(int x0, int y0, int x1, int y1, int color);
SIM_IMPORT(js_draw_circle) void js_draw_circle(int x, int y, int r, int color);
SIM_IMPORT(js_fill_circle) void js_fill_circle(int x, int y, int r, int color);
SIM_IMPORT(js_fill_triangle) void js_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int color);
SIM_IMPORT(js_draw_string) void js_draw_string(const char* s, int len, int x, int y, int datum,
                                               int fg, int bg, int font, float size);
SIM_IMPORT(js_text_width) int js_text_width(const char* s, int len, int font, float size);
SIM_IMPORT(js_font_height) int js_font_height(int font, float size);
SIM_IMPORT(js_set_brightness) void js_set_brightness(int level);
SIM_IMPORT(js_tone) void js_tone(int freq, int ms, int volume, int channel);
SIM_IMPORT(js_stop_tone) void js_stop_tone(int channel);
SIM_IMPORT(js_play_raw) void js_play_raw(int channel, const int16_t* samples, int count, int rate, int volume);
SIM_IMPORT(js_channel_busy) int js_channel_busy(int channel);
SIM_IMPORT(js_millis) double js_millis();
SIM_IMPORT(js_get_time) void js_get_time(int* ymdhms);
SIM_IMPORT(js_set_time) void js_set_time(int y, int mo, int d, int h, int mi, int s);
SIM_IMPORT(js_log) void js_log(const char* s, int len);
SIM_IMPORT(js_pref_get) int js_pref_get(const char* key, int key_len, void* buf, int cap);
SIM_IMPORT(js_pref_set) void js_pref_set(const char* key, int key_len, const void* buf, int len);
SIM_IMPORT(js_analog_write) void js_analog_write(int pin, int value);

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
// Adafruit GFX FreeSans, which M5GFX includes. 2x = regular, 3x = bold;
// the second digit is the size: 9, 12, 18 and 24 pt (22, 29, 42 and 56 px lines).
static const lgfx::IFont FreeSans9pt7b = {20};
static const lgfx::IFont FreeSans12pt7b = {21};
static const lgfx::IFont FreeSans18pt7b = {22};
static const lgfx::IFont FreeSans24pt7b = {23};
static const lgfx::IFont FreeSansBold9pt7b = {30};
static const lgfx::IFont FreeSansBold12pt7b = {31};
static const lgfx::IFont FreeSansBold18pt7b = {32};
static const lgfx::IFont FreeSansBold24pt7b = {33};
}  // namespace fonts

inline uint32_t millis() { return static_cast<uint32_t>(js_millis()); }
inline void analogWrite(int pin, int value) { js_analog_write(pin, value); }

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
  void fillRoundRect(int x, int y, int w, int h, int r, uint32_t c) { js_fill_round_rect(x, y, w, h, r, static_cast<int>(c)); }
  void drawRoundRect(int x, int y, int w, int h, int r, uint32_t c) { js_draw_round_rect(x, y, w, h, r, static_cast<int>(c)); }
  void drawLine(int x0, int y0, int x1, int y1, uint32_t c) { js_draw_line(x0, y0, x1, y1, static_cast<int>(c)); }
  void drawCircle(int x, int y, int r, uint32_t c) { js_draw_circle(x, y, r, static_cast<int>(c)); }
  void fillCircle(int x, int y, int r, uint32_t c) { js_fill_circle(x, y, r, static_cast<int>(c)); }
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t c) {
    js_fill_triangle(x0, y0, x1, y1, x2, y2, static_cast<int>(c));
  }

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
  void reset() { detail_ = touch_detail_t(); }

 private:
  touch_detail_t detail_;
};

// A physical button: one click reported on one M5.update().
class SimButton {
 public:
  bool wasClicked() const { return clicked_; }
  void latch(bool clicked) { clicked_ = clicked; }

 private:
  bool clicked_ = false;
};

// Speaker with M5Unified's channels: a tone or queued raw buffers per channel,
// each with its own volume, all scaled by the master volume.
class SimSpeaker {
 public:
  bool isEnabled() const { return true; }
  void setVolume(uint8_t v) { master_ = v; }
  void setChannelVolume(uint8_t ch, uint8_t v) {
    if (ch < kChannels) channel_[ch] = v;
  }
  bool tone(float freq, uint32_t ms = UINT32_MAX, int channel = -1, bool = true) {
    const int ch = channel < 0 ? 0 : channel;
    js_tone(static_cast<int>(freq), static_cast<int>(ms > 600000 ? 600000 : ms), volume(ch), ch);
    return true;
  }
  bool playRaw(const int16_t* data, size_t count, uint32_t rate = 44100, bool = false,
               uint32_t = 1, int channel = -1, bool = false) {
    const int ch = channel < 0 ? 0 : channel;
    js_play_raw(ch, data, static_cast<int>(count), static_cast<int>(rate), volume(ch));
    return true;
  }
  // 0 = idle, 1 = playing, 2 = playing with one more queued (M5Unified's meaning).
  size_t isPlaying(uint8_t channel) const { return static_cast<size_t>(js_channel_busy(channel)); }
  void stop() { js_stop_tone(-1); }
  void stop(uint8_t channel) { js_stop_tone(channel); }

 private:
  static constexpr int kChannels = 8;
  int volume(int ch) const {
    const int cv = ch < kChannels ? channel_[ch] : 255;
    return master_ * cv / 255;
  }
  uint8_t master_ = 64;
  uint8_t channel_[kChannels] = {255, 255, 255, 255, 255, 255, 255, 255};
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
  SimButton BtnPWR;
};

}  // namespace m5

extern m5::M5Unified M5;

// Serial: output goes to js_log; input is what the simulator (or a test)
// typed, queued by sim_input().
class SimSerial {
 public:
  void begin(unsigned long) {}
  int printf(const char* fmt, ...) {
    char buf[768];
    va_list ap;
    va_start(ap, fmt);
    const int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    js_log(buf, n < 0 ? 0 : (n < static_cast<int>(sizeof buf) ? n : static_cast<int>(sizeof buf) - 1));
    return n;
  }
  int available() const { return static_cast<int>(tail_ - head_); }
  int read() { return head_ < tail_ ? static_cast<unsigned char>(rx_[head_++ % sizeof rx_]) : -1; }
  // Queues bytes for the firmware to read; drops what doesn't fit.
  void feed(const char* s, int n) {
    for (int i = 0; i < n && tail_ - head_ < sizeof rx_; ++i) rx_[tail_++ % sizeof rx_] = s[i];
  }
  void clear() { head_ = tail_ = 0; }

 private:
  char rx_[1024];
  unsigned head_ = 0;
  unsigned tail_ = 0;
};

extern SimSerial Serial;
