#include "date_speech.h"

#include <stdio.h>
#include <stdarg.h>

#include "project_config.h"

namespace {
size_t push_path(char paths[][32], size_t idx, size_t max_paths, const char* fmt, ...) {
  if (idx >= max_paths) return idx;
  va_list args;
  va_start(args, fmt);
  vsnprintf(paths[idx], sizeof(paths[idx]), fmt, args);
  va_end(args);
  return idx + 1;
}

size_t add_number_paths(const char* base, uint16_t number, char paths[][32], size_t idx, size_t max_paths) {
  if (number <= 20 || number % 10 == 0) {
    return push_path(paths, idx, max_paths, "%s/%u.mp3", base, number);
  }
  const uint16_t tens = static_cast<uint16_t>((number / 10) * 10);
  const uint16_t ones = static_cast<uint16_t>(number % 10);
  idx = push_path(paths, idx, max_paths, "%s/%u.mp3", base, tens);
  return push_path(paths, idx, max_paths, "%s/%u.mp3", base, ones);
}

size_t add_ordinal_paths(const char* base, uint16_t number, char paths[][32], size_t idx, size_t max_paths) {
  if (number <= 20 || number % 10 == 0) {
    return push_path(paths, idx, max_paths, "%s/h-%u.mp3", base, number);
  }
  const uint16_t tens = static_cast<uint16_t>((number / 10) * 10);
  const uint16_t ones = static_cast<uint16_t>(number % 10);
  idx = push_path(paths, idx, max_paths, "%s/h-%u.mp3", base, tens);
  return push_path(paths, idx, max_paths, "%s/h-%u.mp3", base, ones);
}

size_t add_year_paths(const char* base, uint16_t year, char paths[][32], size_t idx, size_t max_paths) {
  if (year < 1000) {
    return add_number_paths(base, year, paths, idx, max_paths);
  }
  uint16_t thousands = static_cast<uint16_t>(year / 1000);
  uint16_t rem = static_cast<uint16_t>(year % 1000);
  idx = add_number_paths(base, thousands, paths, idx, max_paths);
  idx = push_path(paths, idx, max_paths, "%s/thousand.mp3", base);
  if (rem >= 100) {
    uint16_t hundreds = static_cast<uint16_t>(rem / 100);
    rem = static_cast<uint16_t>(rem % 100);
    idx = add_number_paths(base, hundreds, paths, idx, max_paths);
    idx = push_path(paths, idx, max_paths, "%s/hundred.mp3", base);
  }
  if (rem > 0) {
    idx = add_number_paths(base, rem, paths, idx, max_paths);
  }
  return idx;
}
}

size_t DateSpeech::build_playlist(const RtcDateTime& dt, const char* out_paths[], size_t max_paths) {
  return build_playlist_lang(dt, kSpeechLanguage, out_paths, max_paths);
}

size_t DateSpeech::build_playlist_lang(const RtcDateTime& dt,
                                       SpeechLanguage lang,
                                       const char* out_paths[],
                                       size_t max_paths) {
  if (!out_paths || max_paths < 4) return 0;
  size_t idx = 0;

  if (lang == SpeechLanguage::kEnglish) {
    const char* base = audio_base_path_for(SpeechLanguage::kEnglish);
    const uint8_t weekday_num = (dt.weekday == 0) ? 7 : dt.weekday; // 1=Mon ... 7=Sun
    idx = push_path(paths_, idx, max_paths, "%s/%02ud.mp3", base, weekday_num);
    idx = push_path(paths_, idx, max_paths, "%s/%02umo.mp3", base, dt.month);
    idx = push_path(paths_, idx, max_paths, "%s/%02u_.mp3", base, dt.day);
    const uint8_t y1 = static_cast<uint8_t>(dt.year / 100);
    const uint8_t y2 = static_cast<uint8_t>(dt.year % 100);
    idx = push_path(paths_, idx, max_paths, "%s/%02u.mp3", base, y1);
    idx = push_path(paths_, idx, max_paths, "%s/%02u.mp3", base, y2);
  } else {
    const char* base = audio_base_path_for(SpeechLanguage::kGerman);
    idx = push_path(paths_, idx, max_paths, "%s/%u_day.mp3", base, dt.weekday);
    idx = push_path(paths_, idx, max_paths, "%s/%02u_.mp3", base, dt.day);
    idx = push_path(paths_, idx, max_paths, "%s/%02u_mo.mp3", base, dt.month);
    idx = push_path(paths_, idx, max_paths, "%s/%u.mp3", base, dt.year);
  }

  for (size_t i = 0; i < idx; ++i) {
    out_paths[i] = paths_[i];
  }
  return idx;
}
