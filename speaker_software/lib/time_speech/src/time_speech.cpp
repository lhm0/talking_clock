#include "time_speech.h"

#include <stdio.h>
#include <stdarg.h>

#include "project_config.h"

namespace {
constexpr size_t kMaxPaths = 5;

size_t push_path(char paths[][32], size_t idx, size_t max_paths, const char* fmt, ...) {
  if (idx >= max_paths) return idx;
  va_list args;
  va_start(args, fmt);
  vsnprintf(paths[idx], sizeof(paths[idx]), fmt, args);
  va_end(args);
  return idx + 1;
}

size_t add_number_paths(const char* base, uint8_t number, char paths[][32], size_t idx, size_t max_paths) {
  if (number <= 20 || number % 10 == 0) {
    return push_path(paths, idx, max_paths, "%s/%u.mp3", base, number);
  }
  const uint8_t tens = static_cast<uint8_t>((number / 10) * 10);
  const uint8_t ones = static_cast<uint8_t>(number % 10);
  idx = push_path(paths, idx, max_paths, "%s/%u.mp3", base, tens);
  return push_path(paths, idx, max_paths, "%s/%u.mp3", base, ones);
}

size_t build_playlist_de(const RtcDateTime& dt, char paths[][32], const char* out_paths[], size_t max_paths) {
  if (!out_paths || max_paths < 1) return 0;
  const uint8_t hour = dt.hour % 24;
  const uint8_t minute = dt.minute % 60;
  const char* base = audio_base_path_for(SpeechLanguage::kGerman);

  if (minute == 0) {
    // Full hour uses single file: HHMM.mp3 (e.g., 0100.mp3)
    snprintf(paths[0], sizeof(paths[0]), "%s/%02u%02u.mp3", base, hour, minute);
    out_paths[0] = paths[0];
    return 1;
  }

  if (max_paths < 2) return 0;
  // Other times: HH_Uhr.mp3 + MM.mp3
  snprintf(paths[0], sizeof(paths[0]), "%s/%02u_Uhr.mp3", base, hour);
  snprintf(paths[1], sizeof(paths[1]), "%s/%02u.mp3", base, minute);
  out_paths[0] = paths[0];
  out_paths[1] = paths[1];
  return 2;
}

size_t build_playlist_en(const RtcDateTime& dt, char paths[][32], const char* out_paths[], size_t max_paths) {
  if (!out_paths || max_paths < 2) return 0;
  const uint8_t hour24 = dt.hour % 24;
  const uint8_t minute = dt.minute % 60;
  const uint8_t hour12 = (hour24 % 12 == 0) ? 12 : (hour24 % 12);
  const char* base = audio_base_path_for(SpeechLanguage::kEnglish);
  size_t idx = 0;

  if (minute == 0) {
    if (hour24 == 0) {
      idx = push_path(paths, idx, max_paths, "%s/it_is.mp3", base);
      idx = push_path(paths, idx, max_paths, "%s/midnight.mp3", base);
    } else if (hour24 == 12) {
      idx = push_path(paths, idx, max_paths, "%s/it_is.mp3", base);
      idx = push_path(paths, idx, max_paths, "%s/12noon.mp3", base);
    } else {
      idx = push_path(paths, idx, max_paths, "%s/%02uoclock.mp3", base, hour12);
      const char* ampm = (hour24 < 12) ? "AM" : "PM";
      idx = push_path(paths, idx, max_paths, "%s/%s.mp3", base, ampm);
    }
  } else {
    idx = push_path(paths, idx, max_paths, "%s/%02u.mp3", base, hour12);
    if (minute < 10) {
      idx = push_path(paths, idx, max_paths, "%s/o%u.mp3", base, minute);
    } else {
      idx = push_path(paths, idx, max_paths, "%s/%02u.mp3", base, minute);
    }
    const char* ampm = (hour24 < 12) ? "AM" : "PM";
    idx = push_path(paths, idx, max_paths, "%s/%s.mp3", base, ampm);
  }

  for (size_t i = 0; i < idx; ++i) {
    out_paths[i] = paths[i];
  }
  return idx;
}
}

size_t TimeSpeech::build_playlist(const RtcDateTime& dt, const char* out_paths[], size_t max_paths) {
  return build_playlist_lang(dt, kSpeechLanguage, out_paths, max_paths);
}

size_t TimeSpeech::build_playlist_lang(const RtcDateTime& dt,
                                       SpeechLanguage lang,
                                       const char* out_paths[],
                                       size_t max_paths) {
  switch (lang) {
    case SpeechLanguage::kEnglish:
      return build_playlist_en(dt, paths_, out_paths, max_paths);
    case SpeechLanguage::kGerman:
    default:
      return build_playlist_de(dt, paths_, out_paths, max_paths);
  }
}
