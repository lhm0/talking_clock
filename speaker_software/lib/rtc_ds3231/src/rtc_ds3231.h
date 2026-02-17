#pragma once

#include <RTClib.h>

#include "hal_rtc.h"

class RtcDs3231 : public HalRtc {
 public:
  bool begin() override;
  bool read_datetime(RtcDateTime* out_dt) override;
  bool set_datetime(uint16_t year, uint8_t month, uint8_t day,
                    uint8_t hour, uint8_t minute, uint8_t second);

 private:
  RTC_DS3231 rtc_;
};
