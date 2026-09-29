// Stand-in for the ESP32 Preferences (NVS) library. Values are kept by the
// browser (localStorage, via js_pref_*), so settings survive a page reload the
// way they survive a power cut on the real clock.

#pragma once

#include "M5Unified.h"

class Preferences {
 public:
  bool begin(const char* ns, bool read_only = false) {
    ns_ = ns;
    read_only_ = read_only;
    return true;
  }
  void end() {}

  size_t getBytes(const char* key, void* buf, size_t len) {
    char k[64];
    const int n = snprintf(k, sizeof k, "%s/%s", ns_, key);
    const int got = js_pref_get(k, n, buf, static_cast<int>(len));
    return got < 0 ? 0 : static_cast<size_t>(got);
  }
  size_t putBytes(const char* key, const void* buf, size_t len) {
    if (read_only_) return 0;
    char k[64];
    const int n = snprintf(k, sizeof k, "%s/%s", ns_, key);
    js_pref_set(k, n, buf, static_cast<int>(len));
    return len;
  }

 private:
  const char* ns_ = "";
  bool read_only_ = false;
};
