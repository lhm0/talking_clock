#include "serial_cli.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <math.h>

#include "app_state.h"

#if ENABLE_SERIAL_DEBUG
#define DBG_PRINT(...) Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#define DBG_PRINTLN(...)
#define DBG_PRINTF(...)
#endif

namespace {
uint8_t weekday_from_ymd(int year, int month, int day) {
  static const uint8_t kTable[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3) year -= 1;
  const int wd = (year + year / 4 - year / 100 + year / 400 + kTable[month - 1] + day) % 7;
  return static_cast<uint8_t>(wd); // 0=Sunday
}

void speak_time_custom(TimeSpeech& time_speech,
                       uint8_t hour,
                       uint8_t minute,
                       SpeechLanguage lang,
                       bool fs_ok,
                       PlayFileFn play_file) {
  RtcDateTime dt{};
  dt.year = 2026;
  dt.month = 1;
  dt.day = 1;
  dt.hour = hour;
  dt.minute = minute;
  dt.second = 0;
  dt.weekday = weekday_from_ymd(dt.year, dt.month, dt.day);
  const char* playlist[6] = {};
  const size_t count = time_speech.build_playlist_lang(dt, lang, playlist, 6);
  for (size_t i = 0; i < count; ++i) {
    DBG_PRINT("Test path: ");
    DBG_PRINT(playlist[i]);
    DBG_PRINT(" exists=");
    DBG_PRINTLN((fs_ok && LittleFS.exists(playlist[i])) ? "yes" : "no");
    play_file(playlist[i]);
  }
}

void speak_date_custom(DateSpeech& date_speech,
                       uint8_t day,
                       uint8_t month,
                       uint16_t year,
                       SpeechLanguage lang,
                       PlayFileFn play_file) {
  RtcDateTime dt{};
  dt.year = year;
  dt.month = month;
  dt.day = day;
  dt.hour = 0;
  dt.minute = 0;
  dt.second = 0;
  dt.weekday = weekday_from_ymd(year, month, day);
  const char* playlist[10] = {};
  const size_t count = date_speech.build_playlist_lang(dt, lang, playlist, 10);
  if (lang == SpeechLanguage::kEnglish && count > 0) {
    play_file(playlist[0]);
    delay(100);
    for (size_t i = 1; i < count; ++i) {
      play_file(playlist[i]);
    }
  } else {
    for (size_t i = 0; i < count; ++i) {
      play_file(playlist[i]);
    }
  }
}
} // namespace

void serial_cli_handle(TimeSpeech& time_speech,
                       DateSpeech& date_speech,
                       bool fs_ok,
                       PlayFileFn play_file,
                       GetLanguageFn get_lang,
                       SetLanguageFn set_lang,
                       ReadBatteryFn read_battery,
                       ReadBatteryAdcFn read_battery_adc) {
#if ENABLE_SERIAL_DEBUG
  static String line;
  static bool cal_active = false;
  static uint8_t cal_index = 0;
  static float cal_adc[5] = {};
  static float cal_bat[5] = {};

  auto cal_reset = [&]() {
    cal_active = false;
    cal_index = 0;
  };

  auto cal_prompt = [&]() {
    DBG_PRINTF("CAL %u/5: set voltage, enter U_Bat in V (e.g. 3.70)\n",
               static_cast<unsigned>(cal_index + 1));
  };

  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    Serial.write(c);
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line.length() == 0) {
        if (cal_active) {
          cal_prompt();
        }
        line = "";
        continue;
      }
      if (line.length() >= 1) {
        String upper = line;
        upper.toUpperCase();
        if (line == "?" || line == "help") {
          DBG_PRINTLN("Test commands:");
          DBG_PRINTLN("  E HH:MM    - speak time in English (24h input)");
          DBG_PRINTLN("  D HH:MM    - speak time in German (24h input)");
          DBG_PRINTLN("  E DD.MM.YYYY - speak date in English");
          DBG_PRINTLN("  D DD.MM.YYYY - speak date in German");
          DBG_PRINTLN("  BAT        - read battery voltage (ADC GPIO04)");
          DBG_PRINTLN("  CAL        - calibrate ADC (5 points)");
          DBG_PRINTLN("  LANG EN|DE - set default language");
          DBG_PRINTLN("  LANG ?     - show current language");
          line = "";
          continue;
        }
        if (cal_active) {
          if (!read_battery_adc) {
            DBG_PRINTLN("CAL: no ADC reader configured");
            cal_reset();
            line = "";
            continue;
          }
          const char* start = line.c_str();
          char* end = nullptr;
          const float u_bat = strtof(start, &end);
          if (end == start) {
            DBG_PRINTLN("CAL: invalid number, try again");
            cal_prompt();
            line = "";
            continue;
          }
          const float u_adc = read_battery_adc();
          cal_adc[cal_index] = u_adc;
          cal_bat[cal_index] = u_bat;
          DBG_PRINTF("CAL: recorded %u/5 -> U_ADC=%.4f V, U_Bat=%.4f V\n",
                     static_cast<unsigned>(cal_index + 1), u_adc, u_bat);
          cal_index++;
          if (cal_index < 5) {
            cal_prompt();
            line = "";
            continue;
          }

          float s_x = 0.0f;
          float s_x2 = 0.0f;
          float s_x3 = 0.0f;
          float s_x4 = 0.0f;
          float s_y = 0.0f;
          float s_xy = 0.0f;
          float s_x2y = 0.0f;
          for (uint8_t i = 0; i < 5; ++i) {
            const float x = cal_adc[i];
            const float y = cal_bat[i];
            const float x2 = x * x;
            s_x += x;
            s_x2 += x2;
            s_x3 += x2 * x;
            s_x4 += x2 * x2;
            s_y += y;
            s_xy += x * y;
            s_x2y += x2 * y;
          }

          float m[3][4] = {
            {s_x4, s_x3, s_x2, s_x2y},
            {s_x3, s_x2, s_x,  s_xy},
            {s_x2, s_x,  5.0f, s_y},
          };

          auto swap_rows = [&](int r1, int r2) {
            if (r1 == r2) return;
            for (int c = 0; c < 4; ++c) {
              const float tmp = m[r1][c];
              m[r1][c] = m[r2][c];
              m[r2][c] = tmp;
            }
          };

          bool singular = false;
          for (int col = 0; col < 3; ++col) {
            int pivot = col;
            float maxv = fabsf(m[col][col]);
            for (int r = col + 1; r < 3; ++r) {
              const float v = fabsf(m[r][col]);
              if (v > maxv) {
                maxv = v;
                pivot = r;
              }
            }
            if (maxv < 1e-9f) {
              DBG_PRINTLN("CAL: regression failed (singular)");
              singular = true;
              break;
            }
            swap_rows(col, pivot);
            const float div = m[col][col];
            for (int c = col; c < 4; ++c) {
              m[col][c] /= div;
            }
            for (int r = 0; r < 3; ++r) {
              if (r == col) continue;
              const float factor = m[r][col];
              for (int c = col; c < 4; ++c) {
                m[r][c] -= factor * m[col][c];
              }
            }
          }

          if (singular) {
            cal_reset();
            line = "";
            continue;
          }

          const float a = m[0][3];
          const float b = m[1][3];
          const float c = m[2][3];
          const bool saved = save_battery_calibration(a, b, c);
          if (saved) {
            DBG_PRINTF("CAL: saved U_Bat = %.6f * U_ADC^2 + %.6f * U_ADC + %.6f\n", a, b, c);
          } else {
            DBG_PRINTF("CAL: computed U_Bat = %.6f * U_ADC^2 + %.6f * U_ADC + %.6f\n", a, b, c);
            DBG_PRINTLN("CAL: save failed (LittleFS?)");
          }
          cal_reset();
          line = "";
          continue;
        }
        if (upper == "BAT") {
          if (!read_battery) {
            DBG_PRINTLN("BAT: no battery reader configured");
          } else {
            const float v = read_battery();
            DBG_PRINTF("BAT: %.3f V\n", v);
          }
          line = "";
          continue;
        }
        if (upper == "CAL") {
          cal_active = true;
          cal_index = 0;
          DBG_PRINTLN("CAL: enter 5 calibration points.");
          cal_prompt();
          line = "";
          continue;
        }
        if (line.length() >= 4) {
          if (upper.startsWith("LANG")) {
            String arg = line.substring(4);
            arg.trim();
            arg.toUpperCase();
            if (arg == "EN") {
              set_lang(SpeechLanguage::kEnglish);
              DBG_PRINTLN("Language set to English");
            } else if (arg == "DE") {
              set_lang(SpeechLanguage::kGerman);
              DBG_PRINTLN("Language set to German");
            } else if (arg == "?" || arg.length() == 0) {
              DBG_PRINT("Language: ");
              DBG_PRINTLN(get_lang() == SpeechLanguage::kEnglish ? "EN" : "DE");
            }
            line = "";
            continue;
          }
        }
        const char lang_ch = line.charAt(0);
        if (lang_ch != 'E' && lang_ch != 'e' && lang_ch != 'D' && lang_ch != 'd') {
          line = "";
          continue;
        }
        SpeechLanguage lang = (lang_ch == 'E' || lang_ch == 'e') ? SpeechLanguage::kEnglish : SpeechLanguage::kGerman;
        String rest = line.substring(1);
        rest.trim();
        if (rest.indexOf(':') >= 0) {
          int colon = rest.indexOf(':');
          int hh = rest.substring(0, colon).toInt();
          int mm = rest.substring(colon + 1).toInt();
          if (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59) {
            DBG_PRINTF("Test time %02d:%02d (%c)\n", hh, mm, lang_ch);
            speak_time_custom(time_speech, static_cast<uint8_t>(hh), static_cast<uint8_t>(mm), lang, fs_ok, play_file);
          }
        } else if (rest.indexOf('.') >= 0) {
          int d1 = rest.indexOf('.');
          int d2 = rest.indexOf('.', d1 + 1);
          if (d1 > 0 && d2 > d1) {
            int dd = rest.substring(0, d1).toInt();
            int mo = rest.substring(d1 + 1, d2).toInt();
            int yy = rest.substring(d2 + 1).toInt();
            if (dd >= 1 && dd <= 31 && mo >= 1 && mo <= 12 && yy >= 1970) {
              DBG_PRINTF("Test date %02d.%02d.%04d (%c)\n", dd, mo, yy, lang_ch);
              speak_date_custom(date_speech,
                                static_cast<uint8_t>(dd),
                                static_cast<uint8_t>(mo),
                                static_cast<uint16_t>(yy),
                                lang,
                                play_file);
            }
          }
        }
      }
      line = "";
    } else {
      line += c;
    }
  }
#endif
}
