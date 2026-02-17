#include "datetime_util.h"

#include <time.h>

#include "project_config.h"

namespace {
bool is_leap_year(uint16_t year) {
  return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

uint8_t days_in_month(uint16_t year, uint8_t month) {
  static const uint8_t kDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month == 2 && is_leap_year(year)) return 29;
  return kDays[month - 1];
}

// 0=Sunday ... 6=Saturday
uint8_t weekday_for_date(uint16_t y, uint8_t m, uint8_t d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

uint8_t last_sunday_of_month(uint16_t year, uint8_t month) {
  const uint8_t last_day = days_in_month(year, month);
  const uint8_t wd = weekday_for_date(year, month, last_day);
  return last_day - wd; // if last day is Sunday (wd=0), returns last_day
}

void add_minutes(RtcDateTime* dt, int delta_minutes) {
  if (!dt || delta_minutes == 0) return;

  int total_minutes = static_cast<int>(dt->hour) * 60 + dt->minute + delta_minutes;
  int day_delta = 0;

  while (total_minutes < 0) {
    total_minutes += 1440;
    day_delta -= 1;
  }
  while (total_minutes >= 1440) {
    total_minutes -= 1440;
    day_delta += 1;
  }

  dt->hour = static_cast<uint8_t>(total_minutes / 60);
  dt->minute = static_cast<uint8_t>(total_minutes % 60);

  while (day_delta != 0) {
    if (day_delta > 0) {
      const uint8_t dim = days_in_month(dt->year, dt->month);
      if (dt->day < dim) {
        dt->day += 1;
      } else {
        dt->day = 1;
        if (dt->month < 12) {
          dt->month += 1;
        } else {
          dt->month = 1;
          dt->year += 1;
        }
      }
      dt->weekday = (dt->weekday + 1) % 7;
      day_delta -= 1;
    } else {
      if (dt->day > 1) {
        dt->day -= 1;
      } else {
        if (dt->month > 1) {
          dt->month -= 1;
        } else {
          dt->month = 12;
          dt->year -= 1;
        }
        dt->day = days_in_month(dt->year, dt->month);
      }
      dt->weekday = (dt->weekday + 6) % 7;
      day_delta += 1;
    }
  }
}

uint32_t to_epoch_utc(const RtcDateTime& utc_dt) {
  const int y = static_cast<int>(utc_dt.year);
  const int m = static_cast<int>(utc_dt.month);
  const int d = static_cast<int>(utc_dt.day);

  int days = 0;
  for (int year = 1970; year < y; ++year) {
    days += is_leap_year(static_cast<uint16_t>(year)) ? 366 : 365;
  }
  for (int month = 1; month < m; ++month) {
    days += days_in_month(static_cast<uint16_t>(y), static_cast<uint8_t>(month));
  }
  days += (d - 1);

  const int seconds =
      days * 86400 +
      static_cast<int>(utc_dt.hour) * 3600 +
      static_cast<int>(utc_dt.minute) * 60 +
      static_cast<int>(utc_dt.second);
  return static_cast<uint32_t>(seconds);
}

bool is_eu_dst_utc(const RtcDateTime& utc_dt) {
  // DST starts: last Sunday in March at 01:00 UTC
  // DST ends:   last Sunday in October at 01:00 UTC
  const uint8_t start_day = last_sunday_of_month(utc_dt.year, 3);
  const uint8_t end_day = last_sunday_of_month(utc_dt.year, 10);

  if (utc_dt.month < 3 || utc_dt.month > 10) return false;
  if (utc_dt.month > 3 && utc_dt.month < 10) return true;

  if (utc_dt.month == 3) {
    if (utc_dt.day > start_day) return true;
    if (utc_dt.day < start_day) return false;
    return utc_dt.hour >= 1;
  }

  // October
  if (utc_dt.day < end_day) return true;
  if (utc_dt.day > end_day) return false;
  return utc_dt.hour < 1;
}

bool is_eu_dst_local(const RtcDateTime& local_dt) {
  // Local rule: DST starts last Sunday in March at 02:00 local,
  // ends last Sunday in October at 03:00 local.
  const uint8_t start_day = last_sunday_of_month(local_dt.year, 3);
  const uint8_t end_day = last_sunday_of_month(local_dt.year, 10);

  if (local_dt.month < 3 || local_dt.month > 10) return false;
  if (local_dt.month > 3 && local_dt.month < 10) return true;

  if (local_dt.month == 3) {
    if (local_dt.day > start_day) return true;
    if (local_dt.day < start_day) return false;
    return local_dt.hour >= 2;
  }

  if (local_dt.day < end_day) return true;
  if (local_dt.day > end_day) return false;
  return local_dt.hour < 3;
}
} // namespace

RtcDateTime to_local_time(const RtcDateTime& rtc_time) {
  RtcDateTime local = rtc_time;

  if (kRtcIsUtc) {
    if (kTimeZonePosix && kTimeZonePosix[0] != '\0') {
      setenv("TZ", kTimeZonePosix, 1);
      tzset();
      const time_t epoch = static_cast<time_t>(to_epoch_utc(rtc_time));
      struct tm t {};
      localtime_r(&epoch, &t);
      local.year = static_cast<uint16_t>(t.tm_year + 1900);
      local.month = static_cast<uint8_t>(t.tm_mon + 1);
      local.day = static_cast<uint8_t>(t.tm_mday);
      local.weekday = static_cast<uint8_t>(t.tm_wday);
      local.hour = static_cast<uint8_t>(t.tm_hour);
      local.minute = static_cast<uint8_t>(t.tm_min);
      local.second = static_cast<uint8_t>(t.tm_sec);
    } else {
      add_minutes(&local, kTimeZoneOffsetMinutes);
      if (kEnableEuDst && is_eu_dst_utc(rtc_time)) {
        add_minutes(&local, 60);
      }
    }
  } else {
    // RTC already holds local time; apply local DST rule.
    if (kEnableEuDst && is_eu_dst_local(rtc_time)) {
      add_minutes(&local, 60);
    }
  }

  return local;
}
