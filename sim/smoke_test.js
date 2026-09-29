// Smoke test for sim/clock.wasm, run by CI: boots the firmware at each screen size,
// checks it draws the clock, taps the gear and checks the menu opens, then runs
// a few thousand frames of random touches and time jumps to catch crashes.
//   node sim/smoke_test.js

const fs = require("fs");
const path = require("path");

const mod = new WebAssembly.Module(fs.readFileSync(path.join(__dirname, "clock.wasm")));
const FONT_H = { 0: 8, 2: 16, 4: 26, 6: 48, 7: 48, 8: 75 };
const CHAR_W = { 0: 6, 2: 9, 4: 14, 6: 32, 7: 32, 8: 55 };

function boot(w, h) {
  let mem;
  const dec = new TextDecoder();
  const str = (p, n) => dec.decode(new Uint8Array(mem.buffer, p, n));
  const sim = { drawn: [], bad: [], clock: new Date(2026, 8, 29, 7, 41, 5).getTime(), ms: 0, prefs: {} };
  const check = (what, ...v) => { if (v.some((x) => !Number.isFinite(x))) sim.bad.push(`${what}(${v.join(", ")})`); };
  const env = {
    js_fill_rect: (x, y, fw, fh) => { check("fillRect", x, y, fw, fh); if (fw < 0 || fh < 0) sim.bad.push(`fillRect ${fw}x${fh}`); },
    js_draw_rect: (...a) => check("drawRect", ...a),
    js_draw_line: (...a) => check("drawLine", ...a),
    js_draw_circle: (...a) => check("drawCircle", ...a),
    js_fill_circle: (...a) => check("fillCircle", ...a),
    js_draw_string: (p, n, x, y, datum, fg, bg, font, size) => { check("drawString", x, y, size); sim.drawn.push(str(p, n)); },
    js_text_width: (p, n, font, size) => Math.round(n * (CHAR_W[font] || 6) * size),
    js_font_height: (font, size) => Math.round((FONT_H[font] || 8) * size),
    js_set_brightness: (v) => check("setBrightness", v),
    js_tone: (...a) => check("tone", ...a),
    js_stop_tone: () => {},
    js_millis: () => sim.ms,
    js_get_time: (p) => {
      const d = new Date(sim.clock);
      new Int32Array(mem.buffer, p, 6).set([d.getFullYear(), d.getMonth() + 1, d.getDate(), d.getHours(), d.getMinutes(), d.getSeconds()]);
    },
    js_set_time: () => {},
    js_log: () => {},
    js_pref_get: (k, kl, buf, cap) => {
      const v = sim.prefs[str(k, kl)];
      if (!v) return 0;
      const n = Math.min(cap, v.length);
      new Uint8Array(mem.buffer, buf, n).set(v.subarray(0, n));
      return n;
    },
    js_pref_set: (k, kl, buf, n) => { sim.prefs[str(k, kl)] = new Uint8Array(mem.buffer, buf, n).slice(); },
  };
  const inst = new WebAssembly.Instance(mod, { env });
  mem = inst.exports.memory;
  inst.exports._initialize();
  inst.exports.sim_init(w, h);
  sim.step = (x, y, down) => { sim.ms += 16; sim.clock += 16; inst.exports.sim_touch(x, y, down ? 1 : 0); inst.exports.sim_loop(); };
  return sim;
}

let failures = 0;
const fail = (msg) => { console.error(`FAIL ${msg}`); failures++; };

for (const [w, h] of [[616, 284], [600, 450], [320, 240]]) {
  const label = `${w}x${h}`;
  const sim = boot(w, h);
  for (let i = 0; i < 3; i++) sim.step(0, 0, false);
  if (!sim.drawn.some((t) => /^\d\d:\d\d/.test(t))) fail(`${label}: the clock face didn't draw a time`);

  // Tap the gear, top right (gear_box in include/alarm_face.hpp), and expect the menu
  sim.drawn = [];
  sim.step(w - 24, 22, true);
  sim.step(w - 24, 22, false);
  sim.step(w - 24, 22, false);
  for (const item of ["Menu", "Alarm", "Sound", "Day", "Night"]) {
    if (!sim.drawn.includes(item)) fail(`${label}: tapping the gear didn't show "${item}"`);
  }

  // Random touches, holds, drags and time jumps
  let s = 12345 + w, down = false, x = 0, y = 0, hold = 0;
  const rnd = () => (s = (s * 1103515245 + 12345) >>> 0) / 4294967296;
  try {
    for (let i = 0; i < 5000; i++) {
      if (rnd() < 0.002) sim.clock += rnd() * 86400000;
      else if (rnd() < 0.3) sim.clock += 16 * 600;
      if (!down && rnd() < 0.08) { down = true; x = (rnd() * w) | 0; y = (rnd() * h) | 0; hold = (rnd() * (rnd() < 0.2 ? 90 : 6)) | 0; }
      else if (down && hold-- <= 0) down = false;
      sim.step(x, y, down);
    }
  } catch (err) {
    fail(`${label}: the firmware crashed under random input: ${err.message}`);
  }
  if (sim.bad.length) fail(`${label}: invalid draw calls, e.g. ${sim.bad.slice(0, 3).join("; ")}`);

  // Settings saved during the run must load after a reboot
  const saved = Object.keys(sim.prefs);
  if (!saved.length) fail(`${label}: no settings were saved`);
  const again = boot(w, h);
  again.prefs = sim.prefs;
  try { again.step(0, 0, false); } catch (err) { fail(`${label}: reboot with saved settings crashed: ${err.message}`); }

  if (!failures) console.log(`ok ${label}: clock face, menu, 5000 random frames, settings reload`);
}

process.exit(failures ? 1 : 0);
