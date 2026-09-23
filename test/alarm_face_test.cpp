#include "alarm_face.hpp"

#include <cstdio>
#include <cstring>

namespace {

int failures = 0;

void check(bool ok, const char* name) {
  if (!ok) {
    std::fprintf(stderr, "%s\n", name);
    failures = 1;
  }
}

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
    check(preview_playing(0), "preview_playing(0) true");
    check(preview_playing(1999), "preview_playing(1999) true");
    check(!preview_playing(2000), "preview_playing(2000) false");
  }
  {
    check(clock_hit(290, 20, 320, 240) == ClockHit::Gear,
          "clock_hit(290, 20, 320, 240) Gear");
    check(clock_hit(250, 20, 320, 240) == ClockHit::Bell,
          "clock_hit(250, 20, 320, 240) Bell");
    check(clock_hit(200, 20, 320, 240) == ClockHit::Skip,
          "clock_hit(200, 20, 320, 240) Skip");
    check(clock_hit(160, 20, 320, 240) == ClockHit::Face,
          "clock_hit(160, 20, 320, 240) Face");
    check(clock_hit(290, 60, 320, 240) == ClockHit::Face,
          "clock_hit(290, 60, 320, 240) Face");
  }
  {
    const int w = 320;
    const int h = 240;
    check(menu_hit(20, 30, w, h) == MenuHit::Back, "menu_hit (20, 30) Back");
    check(menu_hit(200, 30, w, h) == MenuHit::None, "menu_hit (200, 30) None");
    check(menu_hit(30, 52, w, h) == MenuHit::None, "menu_hit (30, 52) None");
    check(menu_hit(160, 72, w, h) == MenuHit::Alarm, "menu_hit (160, 72) Alarm");
    check(menu_hit(160, 90, w, h) == MenuHit::None, "menu_hit (160, 90) None");
    check(menu_hit(20, 90, w, h) == MenuHit::None, "menu_hit (20, 90) None");
    check(menu_hit(160, 120, w, h) == MenuHit::Sound,
          "menu_hit (160, 120) Sound");
    check(menu_hit(160, 168, w, h) == MenuHit::ClockFace,
          "menu_hit (160, 168) ClockFace");
    check(menu_hit(160, 216, w, h) == MenuHit::Display,
          "menu_hit (160, 216) Display");
  }
  {
    const int w = 320;
    const int h = 240;
    check(alarm_page_hit(20, 40, w, h) == AlarmPageHit::Back,
          "alarm_page_hit (20, 40) Back");
    check(alarm_page_hit(160, 80, w, h) == AlarmPageHit::None,
          "alarm_page_hit (160, 80) None");
    check(alarm_page_hit(160, 120, w, h) == AlarmPageHit::Toggle,
          "alarm_page_hit (160, 120) Toggle");
    check(alarm_page_hit(20, 200, w, h) == AlarmPageHit::HourDown,
          "alarm_page_hit (20, 200) HourDown");
    check(alarm_page_hit(78, 200, w, h) == AlarmPageHit::None,
          "alarm_page_hit (78, 200) None");
    check(alarm_page_hit(300, 200, w, h) == AlarmPageHit::MinuteUp,
          "alarm_page_hit (300, 200) MinuteUp");
  }
  {
    const int w = 320;
    const int h = 240;
    check(sound_page_hit(20, 18, w, h) == SoundPageHit::Back,
          "sound_page_hit (20, 18) Back");
    check(sound_page_hit(20, 35, w, h) == SoundPageHit::None,
          "sound_page_hit (20, 35) None");
    check(sound_page_hit(20, 58, w, h) == SoundPageHit::VolumeDown,
          "sound_page_hit (20, 58) VolumeDown");
    check(sound_page_hit(160, 58, w, h) == SoundPageHit::None,
          "sound_page_hit (160, 58) None");
    check(sound_page_hit(300, 58, w, h) == SoundPageHit::VolumeUp,
          "sound_page_hit (300, 58) VolumeUp");
    check(sound_page_hit(20, 98, w, h) == SoundPageHit::SoftDown,
          "sound_page_hit (20, 98) SoftDown");
    check(sound_page_hit(300, 98, w, h) == SoundPageHit::SoftUp,
          "sound_page_hit (300, 98) SoftUp");
    check(sound_page_hit(20, 140, w, h) == SoundPageHit::GentleDown,
          "sound_page_hit (20, 140) GentleDown");
    check(sound_page_hit(300, 140, w, h) == SoundPageHit::GentleUp,
          "sound_page_hit (300, 140) GentleUp");
    check(sound_page_hit(160, 180, w, h) == SoundPageHit::PreviewSoft,
          "sound_page_hit (160, 180) PreviewSoft");
    check(sound_page_hit(160, 218, w, h) == SoundPageHit::PreviewLoud,
          "sound_page_hit (160, 218) PreviewLoud");
    check(sound_page_hit(20, 75, w, h) == SoundPageHit::None,
          "sound_page_hit (20, 75) None");
  }
  {
    const int w = 320;
    const int h = 240;
    check(clock_page_hit(20, 40, w, h) == ClockPageHit::Back,
          "clock_page_hit (20, 40) Back");
    check(clock_page_hit(200, 40, w, h) == ClockPageHit::None,
          "clock_page_hit (200, 40) None");
    check(clock_page_hit(20, 80, w, h) == ClockPageHit::None,
          "clock_page_hit (20, 80) None");
    check(clock_page_hit(160, 120, w, h) == ClockPageHit::DayFormat,
          "clock_page_hit (160, 120) DayFormat");
    check(clock_page_hit(160, 200, w, h) == ClockPageHit::NightFormat,
          "clock_page_hit (160, 200) NightFormat");
  }
  {
    check(next_format(TimeFormat::Hidden) == TimeFormat::Hours,
          "next_format Hidden -> Hours");
    check(next_format(TimeFormat::HoursMinutesSeconds) == TimeFormat::Hidden,
          "next_format H:M:S -> Hidden");
  }
  {
    const int w = 320;
    const int h = 240;
    check(ring_hit(40, 200, w, h) == RingHit::Snooze,
          "ring_hit (40, 200) Snooze");
    check(ring_hit(250, 200, w, h) == RingHit::Stop, "ring_hit (250, 200) Stop");
    check(ring_hit(160, 200, w, h) == RingHit::None,
          "ring_hit (160, 200) None");
    check(ring_hit(40, 180, w, h) == RingHit::None, "ring_hit (40, 180) None");
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
    Settings ok = {};
    ok.magic = kSettingsMagic;
    ok.version = kSettingsVersion;
    ok.alarm_hour = 7;
    ok.volume = 80;
    ok.soft_volume = 40;
    ok.gentle_seconds = 30;
    ok.day_format = 3;
    ok.night_format = 3;
    ok.night_start_hour = 21;
    ok.night_end_hour = 7;
    ok.day_bright = 70;
    ok.night_bright = 20;
    check(settings_valid(ok), "settings_valid defaults");
    ok.magic = 1;
    check(!settings_valid(ok), "settings_valid rejects bad magic");
    ok.magic = kSettingsMagic;
    ok.soft_volume = 90;
    check(!settings_valid(ok), "settings_valid rejects soft above loud");
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
    check(adjust_brightness(5, -10) == 5, "adjust_brightness floor 5");
    check(adjust_brightness(100, 10) == 100, "adjust_brightness ceiling 100");
    check(backlight_level(100) == 255, "backlight_level 100");
    check(backlight_level(5) == 12, "backlight_level 5");
    check(backlight_level(0) == 12, "backlight_level 0 clamps to 5");
  }
  {
    const int w = 320;
    const int h = 240;
    check(display_page_hit(20, 18, w, h) == DisplayPageHit::Back,
          "display_page_hit (20, 18) Back");
    check(display_page_hit(20, 35, w, h) == DisplayPageHit::None,
          "display_page_hit (20, 35) None");
    check(display_page_hit(160, 60, w, h) == DisplayPageHit::Mode,
          "display_page_hit (160, 60) Mode");
    check(display_page_hit(20, 98, w, h) == DisplayPageHit::DayDown,
          "display_page_hit (20, 98) DayDown");
    check(display_page_hit(160, 98, w, h) == DisplayPageHit::None,
          "display_page_hit (160, 98) None");
    check(display_page_hit(300, 98, w, h) == DisplayPageHit::DayUp,
          "display_page_hit (300, 98) DayUp");
    check(display_page_hit(20, 140, w, h) == DisplayPageHit::NightDown,
          "display_page_hit (20, 140) NightDown");
    check(display_page_hit(20, 180, w, h) == DisplayPageHit::StartDown,
          "display_page_hit (20, 180) StartDown");
    check(display_page_hit(300, 180, w, h) == DisplayPageHit::StartUp,
          "display_page_hit (300, 180) StartUp");
    check(display_page_hit(300, 218, w, h) == DisplayPageHit::EndUp,
          "display_page_hit (300, 218) EndUp");
    check(display_page_hit(20, 120, w, h) == DisplayPageHit::None,
          "display_page_hit (20, 120) None");
  }

  return failures;
}
