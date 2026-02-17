#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hal_rtc.h"
#include "project_config.h"

// Generates a sequence of WAV file paths to speak the current time.
class TimeSpeech {
 public:
 // Returns number of paths written to out_paths.
 size_t build_playlist(const RtcDateTime& dt, const char* out_paths[], size_t max_paths);
 // Returns number of paths written to out_paths, overriding language.
 size_t build_playlist_lang(const RtcDateTime& dt,
                            SpeechLanguage lang,
                            const char* out_paths[],
                            size_t max_paths);

 private:
  char paths_[5][32];
};
