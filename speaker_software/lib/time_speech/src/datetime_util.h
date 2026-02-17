#pragma once

#include "hal_rtc.h"

// Apply timezone and optional EU DST rules.
RtcDateTime to_local_time(const RtcDateTime& rtc_time);
