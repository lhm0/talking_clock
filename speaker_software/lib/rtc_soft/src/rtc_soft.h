#pragma once

#include <stdint.h>

#include "hal_rtc.h"

// Simple software clock using millis() as time base.
class RtcSoft : public HalRtc {
 public:
  bool begin() override;
  bool read_datetime(RtcDateTime* out_dt) override;

  void set_start_datetime(const RtcDateTime& start);

 private:
  RtcDateTime start_{};
  unsigned long start_ms_ = 0;
  bool started_ = false;
};
