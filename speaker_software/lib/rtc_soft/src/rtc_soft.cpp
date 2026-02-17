#include "rtc_soft.h"

#include <Arduino.h>

namespace {
bool is_leap_year(uint16_t year) {
  return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

uint8_t days_in_month(uint16_t year, uint8_t month) {
  static const uint8_t kDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month == 2 && is_leap_year(year)) return 29;
  return kDays[month - 1];
}

uint8_t weekday_for_date(uint16_t y, uint8_t m, uint8_t d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

RtcDateTime add_seconds(const RtcDateTime& base, uint32_t seconds) {
  RtcDateTime dt = base;
  uint32_t total = static_cast<uint32_t>(dt.hour) * 3600U +
                   static_cast<uint32_t>(dt.minute) * 60U +
                   static_cast<uint32_t>(dt.second) + seconds;

  uint32_t day_delta = total / 86400U;
  total %= 86400U;

  dt.hour = static_cast<uint8_t>(total / 3600U);
  dt.minute = static_cast<uint8_t>((total / 60U) % 60U);
  dt.second = static_cast<uint8_t>(total % 60U);

  while (day_delta > 0) {
    const uint8_t dim = days_in_month(dt.year, dt.month);
    if (dt.day < dim) {
      dt.day += 1;
    } else {
      dt.day = 1;
      if (dt.month < 12) {
        dt.month += 1;
      } else {
        dt.month = 1;
        dt.year += 1;
      }
    }
    dt.weekday = (dt.weekday + 1) % 7;
    day_delta -= 1;
  }

  return dt;
}
} // namespace

bool RtcSoft::begin() {
  start_ms_ = millis();
  started_ = true;
  return true;
}

bool RtcSoft::read_datetime(RtcDateTime* out_dt) {
  if (!out_dt || !started_) return false;
  const unsigned long elapsed_ms = millis() - start_ms_;
  const uint32_t elapsed_sec = static_cast<uint32_t>(elapsed_ms / 1000UL);
  *out_dt = add_seconds(start_, elapsed_sec);
  return true;
}

void RtcSoft::set_start_datetime(const RtcDateTime& start) {
  start_ = start;
  if (start_.month < 1) start_.month = 1;
  if (start_.month > 12) start_.month = 12;
  const uint8_t dim = days_in_month(start_.year, start_.month);
  if (start_.day < 1) start_.day = 1;
  if (start_.day > dim) start_.day = dim;
  start_.weekday = weekday_for_date(start_.year, start_.month, start_.day);
}
