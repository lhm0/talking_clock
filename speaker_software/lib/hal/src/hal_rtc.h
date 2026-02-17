#pragma once

#include <stdint.h>

struct RtcDateTime {
  uint16_t year;
  uint8_t month;   // 1-12
  uint8_t day;     // 1-31
  uint8_t weekday; // 0=Sunday
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

class HalRtc {
 public:
  virtual ~HalRtc() = default;
  virtual bool begin() = 0;
  virtual bool read_datetime(RtcDateTime* out_dt) = 0;
};
