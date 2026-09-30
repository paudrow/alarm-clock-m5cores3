#include "host.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace host {

State st;

namespace {

// ---- font metrics: a little wider than the real fonts, so text that fits
// here fits on the clock.
int font_h(int font) {
  switch (font) {
    case 0: return 8;
    case 2: return 16;
    case 4: return 26;
    case 6: return 48;
    case 7: return 48;
    case 8: return 75;
    case 20: case 30: return 22;
    case 21: case 31: return 29;
    case 22: case 32: return 42;
    case 23: case 33: return 56;
  }
  return 8;
}

int char_w(int font, char c, float size) {
  if (font == 7) {
    return static_cast<int>(std::lround(((c == ':' || c == '.') ? 12 : 32) * size));
  }
  if (font == 8) {
    return static_cast<int>(std::lround(55 * size));
  }
  if (font >= 20) {
    // Close to Adafruit's FreeSans advances, rounded up.
    const int h = font_h(font);
    int pct = 47;
    if (std::strchr(" ijlI.,:;'!|", c)) pct = 26;
    else if (std::strchr("ftr()-/", c)) pct = 33;
    else if (std::strchr("mwMW%", c)) pct = 76;
    else if (c >= 'A' && c <= 'Z') pct = 60;
    if (font >= 30) pct = pct * 106 / 100;
    return static_cast<int>(std::ceil(h * pct / 100.0 * size));
  }
  const int w = font == 2 ? 9 : font == 4 ? 14 : 6;
  return static_cast<int>(std::lround(w * size));
}

int text_w(const char* s, int n, int font, float size) {
  int w = 0;
  for (int i = 0; i < n; ++i) {
    w += char_w(font, s[i], size);
  }
  return w;
}

int text_h(int font, float size) { return static_cast<int>(std::lround(font_h(font) * size)); }

// ---- checks
void check_box(const char* what, int x, int y, int w, int h) {
  ++st.draw_calls;
  if (w < 0 || h < 0 || x < -1 || y < -1 || x + w > st.w + 1 || y + h > st.h + 1) {
    char buf[160];
    std::snprintf(buf, sizeof buf, "%s(%d, %d, %d, %d) off a %dx%d screen", what, x, y, w, h,
                  st.w, st.h);
    if (st.bad.size() < 50) st.bad.push_back(buf);
  }
}

// ---- canvas
uint32_t rgb(int c565) {
  const int r = (c565 >> 11) & 31, g = (c565 >> 5) & 63, b = c565 & 31;
  return static_cast<uint32_t>((r * 255 / 31) << 16 | (g * 255 / 63) << 8 | (b * 255 / 31));
}

void put(int x, int y, uint32_t c) {
  if (x < 0 || y < 0 || x >= st.w || y >= st.h) return;
  st.px[static_cast<size_t>(y) * st.w + x] = c;
}

void fill(int x, int y, int w, int h, uint32_t c) {
  for (int j = std::max(0, y); j < std::min(st.h, y + h); ++j)
    for (int i = std::max(0, x); i < std::min(st.w, x + w); ++i) st.px[static_cast<size_t>(j) * st.w + i] = c;
}

bool in_round(int px, int py, int x, int y, int w, int h, int r) {
  if (px < x || py < y || px >= x + w || py >= y + h) return false;
  r = std::min(r, std::min(w, h) / 2);
  const int cx = std::min(std::max(px, x + r), x + w - 1 - r);
  const int cy = std::min(std::max(py, y + r), y + h - 1 - r);
  return (px - cx) * (px - cx) + (py - cy) * (py - cy) <= r * r;
}

void line(int x0, int y0, int x1, int y1, uint32_t c) {
  const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int guard = 0; guard < 100000; ++guard) {
    put(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    const int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// A 3x5 pixel font, enough to read the pictures.
int glyph(char ch, int row) {
  static const char* rows[] = {
      "111101101101111", "010110010010111", "111001111100111", "111001111001111",
      "101101111001001", "111100111001111", "111100111101111", "111001001001001",
      "111101111101111", "111101111001111", "010101111101101", "110101110101110",
      "111100100100111", "110101101101110", "111100111100111", "111100111100100",
      "111100101101111", "101101111101101", "111010010010111", "001001001101010",
      "101101110101101", "100100100100111", "101111101101101", "110101101101101",
      "111101101101111", "111101111100100", "111101101111001", "111101110101101",
      "111100111001111", "111010010010010", "101101101101111", "101101101101010",
      "101101101111101", "101101010101101", "101101010010010", "111001010100111",
      "000000000000000", "000010000010000", "000000111000000", "010010111010010",
      "000000000000010", "101001010100101", "001010010010001", "100010010010100",
      "001001010100100", "000000000010100"};
  int index = 36;
  if (ch >= '0' && ch <= '9') index = ch - '0';
  else if (ch >= 'A' && ch <= 'Z') index = 10 + (ch - 'A');
  else if (ch >= 'a' && ch <= 'z') index = 10 + (ch - 'a');
  else if (ch == ':') index = 37;
  else if (ch == '-') index = 38;
  else if (ch == '+') index = 39;
  else if (ch == '.') index = 40;
  else if (ch == '%') index = 41;
  else if (ch == '(') index = 42;
  else if (ch == ')') index = 43;
  else if (ch == '/') index = 44;
  else if (ch == ',') index = 45;
  const char* bits = rows[index];
  int v = 0;
  for (int col = 0; col < 3; ++col)
    if (bits[row * 3 + col] == '1') v |= 1 << (2 - col);
  return v;
}

void paint_text(const char* s, int n, int left, int top, int font, float size, uint32_t c) {
  const int h = text_h(font, size);
  const int k = std::max(1, h * 6 / 10 / 5);
  int x = left;
  for (int i = 0; i < n; ++i) {
    const int cw = char_w(font, s[i], size);
    const int gx = x + (cw - 3 * k) / 2;
    const int gy = top + (h - 5 * k) / 2;
    for (int row = 0; row < 5; ++row) {
      const int bits = glyph(s[i], row);
      for (int col = 0; col < 3; ++col)
        if (bits & (1 << (2 - col))) fill(gx + col * k, gy + row * k, k, k, c);
    }
    x += cw;
  }
}

std::string key_of(const char* k, int n) { return std::string(k, static_cast<size_t>(n)); }

void advance(int dt_ms) {
  st.ms += dt_ms;
  st.clock_frac += static_cast<double>(dt_ms) * st.speed;
  const int64_t whole = static_cast<int64_t>(st.clock_frac / 1000.0);
  st.clock_s += whole;
  st.clock_frac -= static_cast<double>(whole) * 1000.0;
}

bool touch_down = false;
int touch_x = 0, touch_y = 0;

}  // namespace

void boot(int w, int h, int year, int month, int day, int hour, int minute, int second) {
  st.w = w;
  st.h = h;
  st.ms = 0;
  st.px.assign(static_cast<size_t>(w) * h, 0);
  std::tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  st.clock_s = timegm(&t);
  st.clock_frac = 0;
  st.speed = 1;
  st.brightness = -1;
  st.pins.clear();
  st.tones.clear();
  st.raws.clear();
  for (int i = 0; i < 8; ++i) st.queue_end[i].clear();
  st.log.clear();
  st.bad.clear();
  st.texts.clear();
  touch_down = false;
  sim_init(w, h);
}

void frames(int n, int dt_ms) {
  for (int i = 0; i < n; ++i) {
    advance(dt_ms);
    sim_touch(touch_x, touch_y, touch_down ? 1 : 0);
    sim_loop();
  }
}

void touch(int x, int y, bool down) {
  touch_x = x;
  touch_y = y;
  touch_down = down;
}

void tap(int x, int y) {
  touch(x, y, true);
  frames(2);
  touch(x, y, false);
  frames(2);
}

void hold(int x, int y, int ms) {
  touch(x, y, true);
  frames(ms / 16 + 1);
  touch(x, y, false);
  frames(2);
}

void button() {
  sim_button();
  frames(1);
}

std::string command(const std::string& line) {
  const size_t before = st.log.size();
  std::snprintf(sim_io(), 1024, "%s", line.c_str());
  sim_input(static_cast<int>(std::min<size_t>(line.size(), 1023)));
  frames(1);
  std::string out;
  for (size_t i = before; i < st.log.size(); ++i) {
    const std::string& l = st.log[i];
    // Skip the once-a-second status line
    if (l.size() > 9 && l[2] == ':' && l[5] == ':' && l[8] == ' ') continue;
    out += l;
  }
  return out;
}

std::string state() {
  const int n = sim_state();
  return std::string(sim_io(), static_cast<size_t>(n));
}

void set_clock(int hour, int minute, int second) {
  std::tm t;
  const std::time_t now = static_cast<std::time_t>(st.clock_s);
  gmtime_r(&now, &t);
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  st.clock_s = timegm(&t);
  st.clock_frac = 0;
}

static std::tm now_tm() {
  std::tm t;
  const std::time_t now = static_cast<std::time_t>(st.clock_s);
  gmtime_r(&now, &t);
  return t;
}
int clock_hour() { return now_tm().tm_hour; }
int clock_minute() { return now_tm().tm_min; }
int clock_second() { return now_tm().tm_sec; }

void clear_frame_record() { st.texts.clear(); }

bool drew(const std::string& text) {
  for (const Text& t : st.texts)
    if (t.s == text) return true;
  return false;
}

bool drew_prefix(const std::string& prefix) {
  for (const Text& t : st.texts)
    if (t.s.compare(0, prefix.size(), prefix) == 0) return true;
  return false;
}

int lamp_duty() {
  auto it = st.pins.find(9);
  return it == st.pins.end() ? -1 : it->second;
}

std::string jget(const std::string& json, const std::string& a, const std::string& b) {
  size_t at = json.find("\"" + a + "\":");
  if (at == std::string::npos) return "";
  at += a.size() + 3;
  if (!b.empty()) {
    const size_t close = json.find('}', at);
    at = json.find("\"" + b + "\":", at);
    if (at == std::string::npos || at > close) return "";
    at += b.size() + 3;
  }
  if (json[at] == '"') {
    const size_t end = json.find('"', at + 1);
    return json.substr(at + 1, end - at - 1);
  }
  size_t end = at;
  while (end < json.size() && json[end] != ',' && json[end] != '}') ++end;
  return json.substr(at, end - at);
}

// ---- PNG (stored, uncompressed deflate)
namespace {
uint32_t crc_table[256];
void init_crc() {
  for (uint32_t n = 0; n < 256; ++n) {
    uint32_t c = n;
    for (int k = 0; k < 8; ++k) c = (c & 1u) ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
    crc_table[n] = c;
  }
}
uint32_t crc32(const uint8_t* d, size_t n) {
  uint32_t c = 0xffffffffu;
  for (size_t i = 0; i < n; ++i) c = crc_table[(c ^ d[i]) & 0xffu] ^ (c >> 8);
  return c ^ 0xffffffffu;
}
void be32(std::vector<uint8_t>& o, uint32_t v) {
  o.push_back(static_cast<uint8_t>(v >> 24));
  o.push_back(static_cast<uint8_t>(v >> 16));
  o.push_back(static_cast<uint8_t>(v >> 8));
  o.push_back(static_cast<uint8_t>(v));
}
void chunk(std::vector<uint8_t>& o, const char* type, const std::vector<uint8_t>& d) {
  be32(o, static_cast<uint32_t>(d.size()));
  const size_t start = o.size();
  o.insert(o.end(), type, type + 4);
  o.insert(o.end(), d.begin(), d.end());
  be32(o, crc32(&o[start], o.size() - start));
}
}  // namespace

bool write_png(const std::string& path) {
  init_crc();
  std::vector<uint8_t> raw;
  for (int y = 0; y < st.h; ++y) {
    raw.push_back(0);
    for (int x = 0; x < st.w; ++x) {
      const uint32_t c = st.px[static_cast<size_t>(y) * st.w + x];
      raw.push_back(static_cast<uint8_t>(c >> 16));
      raw.push_back(static_cast<uint8_t>(c >> 8));
      raw.push_back(static_cast<uint8_t>(c));
    }
  }
  std::vector<uint8_t> z = {0x78, 0x01};
  uint32_t s1 = 1, s2 = 0;
  size_t off = 0;
  while (off < raw.size()) {
    const size_t n = std::min<size_t>(65535, raw.size() - off);
    z.push_back(off + n == raw.size() ? 1 : 0);
    z.push_back(static_cast<uint8_t>(n & 0xff));
    z.push_back(static_cast<uint8_t>(n >> 8));
    z.push_back(static_cast<uint8_t>(~n & 0xff));
    z.push_back(static_cast<uint8_t>((~n >> 8) & 0xff));
    z.insert(z.end(), raw.begin() + static_cast<std::ptrdiff_t>(off),
             raw.begin() + static_cast<std::ptrdiff_t>(off + n));
    for (size_t i = 0; i < n; ++i) {
      s1 = (s1 + raw[off + i]) % 65521;
      s2 = (s2 + s1) % 65521;
    }
    off += n;
  }
  be32(z, (s2 << 16) | s1);
  std::vector<uint8_t> png = {137, 80, 78, 71, 13, 10, 26, 10};
  std::vector<uint8_t> ihdr;
  be32(ihdr, static_cast<uint32_t>(st.w));
  be32(ihdr, static_cast<uint32_t>(st.h));
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
  chunk(png, "IHDR", ihdr);
  chunk(png, "IDAT", z);
  chunk(png, "IEND", {});
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  std::fwrite(png.data(), 1, png.size(), f);
  std::fclose(f);
  return true;
}

}  // namespace host

using host::st;

// ---- what the shim calls
extern "C" {

void js_fill_rect(int x, int y, int w, int h, int c) {
  host::check_box("fillRect", x, y, w, h);
  if (x <= 0 && y <= 0 && x + w >= st.w && y + h >= st.h) st.texts.clear();  // a new screen
  host::fill(x, y, w, h, host::rgb(c));
}

void js_draw_rect(int x, int y, int w, int h, int c) {
  host::check_box("drawRect", x, y, w, h);
  const uint32_t k = host::rgb(c);
  host::fill(x, y, w, 1, k);
  host::fill(x, y + h - 1, w, 1, k);
  host::fill(x, y, 1, h, k);
  host::fill(x + w - 1, y, 1, h, k);
}

void js_fill_round_rect(int x, int y, int w, int h, int r, int c) {
  host::check_box("fillRoundRect", x, y, w, h);
  const uint32_t k = host::rgb(c);
  for (int j = y; j < y + h; ++j)
    for (int i = x; i < x + w; ++i)
      if (host::in_round(i, j, x, y, w, h, r)) host::put(i, j, k);
}

void js_draw_round_rect(int x, int y, int w, int h, int r, int c) {
  host::check_box("drawRoundRect", x, y, w, h);
  const uint32_t k = host::rgb(c);
  for (int j = y; j < y + h; ++j)
    for (int i = x; i < x + w; ++i)
      if (host::in_round(i, j, x, y, w, h, r) &&
          !host::in_round(i, j, x + 1, y + 1, w - 2, h - 2, r > 0 ? r - 1 : 0))
        host::put(i, j, k);
}

void js_draw_line(int x0, int y0, int x1, int y1, int c) {
  host::check_box("drawLine", std::min(x0, x1), std::min(y0, y1), std::abs(x1 - x0) + 1,
                  std::abs(y1 - y0) + 1);
  host::line(x0, y0, x1, y1, host::rgb(c));
}

void js_draw_circle(int x, int y, int r, int c) {
  host::check_box("drawCircle", x - r, y - r, 2 * r + 1, 2 * r + 1);
  const uint32_t k = host::rgb(c);
  for (int j = -r; j <= r; ++j)
    for (int i = -r; i <= r; ++i) {
      const int d = i * i + j * j;
      if (d <= r * r && d > (r - 1) * (r - 1)) host::put(x + i, y + j, k);
    }
}

void js_fill_circle(int x, int y, int r, int c) {
  host::check_box("fillCircle", x - r, y - r, 2 * r + 1, 2 * r + 1);
  const uint32_t k = host::rgb(c);
  for (int j = -r; j <= r; ++j)
    for (int i = -r; i <= r; ++i)
      if (i * i + j * j <= r * r) host::put(x + i, y + j, k);
}

void js_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int c) {
  const int minx = std::min({x0, x1, x2}), maxx = std::max({x0, x1, x2});
  const int miny = std::min({y0, y1, y2}), maxy = std::max({y0, y1, y2});
  host::check_box("fillTriangle", minx, miny, maxx - minx + 1, maxy - miny + 1);
  const uint32_t k = host::rgb(c);
  auto edge = [](int ax, int ay, int bx, int by, int px, int py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
  };
  for (int j = miny; j <= maxy; ++j)
    for (int i = minx; i <= maxx; ++i) {
      const int a = edge(x0, y0, x1, y1, i, j), b = edge(x1, y1, x2, y2, i, j),
                d = edge(x2, y2, x0, y0, i, j);
      if ((a >= 0 && b >= 0 && d >= 0) || (a <= 0 && b <= 0 && d <= 0)) host::put(i, j, k);
    }
}

void js_draw_string(const char* s, int n, int x, int y, int datum, int fg, int bg, int font,
                    float size) {
  const int w = host::text_w(s, n, font, size);
  const int h = host::text_h(font, size);
  const int xa = datum & 3, ya = (datum >> 2) & 3;
  const int left = x - (xa == 1 ? w / 2 : xa == 2 ? w : 0);
  const int top = datum >= 16 ? y - h * 8 / 10 : y - (ya == 1 ? h / 2 : ya == 2 ? h : 0);
  host::check_box(("drawString \"" + std::string(s, static_cast<size_t>(n)) + "\"").c_str(), left,
                  top, w, h);
  if (bg >= 0 && bg != fg) host::fill(left, top, w, h, host::rgb(bg));
  host::paint_text(s, n, left, top, font, size, host::rgb(fg));
  st.texts.push_back(host::Text{std::string(s, static_cast<size_t>(n)), left, top, w, h, font, fg});
}

int js_text_width(const char* s, int n, int font, float size) {
  return host::text_w(s, n, font, size);
}

int js_font_height(int font, float size) { return host::text_h(font, size); }

void js_set_brightness(int level) { st.brightness = level; }

void js_tone(int freq, int ms, int volume, int channel) {
  st.tones.push_back(host::Tone{st.ms, freq, ms, volume, channel});
}

void js_stop_tone(int channel) {
  for (int i = 0; i < 8; ++i)
    if (channel < 0 || channel == i) st.queue_end[i].clear();
}

void js_play_raw(int channel, const int16_t* samples, int count, int rate, int volume) {
  if (channel < 0 || channel >= 8 || count <= 0 || rate <= 0) {
    st.bad.push_back("playRaw with bad arguments");
    return;
  }
  auto& q = st.queue_end[channel];
  const double start = q.empty() ? st.ms : std::max(st.ms, q.back());
  q.push_back(start + 1000.0 * count / rate);
  int peak = 0;
  double sum = 0;
  for (int i = 0; i < count; ++i) {
    peak = std::max(peak, std::abs(static_cast<int>(samples[i])));
    sum += static_cast<double>(samples[i]) * samples[i];
  }
  st.raws.push_back(host::Raw{st.ms, channel, count, rate, volume, peak, std::sqrt(sum / count)});
}

int js_channel_busy(int channel) {
  if (channel < 0 || channel >= 8) return 0;
  auto& q = st.queue_end[channel];
  q.erase(std::remove_if(q.begin(), q.end(), [](double end) { return end <= st.ms; }), q.end());
  return static_cast<int>(std::min<size_t>(q.size(), 2));
}

double js_millis() { return st.ms; }

void js_get_time(int* v) {
  std::tm t;
  const std::time_t now = static_cast<std::time_t>(st.clock_s);
  gmtime_r(&now, &t);
  v[0] = t.tm_year + 1900;
  v[1] = t.tm_mon + 1;
  v[2] = t.tm_mday;
  v[3] = t.tm_hour;
  v[4] = t.tm_min;
  v[5] = t.tm_sec;
}

void js_set_time(int y, int mo, int d, int h, int mi, int s) {
  std::tm t = {};
  t.tm_year = y - 1900;
  t.tm_mon = mo - 1;
  t.tm_mday = d;
  t.tm_hour = h;
  t.tm_min = mi;
  t.tm_sec = s;
  st.clock_s = timegm(&t);
  st.clock_frac = 0;
}

void js_log(const char* s, int n) { st.log.push_back(std::string(s, static_cast<size_t>(n))); }

int js_pref_get(const char* k, int kl, void* buf, int cap) {
  auto it = st.prefs.find(host::key_of(k, kl));
  if (it == st.prefs.end()) return 0;
  // Like ESP32 Preferences: a stored blob bigger than the buffer isn't read.
  if (static_cast<int>(it->second.size()) > cap) return 0;
  std::memcpy(buf, it->second.data(), it->second.size());
  return static_cast<int>(it->second.size());
}

void js_pref_set(const char* k, int kl, const void* buf, int n) {
  const uint8_t* b = static_cast<const uint8_t*>(buf);
  st.prefs[host::key_of(k, kl)] = std::vector<uint8_t>(b, b + n);
}

void js_analog_write(int pin, int value) {
  if (value < 0 || value > 255) st.bad.push_back("analogWrite out of range");
  st.pins[pin] = value;
}

}  // extern "C"
