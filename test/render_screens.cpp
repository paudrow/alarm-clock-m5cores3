#include "alarm_face.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

const Rgb kWhite = {255, 255, 255};
const Rgb kRed = {255, 40, 40};

struct Image {
  int w;
  int h;
  std::vector<uint8_t> px;
};

Image make_image(int w, int h) {
  Image im;
  im.w = w;
  im.h = h;
  im.px.assign(static_cast<size_t>(w * h * 3), 0);
  return im;
}

void put(Image& im, int x, int y, Rgb c) {
  if (x < 0 || y < 0 || x >= im.w || y >= im.h) {
    return;
  }
  const size_t i = static_cast<size_t>((y * im.w + x) * 3);
  im.px[i] = c.r;
  im.px[i + 1] = c.g;
  im.px[i + 2] = c.b;
}

void stroke(Image& im, Box b, Rgb c) {
  for (int x = b.x; x < b.x + b.w; ++x) {
    put(im, x, b.y, c);
    put(im, x, b.y + b.h - 1, c);
  }
  for (int y = b.y; y < b.y + b.h; ++y) {
    put(im, b.x, y, c);
    put(im, b.x + b.w - 1, y, c);
  }
}

int glyph(char ch, int row) {
  static const char* rows[40] = {
      "111101101101111", "010110010010111", "111001111100111", "111001111001111",
      "101101111001001", "111100111001111", "111100111101111", "111001001001001",
      "111101111101111", "111101111001111", "010101111101101", "110101110101110",
      "111100100100111", "110101101101110", "111100111100111", "111100111100100",
      "111100101101111", "101101111101101", "111010010010111", "001001001101010",
      "101101110101101", "100100100100111", "101111101101101", "110101101101101",
      "111101101101111", "111101111100100", "111101101111001", "111101110101101",
      "111100111001111", "111010010010010", "101101101101111", "101101101101010",
      "101101101111101", "101101010101101", "101101010010010", "111001010100111",
      "000000000000000", "000010000010000", "000000111000000", "010010111010010"};
  int index = 36;
  if (ch >= '0' && ch <= '9') {
    index = ch - '0';
  } else if (ch >= 'A' && ch <= 'Z') {
    index = 10 + (ch - 'A');
  } else if (ch >= 'a' && ch <= 'z') {
    index = 10 + (ch - 'a');
  } else if (ch == ':') {
    index = 37;
  } else if (ch == '-') {
    index = 38;
  } else if (ch == '+') {
    index = 39;
  }
  const char* bits = rows[index];
  int value = 0;
  for (int col = 0; col < 3; ++col) {
    if (bits[row * 3 + col] == '1') {
      value |= 1 << (2 - col);
    }
  }
  return value;
}

int text_width(const char* text, int scale) {
  const int n = static_cast<int>(std::strlen(text));
  if (n == 0) {
    return 0;
  }
  return n * 4 * scale - scale;
}

void text_at(Image& im, int x, int y, const char* text, Rgb c, int scale) {
  for (int i = 0; text[i] != '\0'; ++i) {
    for (int row = 0; row < 5; ++row) {
      const int bits = glyph(text[i], row);
      for (int col = 0; col < 3; ++col) {
        if ((bits & (1 << (2 - col))) == 0) {
          continue;
        }
        for (int sy = 0; sy < scale; ++sy) {
          for (int sx = 0; sx < scale; ++sx) {
            put(im, x + col * scale + sx, y + row * scale + sy, c);
          }
        }
      }
    }
    x += 4 * scale;
  }
}

void label(Image& im, Box b, const char* text, Rgb c) {
  int scale = 3;
  while (scale > 1 && (text_width(text, scale) > b.w - 8 || 5 * scale > b.h - 6)) {
    --scale;
  }
  const int tw = text_width(text, scale);
  const int th = 5 * scale;
  text_at(im, b.x + (b.w - tw) / 2, b.y + (b.h - th) / 2, text, c, scale);
}

void button(Image& im, Box b, const char* text, Rgb c) {
  stroke(im, b, c);
  label(im, b, text, c);
}

void icon_skip(Image& im, Box b, Rgb c) {
  const int x = b.x + 8;
  const int y = b.y + b.h / 2;
  for (int i = 0; i < 8; ++i) {
    put(im, x + i, y - i, c);
    put(im, x + i, y + i, c);
    put(im, x + 10 + i, y - i, c);
    put(im, x + 10 + i, y + i, c);
  }
}

void icon_bell(Image& im, Box b, Rgb c, bool enabled) {
  const int cx = b.x + b.w / 2;
  const int top = b.y + 8;
  for (int i = 0; i < 10; ++i) {
    put(im, cx - 8 + i, top + 14, c);
    put(im, cx - 2 + i / 3, top + i, c);
    put(im, cx + 2 - i / 3, top + i, c);
  }
  if (!enabled) {
    for (int i = 0; i < 22; ++i) {
      put(im, b.x + 8 + i, b.y + 8 + i, c);
    }
  }
}

void icon_gear(Image& im, Box b, Rgb c) {
  const int cx = b.x + b.w / 2;
  const int cy = b.y + b.h / 2;
  for (int a = 0; a < 12; ++a) {
    put(im, cx - 6 + a, cy - 6, c);
    put(im, cx - 6 + a, cy + 5, c);
    put(im, cx - 6, cy - 6 + a, c);
    put(im, cx + 5, cy - 6 + a, c);
  }
  for (int i = -2; i <= 2; ++i) {
    put(im, cx + i, cy - 10, c);
    put(im, cx + i, cy + 9, c);
    put(im, cx - 10, cy + i, c);
    put(im, cx + 9, cy + i, c);
  }
}

uint32_t crc_table[256];
bool crc_ready = false;

void init_crc() {
  if (crc_ready) {
    return;
  }
  for (uint32_t n = 0; n < 256; ++n) {
    uint32_t c = n;
    for (int k = 0; k < 8; ++k) {
      c = (c & 1u) ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
    }
    crc_table[n] = c;
  }
  crc_ready = true;
}

uint32_t crc32(const uint8_t* data, size_t n) {
  init_crc();
  uint32_t c = 0xffffffffu;
  for (size_t i = 0; i < n; ++i) {
    c = crc_table[(c ^ data[i]) & 0xffu] ^ (c >> 8);
  }
  return c ^ 0xffffffffu;
}

void be32(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back(static_cast<uint8_t>(v >> 24));
  out.push_back(static_cast<uint8_t>(v >> 16));
  out.push_back(static_cast<uint8_t>(v >> 8));
  out.push_back(static_cast<uint8_t>(v));
}

void chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
  be32(out, static_cast<uint32_t>(data.size()));
  const size_t start = out.size();
  out.push_back(static_cast<uint8_t>(type[0]));
  out.push_back(static_cast<uint8_t>(type[1]));
  out.push_back(static_cast<uint8_t>(type[2]));
  out.push_back(static_cast<uint8_t>(type[3]));
  out.insert(out.end(), data.begin(), data.end());
  const uint32_t sum = crc32(&out[start], out.size() - start);
  be32(out, sum);
}

void write_png(const char* path, const Image& im) {
  std::vector<uint8_t> raw;
  raw.reserve(static_cast<size_t>(im.h * (1 + im.w * 3)));
  for (int y = 0; y < im.h; ++y) {
    raw.push_back(0);
    const size_t row = static_cast<size_t>(y * im.w * 3);
    raw.insert(raw.end(), im.px.begin() + static_cast<std::ptrdiff_t>(row),
               im.px.begin() + static_cast<std::ptrdiff_t>(row + im.w * 3));
  }

  std::vector<uint8_t> zlib;
  zlib.push_back(0x78);
  zlib.push_back(0x01);
  size_t offset = 0;
  uint32_t adler = 1;
  uint32_t s1 = 1;
  uint32_t s2 = 0;
  while (offset < raw.size()) {
    const size_t n = raw.size() - offset > 65535 ? 65535 : raw.size() - offset;
    const int last = offset + n == raw.size() ? 1 : 0;
    zlib.push_back(static_cast<uint8_t>(last));
    zlib.push_back(static_cast<uint8_t>(n & 0xff));
    zlib.push_back(static_cast<uint8_t>((n >> 8) & 0xff));
    const uint16_t nnot = static_cast<uint16_t>(~n);
    zlib.push_back(static_cast<uint8_t>(nnot & 0xff));
    zlib.push_back(static_cast<uint8_t>((nnot >> 8) & 0xff));
    zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset),
                raw.begin() + static_cast<std::ptrdiff_t>(offset + n));
    for (size_t i = 0; i < n; ++i) {
      s1 = (s1 + raw[offset + i]) % 65521;
      s2 = (s2 + s1) % 65521;
    }
    offset += n;
  }
  adler = (s2 << 16) | s1;
  be32(zlib, adler);

  std::vector<uint8_t> png;
  const uint8_t sig[] = {137, 80, 78, 71, 13, 10, 26, 10};
  png.insert(png.end(), sig, sig + 8);
  std::vector<uint8_t> ihdr;
  be32(ihdr, static_cast<uint32_t>(im.w));
  be32(ihdr, static_cast<uint32_t>(im.h));
  ihdr.push_back(8);
  ihdr.push_back(2);
  ihdr.push_back(0);
  ihdr.push_back(0);
  ihdr.push_back(0);
  chunk(png, "IHDR", ihdr);
  chunk(png, "IDAT", zlib);
  chunk(png, "IEND", std::vector<uint8_t>());

  FILE* f = std::fopen(path, "wb");
  if (f == 0) {
    std::perror(path);
    return;
  }
  std::fwrite(png.data(), 1, png.size(), f);
  std::fclose(f);
}

void clock_screen(Image& im, Rgb ink, const char* time, bool enabled, bool ringing) {
  const int w = im.w;
  const int h = im.h;
  text_at(im, (w - text_width(time, 6)) / 2, ringing ? 70 : 90, time, ink, 6);
  stroke(im, skip_box(w), ink);
  stroke(im, bell_box(w), ink);
  stroke(im, gear_box(w), ink);
  icon_skip(im, skip_box(w), ink);
  icon_bell(im, bell_box(w), ink, enabled);
  icon_gear(im, gear_box(w), ink);
  if (ringing) {
    button(im, ring_action_box(w, h, false), "Snooze", ink);
    button(im, ring_action_box(w, h, true), "Stop", ink);
  }
}

void menu_screen(Image& im, Rgb ink) {
  const int w = im.w;
  const int h = im.h;
  button(im, back_button(5, w, h), "Back", ink);
  label(im, Box{w - 90, 10, 80, 28}, "Menu", ink);
  button(im, centered_button(1, 5, w, h, 180), "Alarm", ink);
  button(im, centered_button(2, 5, w, h, 180), "Sound", ink);
  button(im, centered_button(3, 5, w, h, 180), "Day", ink);
  button(im, centered_button(4, 5, w, h, 180), "Night", ink);
}

void alarm_screen(Image& im, Rgb ink) {
  const int w = im.w;
  const int h = im.h;
  button(im, back_button(3, w, h), "Back", ink);
  button(im, centered_button(1, 3, w, h, 220), "07:00 ON", ink);
  const char* steps[] = {"H-", "H+", "M-", "M+"};
  for (int i = 0; i < 4; ++i) {
    button(im, column_button(2, 3, i, 4, w, h), steps[i], ink);
  }
}

void sound_screen(Image& im, Rgb ink) {
  const int w = im.w;
  const int h = im.h;
  button(im, back_button(5, w, h), "Back", ink);
  button(im, side_button(1, 5, w, h, false), "-", ink);
  button(im, side_button(1, 5, w, h, true), "+", ink);
  label(im, Box{80, 58, 160, 28}, "Loud 80", ink);
  button(im, side_button(2, 5, w, h, false), "-", ink);
  button(im, side_button(2, 5, w, h, true), "+", ink);
  label(im, Box{80, 106, 160, 28}, "Soft 40", ink);
  button(im, side_button(3, 5, w, h, false), "-", ink);
  button(im, side_button(3, 5, w, h, true), "+", ink);
  label(im, Box{80, 154, 160, 28}, "30s then loud", ink);
  button(im, column_button(4, 5, 0, 2, w, h), "Preview soft", ink);
  button(im, column_button(4, 5, 1, 2, w, h), "Preview loud", ink);
}

void day_screen(Image& im, Rgb ink) {
  const int w = im.w;
  const int h = im.h;
  button(im, back_button(4, w, h), "Back", ink);
  button(im, side_button(1, 4, w, h, false), "-", ink);
  button(im, side_button(1, 4, w, h, true), "+", ink);
  label(im, Box{80, 70, 160, 40}, "Bright 70", ink);
  button(im, centered_button(2, 4, w, h, 220), "Face H:M:S", ink);
  button(im, centered_button(3, 4, w, h, 220), "24 hour", ink);
}

void night_screen(Image& im, Rgb ink) {
  const int w = im.w;
  const int h = im.h;
  button(im, back_button(5, w, h), "Back", ink);
  button(im, side_button(1, 5, w, h, false), "-", ink);
  button(im, side_button(1, 5, w, h, true), "+", ink);
  label(im, Box{80, 58, 160, 28}, "Bright 20", ink);
  button(im, centered_button(2, 5, w, h, 220), "Face H:M:S", ink);
  button(im, centered_button(3, 5, w, h, 220), "24 hour", ink);
  button(im, centered_button(4, 5, w, h, 220), "21:00-07:00", ink);
}

void when_screen(Image& im, Rgb ink) {
  const int w = im.w;
  const int h = im.h;
  button(im, back_button(4, w, h), "Back", ink);
  button(im, centered_button(1, 4, w, h, 220), "Follow clock", ink);
  button(im, side_button(2, 4, w, h, false), "-", ink);
  button(im, side_button(2, 4, w, h, true), "+", ink);
  label(im, Box{80, 130, 160, 40}, "Start 21:00", ink);
  button(im, side_button(3, 4, w, h, false), "-", ink);
  button(im, side_button(3, 4, w, h, true), "+", ink);
  label(im, Box{80, 190, 160, 40}, "End 07:00", ink);
}

void save(const std::string& dir, const char* name, const Image& im) {
  const std::string path = dir + "/" + name;
  write_png(path.c_str(), im);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: render_screens directory\n");
    return 1;
  }
  const std::string dir = argv[1];
  const int w = 320;
  const int h = 240;

  Image day = make_image(w, h);
  clock_screen(day, kWhite, "12:00:00", true, false);
  save(dir, "clock-day.png", day);

  Image night = make_image(w, h);
  clock_screen(night, kRed, "22:00:00", true, false);
  save(dir, "clock-night.png", night);

  Image ringing = make_image(w, h);
  clock_screen(ringing, kRed, "07:00:10", true, true);
  save(dir, "clock-ringing.png", ringing);

  Image menu = make_image(w, h);
  menu_screen(menu, kWhite);
  save(dir, "menu.png", menu);

  Image alarm = make_image(w, h);
  alarm_screen(alarm, kWhite);
  save(dir, "alarm.png", alarm);

  Image sound = make_image(w, h);
  sound_screen(sound, kWhite);
  save(dir, "sound.png", sound);

  Image day_page = make_image(w, h);
  day_screen(day_page, kWhite);
  save(dir, "day.png", day_page);

  Image night_page = make_image(w, h);
  night_screen(night_page, kRed);
  save(dir, "night.png", night_page);

  Image when = make_image(w, h);
  when_screen(when, kWhite);
  save(dir, "when.png", when);
  return 0;
}
