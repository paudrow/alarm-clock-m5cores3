// Smoke test for sim/clock.wasm, run by CI. It checks the browser build itself:
// boots the firmware at each screen size, taps into the menu, drives it through
// the serial commands the 3D viewer uses (lamp, sound, time travel, button),
// reads back its state, then runs thousands of frames of random input.
// The deeper behaviour tests are in test/firmware_test.cpp.
//   node sim/smoke_test.js

const fs = require("fs");
const path = require("path");

const mod = new WebAssembly.Module(fs.readFileSync(path.join(__dirname, "clock.wasm")));
const FONT_H = { 0: 8, 2: 16, 4: 26, 6: 48, 7: 48, 8: 75, 20: 22, 21: 29, 22: 42, 23: 56, 30: 22, 31: 29, 32: 42, 33: 56 };
const charW = (font) => (font >= 20 ? Math.ceil(FONT_H[font] * 0.5) : { 0: 6, 2: 9, 4: 14, 6: 32, 7: 32, 8: 55 }[font] || 6);
const LAMP_PIN = 9;

function boot(w, h, prefs = {}) {
  let mem;
  const dec = new TextDecoder();
  const enc = new TextEncoder();
  const str = (p, n) => dec.decode(new Uint8Array(mem.buffer, p, n));
  const sim = { w, h, drawn: [], bad: [], log: [], clock: new Date(2026, 8, 29, 7, 41, 5).getTime(), ms: 0, prefs,
    tones: [], raws: [], queue: {}, lamp: -1 };
  const check = (what, ...v) => { if (v.some((x) => !Number.isFinite(x))) sim.bad.push(`${what}(${v.join(", ")})`); };
  const box = (what, x, y, bw, bh) => {
    check(what, x, y, bw, bh);
    if (bw < 0 || bh < 0 || x < -1 || y < -1 || x + bw > w + 1 || y + bh > h + 1) sim.bad.push(`${what}(${x}, ${y}, ${bw}, ${bh}) off screen`);
  };
  const env = {
    js_fill_rect: (x, y, fw, fh) => box("fillRect", x, y, fw, fh),
    js_draw_rect: (x, y, fw, fh) => box("drawRect", x, y, fw, fh),
    js_fill_round_rect: (x, y, fw, fh, r) => { box("fillRoundRect", x, y, fw, fh); check("r", r); },
    js_draw_round_rect: (x, y, fw, fh, r) => { box("drawRoundRect", x, y, fw, fh); check("r", r); },
    js_draw_line: (...a) => check("drawLine", ...a),
    js_draw_circle: (...a) => check("drawCircle", ...a),
    js_fill_circle: (...a) => check("fillCircle", ...a),
    js_fill_triangle: (...a) => check("fillTriangle", ...a),
    js_draw_string: (p, n, x, y, datum, fg, bg, font, size) => { check("drawString", x, y, size); sim.drawn.push(str(p, n)); },
    js_text_width: (p, n, font, size) => Math.round(n * charW(font) * size),
    js_font_height: (font, size) => Math.round((FONT_H[font] || 8) * size),
    js_set_brightness: (v) => check("setBrightness", v),
    js_tone: (freq, ms, vol, ch) => { check("tone", freq, ms, vol, ch); sim.tones.push({ freq, ch }); },
    js_stop_tone: (ch) => { for (const k of Object.keys(sim.queue)) if (ch < 0 || +k === ch) sim.queue[k] = []; },
    js_play_raw: (ch, p, n, rate, vol) => {
      const q = (sim.queue[ch] = (sim.queue[ch] || []).filter((end) => end > sim.ms));
      const start = q.length ? Math.max(sim.ms, q[q.length - 1]) : sim.ms;
      q.push(start + (1000 * n) / rate);
      const s = new Int16Array(mem.buffer, p, n);
      let peak = 0; for (const v of s) peak = Math.max(peak, Math.abs(v));
      sim.raws.push({ ch, n, rate, vol, peak });
    },
    js_channel_busy: (ch) => Math.min(2, (sim.queue[ch] = (sim.queue[ch] || []).filter((end) => end > sim.ms)).length),
    js_millis: () => sim.ms,
    js_get_time: (p) => {
      const d = new Date(sim.clock);
      new Int32Array(mem.buffer, p, 6).set([d.getFullYear(), d.getMonth() + 1, d.getDate(), d.getHours(), d.getMinutes(), d.getSeconds()]);
    },
    js_set_time: (y, mo, d, hh, mi, s) => { sim.clock = new Date(y, mo - 1, d, hh, mi, s).getTime(); },
    js_log: (p, n) => sim.log.push(str(p, n)),
    js_pref_get: (k, kl, buf, cap) => {
      const v = sim.prefs[str(k, kl)];
      if (!v) return 0;
      const n = Math.min(cap, v.length);
      new Uint8Array(mem.buffer, buf, n).set(v.subarray(0, n));
      return n;
    },
    js_pref_set: (k, kl, buf, n) => { sim.prefs[str(k, kl)] = new Uint8Array(mem.buffer, buf, n).slice(); },
    js_analog_write: (pin, v) => { check("analogWrite", pin, v); if (v < 0 || v > 255) sim.bad.push(`analogWrite ${v}`); if (pin === LAMP_PIN) sim.lamp = v; },
  };
  const inst = new WebAssembly.Instance(mod, { env });
  const x = inst.exports;
  mem = x.memory;
  x._initialize();
  x.sim_init(w, h);
  sim.step = (tx, ty, down, dt = 16) => { sim.ms += dt; sim.clock += dt; x.sim_touch(tx, ty, down ? 1 : 0); x.sim_loop(); };
  sim.command = (line) => {
    const bytes = enc.encode(line);
    new Uint8Array(mem.buffer, x.sim_io(), bytes.length).set(bytes);
    x.sim_input(bytes.length);
    const before = sim.log.length;
    sim.step(0, 0, false);
    return sim.log.slice(before).join("");
  };
  sim.state = () => JSON.parse(str(x.sim_io(), x.sim_state()));
  sim.button = () => { x.sim_button(); sim.step(0, 0, false); };
  return sim;
}

let failures = 0;
const fail = (msg) => { console.error(`FAIL ${msg}`); failures++; };
const expect = (ok, msg) => { if (!ok) fail(msg); };

for (const [w, h] of [[616, 284], [600, 450], [320, 240]]) {
  const label = `${w}x${h}`;
  const before = failures;
  const sim = boot(w, h);
  for (let i = 0; i < 3; i++) sim.step(0, 0, false);
  expect(sim.drawn.some((t) => /^\d\d:\d\d/.test(t)), `${label}: the clock face didn't draw a time`);

  // The gear is the top-right icon; it opens the settings tiles
  const s = Math.min(w * 100 / 320 | 0, h * 100 / 240 | 0) / 100;
  const icon = Math.min(Math.max(Math.round(40 * s), 24), h / 4 | 0);
  sim.drawn = [];
  sim.step(w - 6 - icon / 2, icon / 2, true);
  sim.step(w - 6 - icon / 2, icon / 2, false);
  sim.step(0, 0, false);
  for (const item of ["Settings", "Alarm", "Alarm sound", "Sleep sounds", "Lamp", "Day screen", "Night screen"]) {
    expect(sim.drawn.includes(item), `${label}: tapping the gear didn't show "${item}"`);
  }
  expect(sim.state().screen === "settings", `${label}: state doesn't say settings`);

  // The commands the 3D viewer sends
  expect(sim.command("lamp 100").startsWith("ok"), `${label}: lamp command refused`);
  expect(sim.lamp === 255, `${label}: lamp 100 didn't drive the lamp pin fully (${sim.lamp})`);
  sim.button();
  expect(sim.lamp === 0, `${label}: the button didn't put the lamp out`);
  sim.command("sound brown");
  for (let i = 0; i < 200; i++) sim.step(0, 0, false);
  const st = sim.state();
  expect(st.sound.playing && st.sound.kind === "brown", `${label}: sound brown isn't playing`);
  expect(sim.raws.length > 0 && sim.raws.every((r) => r.ch === 1 && r.rate === 24000), `${label}: no noise streamed on channel 1`);
  expect(sim.raws.some((r) => r.peak > 1000), `${label}: the noise is silent`);
  sim.command("alarm 06:00");
  sim.command("go alarm");
  for (let i = 0; i < 15 * 60; i++) sim.step(0, 0, false);
  const ringing = sim.state();
  expect(ringing.alarm.state === "ringing", `${label}: go alarm didn't ring (${ringing.alarm.state})`);
  expect(!ringing.sound.playing, `${label}: sleep sounds kept playing through the alarm`);
  expect(sim.tones.some((t) => t.ch === 0), `${label}: no alarm beeps`);
  sim.command("go night");
  sim.step(0, 0, false);
  expect(sim.state().night === true, `${label}: go night isn't night`);
  sim.command("go day");
  sim.step(0, 0, false);
  expect(sim.state().night === false, `${label}: go day isn't day`);
  expect(sim.command("nonsense").startsWith("?"), `${label}: an unknown command wasn't refused`);

  // Random touches, holds, drags, commands and time jumps
  let seed = 12345 + w, down = false, x = 0, y = 0, hold = 0;
  const rnd = () => (seed = (seed * 1103515245 + 12345) >>> 0) / 4294967296;
  const cmds = ["lamp toggle", "sound toggle", "go day", "go night", "go alarm", "go sunrise", "button", "state"];
  try {
    for (let i = 0; i < 6000; i++) {
      if (rnd() < 0.002) sim.clock += rnd() * 86400000;
      else if (rnd() < 0.3) sim.clock += 16 * 600;
      if (rnd() < 0.003) sim.command(cmds[(rnd() * cmds.length) | 0]);
      if (!down && rnd() < 0.08) { down = true; x = (rnd() * w) | 0; y = (rnd() * h) | 0; hold = (rnd() * (rnd() < 0.2 ? 90 : 6)) | 0; }
      else if (down && hold-- <= 0) down = false;
      sim.step(x, y, down);
    }
  } catch (err) {
    fail(`${label}: the firmware crashed under random input: ${err.message}`);
  }
  if (sim.bad.length) fail(`${label}: invalid draw calls, e.g. ${sim.bad.slice(0, 3).join("; ")}`);

  // Settings saved during the run must load after a reboot
  expect(Object.keys(sim.prefs).length > 0, `${label}: no settings were saved`);
  const saved = sim.state();
  const again = boot(w, h, sim.prefs);
  try { again.step(0, 0, false); } catch (err) { fail(`${label}: reboot with saved settings crashed: ${err.message}`); }
  const back = again.state();
  expect(back.alarm.at === saved.alarm.at && back.lamp.brightness === saved.lamp.brightness && back.sound.kind === saved.sound.kind,
    `${label}: settings didn't survive a reboot`);

  if (failures === before) console.log(`ok ${label}: clock face, settings, commands, sleep sounds, lamp, alarm, 6000 random frames, reboot`);
}

process.exit(failures ? 1 : 0);
