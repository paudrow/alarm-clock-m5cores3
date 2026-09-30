#include "alarm_face.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

int failures = 0;

void check(bool ok, const char* name) {
  if (!ok) {
    std::fprintf(stderr, "%s\n", name);
    failures = 1;
  }
}

void check(bool ok, const std::string& name) { check(ok, name.c_str()); }

bool step_is(Step a, Occurrence occurrence, Intensity intensity, bool skip_next) {
  return a.occurrence == occurrence && a.intensity == intensity &&
         a.skip_next == skip_next;
}

Inputs in_at(int hour, int minute, int second, bool touched, int gentle) {
  return Inputs{Hm{hour, minute}, second, touched, gentle, false, false, Hm{0, 0}};
}

bool alarm_eq(Alarm a, Alarm b) {
  return a.at.hour == b.at.hour && a.at.minute == b.at.minute &&
         a.enabled == b.enabled && a.skip_next == b.skip_next;
}

bool contains(Box outer, Box inner) {
  return inner.x >= outer.x && inner.y >= outer.y &&
         inner.x + inner.w <= outer.x + outer.w &&
         inner.y + inner.h <= outer.y + outer.h;
}

std::string size_name(int w, int h) {
  return std::to_string(w) + "x" + std::to_string(h);
}

// The layout rules every screen size must keep.
void check_layout(int w, int h) {
  const Ui ui = ui_for(w, h);
  const Box screen = {0, 0, w, h};
  const std::string at = size_name(w, h) + ": ";
  const bool big = w >= 320 && h >= 240;
  const int min_touch = big ? 40 : 20;

  // Clock face: icons in a row at the top, the digits under them, the ring
  // buttons and status line at the bottom, nothing overlapping.
  const ClockHit icons[] = {ClockHit::Gear, ClockHit::Bell, ClockHit::Skip,
                            ClockHit::Lamp, ClockHit::Sound};
  for (int i = 0; i < 5; ++i) {
    const Box a = clock_icon_box(ui, icons[i]);
    check(contains(screen, a), at + "clock icon on screen");
    check(a.w >= min_touch - 16 && a.h >= min_touch - 16, at + "clock icon big enough");
    check(clock_hit(ui, a.x + a.w / 2, a.y + a.h / 2) == icons[i], at + "clock icon hit");
    for (int j = i + 1; j < 5; ++j) {
      check(!boxes_overlap(a, clock_icon_box(ui, icons[j])), at + "clock icons apart");
    }
    check(!boxes_overlap(a, clock_digits_box(ui, false)), at + "icons above the digits");
    check(!boxes_overlap(a, clock_digits_box(ui, true)), at + "icons above the digits, ringing");
  }
  check(clock_hit(ui, w / 2, h / 2) == ClockHit::Face, at + "middle is the face");
  const Box snooze = ring_box(ui, false);
  const Box stop = ring_box(ui, true);
  check(contains(screen, snooze) && contains(screen, stop), at + "ring buttons on screen");
  check(!boxes_overlap(snooze, stop), at + "ring buttons apart");
  check(snooze.h >= min_touch - 16 && snooze.w >= w / 3, at + "ring buttons big");
  check(ring_hit(ui, snooze.x + 2, snooze.y + 2) == RingHit::Snooze, at + "snooze hit");
  check(ring_hit(ui, stop.x + stop.w - 3, stop.y + stop.h - 3) == RingHit::Stop, at + "stop hit");
  check(ring_hit(ui, w / 2, snooze.y + snooze.h / 2) == RingHit::None, at + "gap between ring buttons");
  check(ring_hit(ui, -1, -1) == RingHit::None && ring_hit(ui, w, h) == RingHit::None,
        at + "ring_hit off screen");
  const Box digits = clock_digits_box(ui, true);
  check(digits.h > 0 && !boxes_overlap(digits, snooze), at + "digits clear of ring buttons");
  const Box quiet = clock_digits_box(ui, false);
  check(quiet.h >= digits.h && !boxes_overlap(quiet, clock_status_box(ui)),
        at + "digits clear of status line");
  check(contains(screen, clock_status_box(ui)), at + "status line on screen");

  // Settings menu tiles
  for (int i = 0; i < kMenuItems; ++i) {
    const Box t = tile_box(ui, i, kMenuItems);
    check(contains(screen, t), at + "tile on screen");
    check(t.y >= ui.header, at + "tile under the header");
    check(t.h >= min_touch && t.w >= min_touch, at + "tile big enough");
    check(tile_hit(ui, kMenuItems, t.x + t.w / 2, t.y + t.h / 2) == i, at + "tile hit");
    check(!boxes_overlap(t, header_back_box(ui)), at + "tile clear of Back");
    for (int j = i + 1; j < kMenuItems; ++j) {
      check(!boxes_overlap(t, tile_box(ui, j, kMenuItems)), at + "tiles apart");
    }
  }
  check(tile_hit(ui, kMenuItems, 0, 0) == -1, at + "corner is no tile");

  // Settings pages
  const Box back = header_back_box(ui);
  check(contains(screen, back) && back.h >= min_touch - 16, at + "Back on screen and big");
  check(header_title_box(ui).w > w / 3, at + "room for a title");
  const Screen pages[] = {Screen::Alarm,     Screen::AlarmSound,  Screen::SleepSounds,
                          Screen::Lamp,      Screen::DayScreen,   Screen::NightScreen,
                          Screen::NightHours};
  for (Screen s : pages) {
    RowKind kinds[kMaxRows];
    const int n = page_rows(s, kinds);
    check(n >= 1 && n <= kMaxRows, at + "page has rows");
    for (int r = 0; r < n; ++r) {
      const Box row = row_box(ui, r, n);
      const std::string where = at + screen_key(s) + " row " + std::to_string(r) + ": ";
      check(contains(screen, row), where + "on screen");
      check(!boxes_overlap(row, back) && row.y >= ui.header, where + "under the header");
      check(row.h >= min_touch - (big ? 0 : 0), where + "tall enough to touch");
      if (r + 1 < n) {
        check(!boxes_overlap(row, row_box(ui, r + 1, n)), where + "apart from the next");
      }
      const Box label = row_label_box(ui, row, kinds[r]);
      check(label.w > 0 && contains(row, label), where + "label has room");
      if (kinds[r] == RowKind::Stepper) {
        const Box minus = row_part_box(ui, row, Part::Minus);
        const Box plus = row_part_box(ui, row, Part::Plus);
        const Box value = stepper_value_box(ui, row);
        check(contains(row, minus) && contains(row, plus) && contains(row, value),
              where + "stepper parts in the row");
        check(!boxes_overlap(minus, plus) && !boxes_overlap(minus, value) &&
                  !boxes_overlap(value, plus) && !boxes_overlap(label, minus),
              where + "stepper parts apart");
        check(minus.w >= min_touch - 8, where + "- and + big enough");
        const Hit hm = page_hit(ui, kinds, n, minus.x + minus.w / 2, minus.y + minus.h / 2);
        const Hit hp = page_hit(ui, kinds, n, plus.x + plus.w / 2, plus.y + plus.h / 2);
        check(hm.part == Part::Minus && hm.row == r, where + "- hit");
        check(hp.part == Part::Plus && hp.row == r, where + "+ hit");
        const Hit hv = page_hit(ui, kinds, n, value.x + value.w / 2, value.y + value.h / 2);
        check(hv.part == Part::None, where + "value isn't a button");
      } else if (kinds[r] == RowKind::Pair) {
        const Box l = row_part_box(ui, row, Part::Left);
        const Box rr = row_part_box(ui, row, Part::Right);
        check(!boxes_overlap(l, rr) && contains(row, l) && contains(row, rr), where + "pair apart");
        check(page_hit(ui, kinds, n, l.x + 1, l.y + 1).part == Part::Left, where + "left hit");
        check(page_hit(ui, kinds, n, rr.x + rr.w - 2, rr.y + 1).part == Part::Right, where + "right hit");
      } else {
        const Hit hw = page_hit(ui, kinds, n, row.x + 1, row.y + row.h - 2);
        check(hw.part == Part::Whole && hw.row == r, where + "whole row hit");
      }
    }
    check(page_hit(ui, kinds, n, back.x + 1, back.y + 1).part == Part::Back, at + "Back hit");
    check(page_hit(ui, kinds, n, -5, 10).part == Part::None, at + "off screen is nothing");
    // Every point: whatever it hits, it is inside that thing.
    for (int y = 0; y < h; y += 3) {
      for (int x = 0; x < w; x += 3) {
        const Hit hit = page_hit(ui, kinds, n, x, y);
        if (hit.part == Part::Back) {
          check(in_box(back, x, y), at + "Back hit outside Back");
        } else if (hit.part != Part::None) {
          const Box row = row_box(ui, hit.row, n);
          check(in_box(row_part_box(ui, row, hit.part), x, y), at + "hit outside its part");
        }
      }
    }
  }
}

bool stamp_eq(YmdHms a, YmdHms b) {
  return a.year == b.year && a.month == b.month && a.date == b.date &&
         a.hour == b.hour && a.minute == b.minute && a.second == b.second;
}

}  // namespace

int main() {
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(6, 59, 0, false, 30);
    Step got = step_alarm(Occurrence::Armed, alarm, in);
    check(step_is(got, Occurrence::Armed, Intensity::Silent, false),
          "Armed, 7:00 enabled, now 6:59, second 0, no touch, skip false -> Armed, Silent, skip false");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 0, false, 30);
    Step got = step_alarm(Occurrence::Armed, alarm, in);
    check(step_is(got, Occurrence::Ringing, Intensity::Gentle, false),
          "Armed, enabled, 7:00, second 0, skip false -> Ringing, Gentle, skip false");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 30, false, 30);
    Step got = step_alarm(Occurrence::Armed, alarm, in);
    check(step_is(got, Occurrence::Ringing, Intensity::Loud, false),
          "Armed, enabled, 7:00, second 30, skip false -> Ringing, Loud, skip false");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 29, false, 30);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(got.intensity == Intensity::Gentle,
          "Ringing, second 29, no touch -> Gentle");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 30, false, 30);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(got.intensity == Intensity::Loud,
          "Ringing, second 30, no touch -> Loud");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 0, true, 30);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(step_is(got, Occurrence::Silenced, Intensity::Silent, false),
          "Ringing, touched -> Silenced, Silent");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 0, false, 30);
    Step got = step_alarm(Occurrence::Silenced, alarm, in);
    check(got.occurrence == Occurrence::Silenced,
          "Silenced at 7:00 -> Silenced");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 1, 0, false, 30);
    Step silenced = step_alarm(Occurrence::Silenced, alarm, in);
    Step ringing = step_alarm(Occurrence::Ringing, alarm, in);
    Step skipped = step_alarm(Occurrence::Skipped, alarm, in);
    check(silenced.occurrence == Occurrence::Armed &&
              ringing.occurrence == Occurrence::Armed &&
              skipped.occurrence == Occurrence::Armed,
          "Ringing or Silenced or Skipped at 7:01 -> Armed");
  }
  {
    Alarm alarm = {{7, 0}, false, false};
    Inputs in = in_at(7, 0, 0, false, 30);
    Step got = step_alarm(Occurrence::Armed, alarm, in);
    check(step_is(got, Occurrence::Armed, Intensity::Silent, false),
          "Armed, disabled, at 7:00 -> Armed, Silent");
  }
  {
    Alarm alarm = {{7, 0}, true, true};
    Inputs in = in_at(7, 0, 0, false, 30);
    Step got = step_alarm(Occurrence::Armed, alarm, in);
    check(step_is(got, Occurrence::Skipped, Intensity::Silent, false),
          "Armed, enabled, at 7:00, skip true -> Skipped, Silent, skip false");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 0, false, 30);
    Step got = step_alarm(Occurrence::Skipped, alarm, in);
    check(got.occurrence == Occurrence::Skipped,
          "Skipped at 7:00, skip false -> stay Skipped");
  }
  {
    Alarm alarm = {{7, 0}, true, true};
    Inputs in = in_at(7, 1, 0, false, 30);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(got.occurrence == Occurrence::Armed && got.skip_next == true,
          "Ringing at 7:01 with skip true -> Armed, skip still true");
  }
  {
    Alarm alarm = {{7, 0}, false, false};
    Inputs in = in_at(7, 0, 0, false, 30);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(step_is(got, Occurrence::Armed, Intensity::Silent, false),
          "Ringing, enabled false, still 7:00, gentle 30 -> Armed, Silent");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 0, false, 0);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(got.intensity == Intensity::Loud,
          "Ringing, second 0, gentle_seconds 0 -> Loud");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 9, false, 10);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(got.intensity == Intensity::Gentle,
          "Ringing, second 9, gentle_seconds 10 -> Gentle");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs in = in_at(7, 0, 10, false, 10);
    Step got = step_alarm(Occurrence::Ringing, alarm, in);
    check(got.intensity == Intensity::Loud,
          "Ringing, second 10, gentle_seconds 10 -> Loud");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Alarm got = apply_zone(alarm, Zone::Hour);
    check(alarm_eq(got, Alarm{{8, 0}, true, false}),
          "apply_zone hour 7 -> 8");
  }
  {
    Alarm alarm = {{23, 0}, true, false};
    Alarm got = apply_zone(alarm, Zone::Hour);
    check(alarm_eq(got, Alarm{{0, 0}, true, false}),
          "apply_zone hour 23 -> 0");
  }
  {
    Alarm alarm = {{7, 59}, true, false};
    Alarm got = apply_zone(alarm, Zone::Minute);
    check(alarm_eq(got, Alarm{{7, 0}, true, false}),
          "apply_zone minute 59 -> 0");
  }
  {
    Alarm alarm = {{0, 0}, true, false};
    Alarm got = apply_zone(alarm, Zone::HourDown);
    check(alarm_eq(got, Alarm{{23, 0}, true, false}),
          "apply_zone hour down 0 -> 23");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Alarm got = apply_zone(alarm, Zone::MinuteDown);
    check(alarm_eq(got, Alarm{{7, 59}, true, false}),
          "apply_zone minute down 0 -> 59");
  }
  {
    Alarm alarm = {{7, 0}, false, false};
    Alarm got = apply_zone(alarm, Zone::Toggle);
    check(alarm_eq(got, Alarm{{7, 0}, true, false}),
          "apply_zone toggle flips enabled");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Alarm got = apply_zone(alarm, Zone::Clock);
    check(alarm_eq(got, alarm), "apply_zone clock zone unchanged");
  }
  {
    check(clamp_soft(40, 80) == 40, "clamp_soft(40, 80) == 40");
    check(clamp_soft(90, 80) == 80, "clamp_soft(90, 80) == 80");
    check(clamp_soft(-5, 80) == 0, "clamp_soft(-5, 80) == 0");
    check(adjust_volume(80, 5) == 85, "adjust_volume(80, 5) == 85");
    check(adjust_volume(98, 5) == 100, "adjust_volume(98, 5) == 100");
    check(adjust_volume(0, -5) == 0, "adjust_volume(0, -5) == 0");
  }
  {
    check(adjust_gentle(30, 5) == 35, "adjust_gentle(30, 5) == 35");
    check(adjust_gentle(60, 5) == 60, "adjust_gentle(60, 5) == 60");
    check(adjust_gentle(0, -5) == 0, "adjust_gentle(0, -5) == 0");
  }
  {
    check(speaker_level(0) == 0, "speaker_level(0) == 0");
    check(speaker_level(100) == 255, "speaker_level(100) == 255");
    check(speaker_level(80) == 204, "speaker_level(80) == 204");
  }
  {
    char buf[16];
    format_time(buf, sizeof(buf), TimeFormat::Hidden, 9, 5, 7);
    check(std::strcmp(buf, "") == 0, "format_time Hidden -> \"\"");
  }
  {
    char buf[16];
    format_time(buf, sizeof(buf), TimeFormat::Hours, 9, 5, 7);
    check(std::strcmp(buf, "09") == 0, "format_time Hours 9 -> \"09\"");
  }
  {
    char buf[16];
    format_time(buf, sizeof(buf), TimeFormat::HoursMinutes, 9, 5, 7);
    check(std::strcmp(buf, "09:05") == 0,
          "format_time HoursMinutes 9, 5 -> \"09:05\"");
  }
  {
    char buf[16];
    format_time(buf, sizeof(buf), TimeFormat::HoursMinutesSeconds, 9, 5, 7);
    check(std::strcmp(buf, "09:05:07") == 0,
          "format_time HoursMinutesSeconds 9, 5, 7 -> \"09:05:07\"");
  }
  {
    char buf[16];
    format_time(buf, sizeof(buf), TimeFormat::HoursMinutesSeconds, 0, 5, 7,
                HourCycle::H12);
    check(std::strcmp(buf, "12:05:07 AM") == 0, "format_time midnight AM");
    format_time(buf, sizeof(buf), TimeFormat::HoursMinutes, 12, 0, 0,
                HourCycle::H12);
    check(std::strcmp(buf, "12:00 PM") == 0, "format_time noon PM");
    format_time(buf, sizeof(buf), TimeFormat::Hours, 15, 0, 0, HourCycle::H12);
    check(std::strcmp(buf, "3 PM") == 0, "format_time 15 -> 3 PM");
    format_time(buf, sizeof(buf), TimeFormat::Hidden, 15, 0, 0, HourCycle::H12);
    check(std::strcmp(buf, "") == 0, "format_time hidden stays blank");
    char digits[16];
    char period[4];
    format_clock_face(digits, sizeof(digits), period, sizeof(period),
                      TimeFormat::HoursMinutesSeconds, 15, 5, 7, HourCycle::H12);
    check(std::strcmp(digits, "3:05:07") == 0 && std::strcmp(period, "PM") == 0,
          "format_clock_face splits PM");
    format_clock_face(digits, sizeof(digits), period, sizeof(period),
                      TimeFormat::HoursMinutes, 9, 5, 0, HourCycle::H24);
    check(std::strcmp(digits, "09:05") == 0 && period[0] == '\0',
          "format_clock_face 24h has no period");
    check(next_hour_cycle(HourCycle::H24) == HourCycle::H12, "next hour 12");
    check(next_hour_cycle(HourCycle::H12) == HourCycle::H24, "next hour 24");
  }
  {
    check(preview_playing(0), "preview_playing(0) true");
    check(preview_playing(1999), "preview_playing(1999) true");
    check(!preview_playing(2000), "preview_playing(2000) false");
  }
  {
    check(next_format(TimeFormat::Hidden) == TimeFormat::Hours,
          "next_format Hidden -> Hours");
    check(next_format(TimeFormat::HoursMinutesSeconds) == TimeFormat::Hidden,
          "next_format H:M:S -> Hidden");
  }
  {
    Alarm alarm = {{7, 0}, true, false};
    Inputs snooze_in = {{7, 0}, 10, false, 30, true, false, {0, 0}};
    Step snoozed = step_alarm(Occurrence::Ringing, alarm, snooze_in);
    check(snoozed.occurrence == Occurrence::Silenced && snoozed.snooze_armed &&
              snoozed.snooze_at.hour == 7 && snoozed.snooze_at.minute == 9,
          "Ringing snooze at 7:00 -> silenced until 7:09");
    Inputs later = {{7, 1}, 0, false, 30, false, snoozed.snooze_armed,
                    snoozed.snooze_at};
    Step waiting = step_alarm(Occurrence::Silenced, alarm, later);
    check(waiting.occurrence == Occurrence::Armed && waiting.snooze_armed &&
              waiting.snooze_at.minute == 9,
          "Snoozed minute rolls to 7:01 -> Armed, snooze kept");
    Inputs due = {{7, 9}, 0, false, 30, false, waiting.snooze_armed,
                  waiting.snooze_at};
    Step again = step_alarm(Occurrence::Armed, alarm, due);
    check(again.occurrence == Occurrence::Ringing &&
              again.intensity == Intensity::Gentle,
          "Snooze 7:09 -> Ringing Gentle");
    Inputs stop = {{7, 9}, 0, true, 30, false, again.snooze_armed,
                   again.snooze_at};
    Step stopped = step_alarm(Occurrence::Ringing, alarm, stop);
    check(stopped.occurrence == Occurrence::Silenced && !stopped.snooze_armed,
          "Stop during snooze ring clears snooze");
  }
  {
    Settings ok = default_settings();
    check(settings_valid(ok), "settings_valid defaults");
    ok.magic = 1;
    check(!settings_valid(ok), "settings_valid rejects bad magic");
    ok.magic = kSettingsMagic;
    ok.soft_volume = 90;
    check(!settings_valid(ok), "settings_valid rejects soft above loud");
    ok = default_settings();
    ok.noise_timer = 17;
    check(!settings_valid(ok), "settings_valid rejects a timer not on the list");
    ok = default_settings();
    ok.lamp_bright = 0;
    check(!settings_valid(ok), "settings_valid rejects lamp brightness 0");
    ok = default_settings();
    ok.wake_after = 7;
    check(!settings_valid(ok), "settings_valid rejects wake minutes not on the list");
    ok = default_settings();
    ok.winddown_end_minute = 60;
    check(!settings_valid(ok), "settings_valid rejects a bad lights-out time");
    ok = default_settings();
    ok.noise_kind = 3;
    check(!settings_valid(ok), "settings_valid rejects an unknown sound");
  }
  {
    YmdHms got = parse_compile_stamp("Sep 22 2026", "18:05:09");
    check(stamp_eq(got, YmdHms{2026, 9, 22, 18, 5, 9}),
          "parse_compile_stamp(\"Sep 22 2026\", \"18:05:09\") -> 2026-09-22 18:05:09");
  }
  {
    YmdHms got = parse_compile_stamp("Sep  2 2026", "00:00:00");
    check(stamp_eq(got, YmdHms{2026, 9, 2, 0, 0, 0}),
          "parse_compile_stamp(\"Sep  2 2026\", \"00:00:00\") -> 2026-09-02 00:00:00");
  }
  {
    check(rtc_needs_set(2025) == true && rtc_needs_set(2026) == false,
          "rtc_needs_set(2025) true, rtc_needs_set(2026) false");
  }
  {
    const Hm start = {21, 0};
    const Hm end = {7, 0};
    check(is_night(Hm{21, 0}, start, end), "is_night 21:00 in 21:00-07:00");
    check(is_night(Hm{23, 30}, start, end), "is_night 23:30 in 21:00-07:00");
    check(is_night(Hm{6, 59}, start, end), "is_night 06:59 in 21:00-07:00");
    check(!is_night(Hm{7, 0}, start, end), "is_night 07:00 not in 21:00-07:00");
    check(!is_night(Hm{12, 0}, start, end), "is_night 12:00 not in 21:00-07:00");
    check(!is_night(Hm{20, 59}, start, end),
          "is_night 20:59 not in 21:00-07:00");
    check(is_night(Hm{10, 0}, Hm{9, 0}, Hm{17, 0}),
          "is_night 10:00 in 09:00-17:00");
    check(!is_night(Hm{8, 0}, Hm{9, 0}, Hm{17, 0}),
          "is_night 08:00 not in 09:00-17:00");
    check(!is_night(Hm{12, 0}, Hm{12, 0}, Hm{12, 0}),
          "is_night equal start and end is day");
  }
  {
    const Hm rolled = step_minutes(Hm{23, 30}, 30);
    check(rolled.hour == 0 && rolled.minute == 0, "step_minutes 23:30 +30");
    const Hm back = step_minutes(Hm{0, 0}, -30);
    check(back.hour == 23 && back.minute == 30, "step_minutes 00:00 -30");
  }
  {
    check(adjust_brightness(20, -10) == 10, "adjust_brightness 20-10");
    check(adjust_brightness(10, -10) == 10, "adjust_brightness floor 10");
    check(adjust_brightness(15, 10) == 30, "adjust_brightness snaps then steps 10");
    check(adjust_brightness(100, 10) == 100, "adjust_brightness ceiling 100");
    check(snap_brightness(5) == 10, "snap_brightness 5");
    check(snap_brightness(15) == 20, "snap_brightness 15");
    check(backlight_level(100) == 255, "backlight_level 100");
    check(backlight_level(10) == 1, "backlight_level 10");
    check(backlight_level(0) == 1, "backlight_level 0 clamps to 10");
    check(backlight_level(20) == 29, "backlight_level 20");
  }
  {
    check(next_color_mode(ColorMode::Auto) == ColorMode::Day,
          "next_color_mode Auto -> Day");
    check(next_color_mode(ColorMode::Night) == ColorMode::Auto,
          "next_color_mode Night -> Auto");
  }

  {
    // Layout at the clock's screens, and a sweep of others
    check_layout(320, 240);
    check_layout(616, 284);
    check_layout(600, 450);
    check_layout(1232, 568);
    for (int w = 160; w <= 1400; w += 97) {
      for (int h = 120; h <= 900; h += 71) {
        check_layout(w, h);
      }
    }
    const Ui core = ui_for(320, 240);
    check(core.scale == 100 && core.header == 40, "CoreS3 is the 100% layout");
    const Ui big = ui_for(600, 450);
    check(big.scale > 180 && big.header > 70, "600x450 scales up");
    const Ui zero = ui_for(0, 0);
    check(zero.w == 1 && zero.h == 1, "ui_for survives a zero screen");
    check(tile_columns(ui_for(616, 284)) == 3 && tile_columns(ui_for(320, 240)) == 2,
          "wide screens get three tile columns");
  }
  {
    // Screens and navigation
    check(parent_screen(Screen::NightHours) == Screen::NightScreen, "Night hours backs to Night screen");
    check(parent_screen(Screen::Lamp) == Screen::Settings, "pages back to Settings");
    check(parent_screen(Screen::Settings) == Screen::Clock, "Settings backs to the clock");
    for (int i = 0; i < kMenuItems; ++i) {
      RowKind kinds[kMaxRows];
      check(page_rows(menu_item(i), kinds) > 0, "every menu item opens a page");
      for (int j = i + 1; j < kMenuItems; ++j) {
        check(menu_item(i) != menu_item(j), "menu items differ");
      }
    }
    check(screen_preview(Screen::DayScreen) == 1 && screen_preview(Screen::NightHours) == 2 &&
              screen_preview(Screen::Lamp) == 0,
          "day and night pages preview their look");
  }
  {
    // Sleep sounds
    const NoiseKind kinds[] = {NoiseKind::White, NoiseKind::Pink, NoiseKind::Brown};
    double ratio[3];
    for (int k = 0; k < 3; ++k) {
      NoiseGen g = noise_gen(99);
      double sum = 0, sq = 0, diff = 0, prev = 0;
      const int n = 200000;
      for (int i = 0; i < n; ++i) {
        const double v = noise_value(g, kinds[k]);
        sum += v;
        sq += v * v;
        if (i) diff += (v - prev) * (v - prev);
        prev = v;
        check(v > -1.0 && v < 1.0, "noise stays in range");
      }
      const double rms = std::sqrt(sq / n);
      check(std::fabs(sum / n) < 0.02, std::string(noise_key(kinds[k])) + " noise is centred");
      check(rms > 0.12 && rms < 0.2, std::string(noise_key(kinds[k])) + " noise at the common level");
      ratio[k] = diff / sq;
    }
    check(ratio[0] > 1.8, "white noise is flat");
    check(ratio[1] < 0.6 * ratio[0] && ratio[1] > 0.1, "pink noise leans low");
    check(ratio[2] < 0.3 * ratio[1], "brown noise leans lower still");
    NoiseGen a = noise_gen(5), b = noise_gen(5);
    int16_t x[64], y[64];
    fill_noise(a, NoiseKind::Pink, x, 64, 1000, 1000);
    fill_noise(b, NoiseKind::Pink, y, 64, 1000, 1000);
    check(std::memcmp(x, y, sizeof(x)) == 0, "same seed, same noise");
    NoiseGen z = noise_gen(0);
    check(z.seed != 0, "a zero seed still makes noise");
    fill_noise(a, NoiseKind::White, x, 64, 0, 0);
    bool silent = true;
    for (int16_t v : x) silent = silent && v == 0;
    check(silent, "gain 0 is silence");
    fill_noise(a, NoiseKind::White, x, 64, 0, 1000);
    check(x[0] == 0 && std::abs(static_cast<int>(x[1])) < 1000, "a ramp starts from silence");
    int16_t one;
    fill_noise(a, NoiseKind::Brown, &one, 1, 0, 1000);
    (void)one;

    check(noise_gain_permille(0, 0) == 0, "fades in from 0");
    check(noise_gain_permille(1500, 0) == 500, "halfway through the fade in");
    check(noise_gain_permille(3000, 0) == 1000, "full after 3 s");
    check(noise_gain_permille(4000000000u, 0) == 1000, "no timer plays forever");
    check(noise_gain_permille(14 * 60000, 15) == 1000, "full until the last minute");
    check(noise_gain_permille(14 * 60000 + 30000, 15) == 500, "half, 30 s from the end");
    check(noise_gain_permille(15 * 60000, 15) == 0, "silent at the end");
    check(noise_gain_permille(1000, 120) == 333, "the long timer doesn't overflow");
    check(!noise_timer_done(15 * 60000 - 1, 15) && noise_timer_done(15 * 60000, 15),
          "timer done at the end");
    check(!noise_timer_done(4000000000u, 0), "no timer is never done");
    check(noise_minutes_left(0, 15) == 15 && noise_minutes_left(1, 15) == 15 &&
              noise_minutes_left(60000, 15) == 14 && noise_minutes_left(15 * 60000, 15) == 0 &&
              noise_minutes_left(5, 0) == 0,
          "minutes left round up");
    check(step_timer(0, -1) == 0 && step_timer(0, 1) == 15 && step_timer(120, 1) == 120 &&
              step_timer(60, 1) == 90 && step_timer(17, 1) == 30 && step_timer(17, 0) == 15,
          "timer steps");
    check(step_wake(0, 1) == 10 && step_wake(60, 1) == 60 && step_wake(10, -1) == 0,
          "wake-up light steps");
    check(adjust_level(30, 5) == 35 && adjust_level(5, -5) == 5 && adjust_level(100, 5) == 100 &&
              adjust_level(33, 5) == 40 && adjust_level(0, 0) == 5,
          "levels step by 5 between 5 and 100");
    check(std::strcmp(noise_label(next_noise(NoiseKind::Brown)), "White noise") == 0,
          "sounds cycle");
  }
  {
    // Lamp schedule: wake up
    const Alarm on = {{7, 0}, true, false};
    check(wake_ramp_permille(Hm{6, 39}, 59, on, 20, 100) == 0, "dark before the ramp");
    check(wake_ramp_permille(Hm{6, 40}, 0, on, 20, 100) == 0, "the ramp starts at 0");
    check(wake_ramp_permille(Hm{6, 50}, 0, on, 20, 100) == 500, "half way");
    check(wake_ramp_permille(Hm{6, 50}, 0, on, 20, 60) == 300, "half way to 60%");
    check(wake_ramp_permille(Hm{6, 59}, 59, on, 20, 100) == 999, "almost there");
    check(wake_ramp_permille(Hm{7, 0}, 30, on, 20, 80) == 800, "at its brightness through the alarm minute");
    check(wake_ramp_permille(Hm{7, 1}, 0, on, 20, 100) == 0, "the ramp is before the alarm only");
    check(wake_ramp_permille(Hm{6, 50}, 0, on, 0, 100) == 0, "no ramp with 0 minutes before");
    check(wake_ramp_permille(Hm{6, 50}, 0, Alarm{{7, 0}, false, false}, 20, 100) == 0, "none with the alarm off");
    check(wake_ramp_permille(Hm{6, 50}, 0, Alarm{{7, 0}, true, true}, 20, 100) == 0, "none before a skipped alarm");
    check(wake_ramp_permille(Hm{23, 55}, 0, Alarm{{0, 10}, true, false}, 20, 100) == 250, "across midnight");
    int last = -1;
    bool rising = true;
    for (int s = 0; s < 20 * 60; ++s) {
      const int v = wake_ramp_permille(step_minutes(Hm{6, 40}, s / 60), s % 60, on, 20, 100);
      rising = rising && v >= last && v <= 1000;
      last = v;
    }
    check(rising, "the wake-up light only brightens");
    check(!wake_light_expired(Hm{7, 19}, Hm{7, 0}, 20) && wake_light_expired(Hm{7, 20}, Hm{7, 0}, 20),
          "the light stays on for the minutes after the alarm");
    check(wake_light_expired(Hm{7, 0}, Hm{7, 0}, 0), "none after with 0 minutes");
    check(wake_light_expired(Hm{6, 0}, Hm{7, 0}, 30), "a clock set back ends it");

    // Lamp schedule: wind down, 21:30 to lights out at 22:30 at 40%
    const Hm ws = {21, 30}, we = {22, 30};
    check(winddown_permille(Hm{21, 29}, 59, ws, we, 40) == 0, "off before wind down");
    check(winddown_permille(Hm{21, 30}, 0, ws, we, 40) == 400, "on at its brightness from the start");
    check(winddown_permille(Hm{22, 14}, 59, ws, we, 40) == 400, "steady until the last 15 minutes");
    check(winddown_permille(Hm{22, 22}, 30, ws, we, 40) == 200, "halfway through the fade");
    check(winddown_permille(Hm{22, 29}, 30, ws, we, 40) == 13, "a glimmer 30 s before lights out");
    check(winddown_permille(Hm{22, 30}, 0, ws, we, 40) == 0, "off at lights out");
    check(winddown_permille(Hm{3, 0}, 0, ws, we, 40) == 0, "off overnight");
    check(winddown_permille(Hm{0, 10}, 0, Hm{23, 30}, Hm{0, 30}, 50) == 500, "across midnight");
    check(winddown_permille(Hm{0, 25}, 0, Hm{23, 30}, Hm{0, 30}, 50) > 0 &&
              winddown_permille(Hm{0, 25}, 0, Hm{23, 30}, Hm{0, 30}, 50) < 500,
          "fading across midnight");
    check(winddown_permille(Hm{21, 35}, 0, ws, ws, 40) == 0, "nothing when start and lights out match");
    check(winddown_permille(Hm{21, 34}, 0, Hm{21, 30}, Hm{21, 40}, 40) == 400 &&
              winddown_permille(Hm{21, 37}, 30, Hm{21, 30}, Hm{21, 40}, 40) == 200,
          "a short window fades over its second half");
    bool falling = true;
    last = 1001;
    for (int s = 0; s < 60 * 60; ++s) {
      const int v = winddown_permille(step_minutes(ws, s / 60), s % 60, ws, we, 40);
      falling = falling && v <= last;
      last = v;
    }
    check(falling, "wind down only dims");

    check(lamp_permille(false, 40, 0, 0) == 0, "off");
    check(lamp_permille(true, 40, 0, 0) == 400, "on at 40%");
    check(lamp_permille(true, 40, 700, 0) == 700, "a brighter schedule wins");
    check(lamp_permille(true, 40, 200, 0) == 400, "a dimmer schedule doesn't dim the switch");
    check(lamp_permille(false, 40, 0, 1000) == 1000, "the after-alarm light");
    check(lamp_permille(true, 250, 0, 0) == 1000, "clamped");
    check(lamp_duty(0) == 0 && lamp_duty(1) == 1 && lamp_duty(1000) == 255 && lamp_duty(-5) == 0 &&
              lamp_duty(2000) == 255,
          "lamp duty ends");
    bool mono = true;
    for (int p = 1; p <= 1000; ++p) mono = mono && lamp_duty(p) >= lamp_duty(p - 1);
    check(mono, "lamp duty only rises");
    check(lamp_duty(100) < 10, "the dim end has fine steps");
    check(button_action(true, true) == ButtonAction::Snooze, "button snoozes a ringing alarm");
    check(button_action(false, true) == ButtonAction::LampOff, "button puts a lit lamp out");
    check(button_action(false, false) == ButtonAction::LampOn, "button lights the lamp");
  }
  {
    // Serial commands
    struct Case {
      const char* line;
      CmdKind kind;
      int a, b, c;
    };
    const Case cases[] = {
        {"", CmdKind::None, 0, 0, 0},
        {"   ", CmdKind::None, 0, 0, 0},
        {"help", CmdKind::Help, 0, 0, 0},
        {"STATE", CmdKind::State, 0, 0, 0},
        {"  lamp   on ", CmdKind::LampOn, 0, 0, 0},
        {"lamp off", CmdKind::LampOff, 0, 0, 0},
        {"lamp toggle", CmdKind::LampToggle, 0, 0, 0},
        {"lamp 40", CmdKind::LampLevel, 40, 0, 0},
        {"lamp 0", CmdKind::LampOff, 0, 0, 0},
        {"lamp 100", CmdKind::LampLevel, 100, 0, 0},
        {"lamp 101", CmdKind::Unknown, 0, 0, 0},
        {"lamp 4000", CmdKind::Unknown, 0, 0, 0},
        {"lamp", CmdKind::Unknown, 0, 0, 0},
        {"lampon", CmdKind::Unknown, 0, 0, 0},
        {"sound pink", CmdKind::SoundKind, 1, 0, 0},
        {"sound Brown", CmdKind::SoundKind, 2, 0, 0},
        {"sound white", CmdKind::SoundKind, 0, 0, 0},
        {"sound on", CmdKind::SoundOn, 0, 0, 0},
        {"sound off", CmdKind::SoundOff, 0, 0, 0},
        {"sound toggle", CmdKind::SoundToggle, 0, 0, 0},
        {"sound grey", CmdKind::Unknown, 0, 0, 0},
        {"time 7:05", CmdKind::Time, 7, 5, 0},
        {"time 23:59:58", CmdKind::Time, 23, 59, 58},
        {"time 24:00", CmdKind::Unknown, 0, 0, 0},
        {"time 12:60", CmdKind::Unknown, 0, 0, 0},
        {"time 12:5", CmdKind::Unknown, 0, 0, 0},
        {"time 12:05:61", CmdKind::Unknown, 0, 0, 0},
        {"time 12:05 x", CmdKind::Unknown, 0, 0, 0},
        {"alarm 06:30", CmdKind::AlarmAt, 6, 30, 0},
        {"alarm 06:30:10", CmdKind::Unknown, 0, 0, 0},
        {"alarm on", CmdKind::AlarmOn, 0, 0, 0},
        {"alarm off", CmdKind::AlarmOff, 0, 0, 0},
        {"go day", CmdKind::GoDay, 0, 0, 0},
        {"go night", CmdKind::GoNight, 0, 0, 0},
        {"go alarm", CmdKind::GoAlarm, 0, 0, 0},
        {"go sunrise", CmdKind::GoSunrise, 0, 0, 0},
        {"go", CmdKind::Unknown, 0, 0, 0},
        {"go home", CmdKind::Unknown, 0, 0, 0},
        {"button", CmdKind::Button, 0, 0, 0},
        {"button twice", CmdKind::Unknown, 0, 0, 0},
        {"reboot", CmdKind::Unknown, 0, 0, 0},
        {"go winddown", CmdKind::GoWindDown, 0, 0, 0},
        {"winddown on", CmdKind::WindDownOn, 0, 0, 0},
        {"winddown off", CmdKind::WindDownOff, 0, 0, 0},
        {"winddown 21:30 22:45", CmdKind::WindDownAt, 21 * 60 + 30, 22 * 60 + 45, 0},
        {"winddown 9:05 0:30", CmdKind::WindDownAt, 9 * 60 + 5, 30, 0},
        {"winddown 21:30", CmdKind::Unknown, 0, 0, 0},
        {"winddown 24:00 01:00", CmdKind::Unknown, 0, 0, 0},
        {"winddown 21:60 22:00", CmdKind::Unknown, 0, 0, 0},
        {"wake on", CmdKind::WakeOn, 0, 0, 0},
        {"wake off", CmdKind::WakeOff, 0, 0, 0},
        {"wake 30 20", CmdKind::WakeTimes, 30, 20, 0},
        {"wake 0 45", CmdKind::WakeTimes, 0, 45, 0},
        {"wake 25 20", CmdKind::Unknown, 0, 0, 0},
        {"wake 30", CmdKind::Unknown, 0, 0, 0},
    };
    for (const Case& c : cases) {
      const Command got = parse_command(c.line);
      const bool args = c.kind == CmdKind::Unknown || c.kind == CmdKind::None ||
                        (got.a == c.a && got.b == c.b && got.c == c.c);
      check(got.kind == c.kind && args, std::string("parse_command(\"") + c.line + "\")");
    }
    LineReader r = {};
    const char* feed = "lamp on\r\n\nsound off\n";
    std::string lines;
    for (const char* p = feed; *p; ++p) {
      if (line_push(r, *p)) lines += std::string(r.buf) + "|";
    }
    check(lines == "lamp on|sound off|", "lines split on CR and LF, empty lines skipped: " + lines);
    for (int i = 0; i < 200; ++i) line_push(r, 'x');
    check(!line_push(r, '\n'), "an overlong line is dropped");
    for (const char* p = "state"; *p; ++p) line_push(r, *p);
    check(line_push(r, '\n') && std::strcmp(r.buf, "state") == 0, "and the next line works");
  }
  {
    // Short times and labels
    char buf[16];
    format_hm_short(buf, sizeof(buf), Hm{21, 0}, HourCycle::H12);
    check(std::strcmp(buf, "9PM") == 0, "format_hm_short 9PM");
    format_hm_short(buf, sizeof(buf), Hm{0, 30}, HourCycle::H12);
    check(std::strcmp(buf, "12:30AM") == 0, "format_hm_short 12:30AM");
    format_hm_short(buf, sizeof(buf), Hm{7, 5}, HourCycle::H24);
    check(std::strcmp(buf, "07:05") == 0, "format_hm_short 07:05");
    check(minutes_until(Hm{23, 0}, Hm{1, 0}) == 120 && minutes_until(Hm{7, 0}, Hm{7, 0}) == 0,
          "minutes_until wraps");
  }
  {
    // Loading saved settings, including version 1 from earlier firmware
    Settings s = default_settings();
    s.alarm_hour = 5;
    Settings out;
    check(load_settings(&s, sizeof(s), &out) && out.alarm_hour == 5, "loads version 2");
    check(!load_settings(&s, sizeof(s) - 1, &out), "rejects a short blob");
    check(!load_settings(&s, 0, &out), "rejects nothing");
    SettingsV1 v1;
    std::memset(&v1, 0, sizeof(v1));
    v1.magic = kSettingsMagic;
    v1.version = 1;
    v1.alarm_hour = 6;
    v1.alarm_minute = 45;
    v1.alarm_enabled = 1;
    v1.volume = 90;
    v1.soft_volume = 30;
    v1.gentle_seconds = 20;
    v1.day_format = 3;
    v1.night_format = 1;
    v1.color_mode = 2;
    v1.night_start_hour = 22;
    v1.night_end_hour = 6;
    v1.day_bright = 80;
    v1.night_bright = 10;
    v1.hour12 = 1;
    check(load_settings(&v1, sizeof(v1), &out), "upgrades version 1");
    check(out.version == kSettingsVersion && out.alarm_hour == 6 && out.alarm_minute == 45 &&
              out.volume == 90 && out.night_format == 1 && out.color_mode == 2 && out.hour12 == 1 &&
              out.night_bright == 10,
          "version 1 values carried over");
    check(out.noise_volume == default_settings().noise_volume && out.lamp_on == 0,
          "new values get defaults");
    v1.alarm_hour = 30;
    check(!load_settings(&v1, sizeof(v1), &out), "a bad version 1 blob is rejected");
    v1.alarm_hour = 6;
    v1.version = 7;
    check(!load_settings(&v1, sizeof(v1), &out), "an unknown version is rejected");
    SettingsV2 v2;
    std::memset(&v2, 0, sizeof(v2));
    Settings base = default_settings();
    base.alarm_hour = 5;
    base.lamp_bright = 65;
    std::memcpy(&v2, &base, sizeof(v2));  // the first 29 bytes are the same
    v2.version = 2;
    v2.sunrise_minutes = 20;
    std::memset(v2.reserved, 0, sizeof(v2.reserved));
    check(load_settings(&v2, sizeof(v2), &out), "upgrades version 2");
    check(out.alarm_hour == 5 && out.lamp_bright == 65 && out.noise_kind == base.noise_kind,
          "version 2 values carried over");
    check(out.wake_on == 1 && out.wake_before == 20 && out.wake_after == 30 && out.wake_bright == 100,
          "version 2's sunrise becomes the wake-up light");
    check(out.winddown_on == 0, "wind down starts off");
    v2.sunrise_minutes = 0;
    check(load_settings(&v2, sizeof(v2), &out) && out.wake_on == 0 && out.wake_before == 30,
          "no sunrise, no wake-up light");
    v2.sunrise_minutes = 17;
    check(!load_settings(&v2, sizeof(v2), &out), "a bad version 2 blob is rejected");
    const Alarm a = alarm_of(default_settings());
    check(a.at.hour == 7 && a.at.minute == 0 && !a.enabled && !a.skip_next, "alarm_of defaults");
  }

  return failures;
}
