// Runs the real firmware (src/main.cpp, through sim/shim and sim/sim.cpp) on a
// PC. The js_* functions the shim calls are implemented in host.cpp: they keep
// a clock, a speaker, a lamp pin and settings storage, check every draw call
// stays on the screen, and paint a picture for test/screens/.

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

extern "C" {
void sim_init(int w, int h);
void sim_loop();
void sim_touch(int x, int y, int down);
void sim_button();
char* sim_io();
void sim_input(int n);
int sim_state();
}

namespace host {

struct Tone {
  double at_ms;
  int freq;
  int ms;
  int volume;
  int channel;
};

struct Raw {
  double at_ms;
  int channel;
  int count;
  int rate;
  int volume;
  int peak;
  double rms;
};

struct Text {
  std::string s;
  int x, y, w, h;  // box on screen
  int font;
  int color;
};

struct State {
  int w = 320, h = 240;
  double ms = 0;         // millis() since boot
  int64_t clock_s = 0;   // wall clock, seconds since 1970 (UTC, used as local)
  double clock_frac = 0; // ms part of the wall clock
  int speed = 1;         // wall clock runs this many times real time
  std::map<std::string, std::vector<uint8_t>> prefs;
  int brightness = -1;
  std::map<int, int> pins;  // analogWrite
  std::vector<Tone> tones;
  std::vector<Raw> raws;
  double busy_until[8] = {0};
  int queued[8] = {0};
  std::vector<double> queue_end[8];
  int underruns[8] = {0};        // the speaker ran dry between buffers
  double last_end[8] = {0};
  std::vector<std::string> log;
  std::vector<std::string> bad;  // invariant violations
  std::vector<Text> texts;       // text on screen: drawn since the screen was last cleared
  long draw_calls = 0;
  std::vector<uint32_t> px;      // RGB888 canvas
};

extern State st;

// Boots the firmware with a w x h screen. Storage (st.prefs) is kept, so a
// second boot is a reboot after a power cut.
void boot(int w, int h, int year = 2026, int month = 9, int day = 29, int hour = 7,
          int minute = 41, int second = 5);
// Runs n firmware loops, each dt_ms of time, with the finger as given.
void frames(int n, int dt_ms = 16);
void touch(int x, int y, bool down);
// Tap: press for two frames, release, then one frame to settle.
void tap(int x, int y);
void hold(int x, int y, int ms);
void button();
// Types a serial command and runs a frame so it takes effect. Returns what the
// firmware printed in reply.
std::string command(const std::string& line);
std::string state();
void set_clock(int hour, int minute, int second);
int clock_hour();
int clock_minute();
int clock_second();
void clear_frame_record();
bool drew(const std::string& text);
bool drew_prefix(const std::string& prefix);
int lamp_duty();

// Reads a value from the state JSON: jget(s, "alarm", "state").
std::string jget(const std::string& json, const std::string& a, const std::string& b = "");

bool write_png(const std::string& path);

}  // namespace host
