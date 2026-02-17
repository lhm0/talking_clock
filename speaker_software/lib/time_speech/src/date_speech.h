#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hal_rtc.h"
#include "project_config.h"

// Generates a sequence of WAV file paths to speak the current date.
class DateSpeech {
 public:
  // Order: weekday, day, month, year.
  size_t build_playlist(const RtcDateTime& dt, const char* out_paths[], size_t max_paths);
  // Order: weekday, day, month, year (override language).
  size_t build_playlist_lang(const RtcDateTime& dt,
                             SpeechLanguage lang,
                             const char* out_paths[],
                             size_t max_paths);

 private:
  char paths_[10][32];
};
