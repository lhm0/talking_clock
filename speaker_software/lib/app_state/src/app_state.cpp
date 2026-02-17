#include "app_state.h"

#include <LittleFS.h>
#include <stdio.h>

namespace {
constexpr const char* kLastTimePath = "/last_time.txt";
constexpr const char* kLanguagePath = "/lang.txt";
constexpr const char* kBatteryCalPath = "/bat_cal.txt";

bool g_fs_ok = false;
SpeechLanguage g_lang = kSpeechLanguage;
float g_bat_a = 0.0f;
float g_bat_b = kBatteryVoltageScale;
float g_bat_c = 0.0f;
bool g_bat_cal_valid = false;

bool load_language_from_fs(SpeechLanguage* out_lang) {
  if (!out_lang || !g_fs_ok) return false;
  if (!LittleFS.exists(kLanguagePath)) return false;
  File f = LittleFS.open(kLanguagePath, "r");
  if (!f) return false;
  String s = f.readStringUntil('\n');
  f.close();
  s.trim();
  s.toUpperCase();
  if (s == "EN") {
    *out_lang = SpeechLanguage::kEnglish;
    return true;
  }
  if (s == "DE") {
    *out_lang = SpeechLanguage::kGerman;
    return true;
  }
  return false;
}

bool save_language_to_fs(SpeechLanguage lang) {
  if (!g_fs_ok) return false;
  File f = LittleFS.open(kLanguagePath, "w");
  if (!f) return false;
  f.print(lang == SpeechLanguage::kEnglish ? "EN" : "DE");
  f.print('\n');
  f.close();
  return true;
}

bool load_battery_calibration_from_fs(float* a_out, float* b_out, float* c_out) {
  if (!a_out || !b_out || !c_out || !g_fs_ok) return false;
  if (!LittleFS.exists(kBatteryCalPath)) return false;
  File f = LittleFS.open(kBatteryCalPath, "r");
  if (!f) return false;
  String s = f.readStringUntil('\n');
  f.close();
  s.trim();
  if (s.length() == 0) return false;
  float a = 0.0f;
  float b = 0.0f;
  float c = 0.0f;
  const int count = sscanf(s.c_str(), "%f %f %f", &a, &b, &c);
  if (count == 3) {
    *a_out = a;
    *b_out = b;
    *c_out = c;
    return true;
  }
  if (count == 2) {
    // Backward compat: old linear model U_Bat = a*U_ADC + b.
    *a_out = 0.0f;
    *b_out = a;
    *c_out = b;
    return true;
  }
  return false;
}

bool save_battery_calibration_to_fs(float a, float b, float c) {
  if (!g_fs_ok) return false;
  File f = LittleFS.open(kBatteryCalPath, "w");
  if (!f) return false;
  f.print(a, 6);
  f.print(' ');
  f.print(b, 6);
  f.print(' ');
  f.print(c, 6);
  f.print('\n');
  f.close();
  return true;
}
} // namespace

void app_state_begin(bool fs_ok) {
  g_fs_ok = fs_ok;
  if (!g_fs_ok) return;
  if (!LittleFS.exists(kLanguagePath)) {
    save_language_to_fs(g_lang);
  }
  SpeechLanguage persisted{};
  if (load_language_from_fs(&persisted)) {
    g_lang = persisted;
  }

  float a = 0.0f;
  float b = 0.0f;
  float c = 0.0f;
  if (load_battery_calibration_from_fs(&a, &b, &c)) {
    g_bat_a = a;
    g_bat_b = b;
    g_bat_c = c;
    g_bat_cal_valid = true;
  }
}

bool load_last_time(uint32_t* epoch_out) {
  if (!epoch_out || !g_fs_ok) return false;
  if (!LittleFS.exists(kLastTimePath)) return false;
  File f = LittleFS.open(kLastTimePath, "r");
  if (!f) return false;
  String s = f.readStringUntil('\n');
  f.close();
  s.trim();
  if (s.length() == 0) return false;
  *epoch_out = static_cast<uint32_t>(strtoul(s.c_str(), nullptr, 10));
  return true;
}

bool save_last_time(uint32_t epoch) {
  if (!g_fs_ok) return false;
  File f = LittleFS.open(kLastTimePath, "w");
  if (!f) return false;
  f.print(epoch);
  f.print('\n');
  f.close();
  return true;
}

SpeechLanguage current_language() {
  SpeechLanguage persisted{};
  if (load_language_from_fs(&persisted)) {
    g_lang = persisted;
  }
  return g_lang;
}

void set_language(SpeechLanguage lang) {
  g_lang = lang;
  save_language_to_fs(g_lang);
}

bool get_battery_calibration(float* a_out, float* b_out, float* c_out) {
  if (!a_out || !b_out || !c_out) return false;
  *a_out = g_bat_a;
  *b_out = g_bat_b;
  *c_out = g_bat_c;
  return g_bat_cal_valid;
}

bool save_battery_calibration(float a, float b, float c) {
  g_bat_a = a;
  g_bat_b = b;
  g_bat_c = c;
  g_bat_cal_valid = true;
  return save_battery_calibration_to_fs(a, b, c);
}
