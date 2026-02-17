#include "rtc_ds3231.h"

#include <Wire.h>

#include "board_pins.h"

bool RtcDs3231::begin() {
  Wire.begin(kPinI2cSda, kPinI2cScl);
  return rtc_.begin();
}

bool RtcDs3231::read_datetime(RtcDateTime* out_dt) {
  if (!out_dt) return false;
  const DateTime now = rtc_.now();
  out_dt->year = now.year();
  out_dt->month = now.month();
  out_dt->day = now.day();
  out_dt->weekday = now.dayOfTheWeek();
  out_dt->hour = now.hour();
  out_dt->minute = now.minute();
  out_dt->second = now.second();
  return true;
}

bool RtcDs3231::set_datetime(uint16_t year, uint8_t month, uint8_t day,
                             uint8_t hour, uint8_t minute, uint8_t second) {
  rtc_.adjust(DateTime(year, month, day, hour, minute, second));
  return true;
}
