#include <Arduino.h>

#include "board_pins.h"
#include "project_config.h"

#include <AudioFileSourceFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <LittleFS.h>

#include "rtc_ds3231.h"
#include "time_speech.h"
#include "date_speech.h"
#include "datetime_util.h"
#include "wifi_portal.h"
#include "app_state.h"
#include "serial_cli.h"

#if ENABLE_SERIAL_DEBUG
#define DBG_BEGIN(...) Serial.begin(__VA_ARGS__)
#define DBG_WAIT_FOR_SERIAL(ms) do { \
  const unsigned long _start = millis(); \
  while (!Serial && (millis() - _start) < (ms)) { delay(10); } \
} while (0)
#define DBG_PRINT(...) Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DBG_BEGIN(...)
#define DBG_WAIT_FOR_SERIAL(ms)
#define DBG_PRINT(...)
#define DBG_PRINTLN(...)
#define DBG_PRINTF(...)
#endif

RtcDs3231 g_rtc;
bool g_fs_ok = false;
TimeSpeech g_time_speech;
DateSpeech g_date_speech;
AudioOutputI2S* g_out = nullptr;
WifiPortal g_wifi_portal;
hw_timer_t* g_gain_timer = nullptr;
volatile bool g_gain_update_due = false;

uint32_t rtc_to_epoch_utc(const RtcDateTime& utc_dt) {
  const int y = static_cast<int>(utc_dt.year);
  const int m = static_cast<int>(utc_dt.month);
  const int d = static_cast<int>(utc_dt.day);

  auto is_leap = [](int year) {
    return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
  };
  static const uint8_t kDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};

  int days = 0;
  for (int year = 1970; year < y; ++year) {
    days += is_leap(year) ? 366 : 365;
  }
  for (int month = 1; month < m; ++month) {
    days += (month == 2 && is_leap(y)) ? 29 : kDays[month - 1];
  }
  days += (d - 1);

  const int seconds =
      days * 86400 +
      static_cast<int>(utc_dt.hour) * 3600 +
      static_cast<int>(utc_dt.minute) * 60 +
      static_cast<int>(utc_dt.second);
  return static_cast<uint32_t>(seconds);
}

bool rtc_now_cb(uint32_t* epoch_utc, const char** tz_posix) {
  if (!epoch_utc || !tz_posix) return false;
  RtcDateTime rtc_dt{};
  if (!g_rtc.read_datetime(&rtc_dt)) return false;
  *epoch_utc = rtc_to_epoch_utc(rtc_dt);
  *tz_posix = kTimeZonePosix;
  return true;
}

void IRAM_ATTR on_gain_timer() {
  g_gain_update_due = true;
}

float read_battery_adc_voltage() {
  const int raw = analogRead(kPinBatteryAdc);
  return (static_cast<float>(raw) / 4095.0f) * 3.3f;
}

float read_battery_voltage() {
  const float v_adc = read_battery_adc_voltage();
  float a = 0.0f;
  float b = kBatteryVoltageScale;
  float c = 0.0f;
  get_battery_calibration(&a, &b, &c);
  return a * v_adc * v_adc + b * v_adc + c;
}

float read_volume_gain() {
  const int raw = analogRead(kPinVolumePotAdc);
  const float norm = static_cast<float>(raw) / 4095.0f;
  return kMp3GainMin + (kMp3GainMax - kMp3GainMin) * norm;
}

void set_rtc_from_browser(uint64_t epoch_ms, int16_t tz_offset_min) {
  (void)tz_offset_min;
  const uint32_t utc_sec = static_cast<uint32_t>(epoch_ms / 1000LL);
  const DateTime dt(utc_sec);
  g_rtc.set_datetime(dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  DBG_PRINTF("RTC set to %04u-%02u-%02u %02u:%02u:%02u\n",
             dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
}

bool play_mp3_file(const char* path);
void play_wifi_on() {
  play_mp3_file("/mp3/wifi_on.mp3");
}

void speak_time_once() {
  digitalWrite(kPinPowerOff, LOW);
  RtcDateTime rtc_dt{};
  if (!g_rtc.read_datetime(&rtc_dt)) {
    DBG_PRINTLN("RTC read failed");
    return;
  }
  const RtcDateTime local = to_local_time(rtc_dt);
  const char* playlist[4] = {};
  const size_t count = g_time_speech.build_playlist_lang(local, current_language(), playlist, 4);
  for (size_t i = 0; i < count; ++i) {
    DBG_PRINT("Play: ");
    DBG_PRINTLN(playlist[i]);
    play_mp3_file(playlist[i]);
  }
}

void speak_date_once() {
  RtcDateTime rtc_dt{};
  if (!g_rtc.read_datetime(&rtc_dt)) {
    DBG_PRINTLN("RTC read failed");
    return;
  }
  const RtcDateTime local = to_local_time(rtc_dt);
  const char* playlist[6] = {};
  const size_t count = g_date_speech.build_playlist_lang(local, current_language(), playlist, 6);
  if (current_language() == SpeechLanguage::kEnglish && count > 0) {
    play_mp3_file(playlist[0]);
    delay(100);
    for (size_t i = 1; i < count; ++i) {
      play_mp3_file(playlist[i]);
    }
  } else {
    for (size_t i = 0; i < count; ++i) {
      play_mp3_file(playlist[i]);
    }
  }
}

bool play_mp3_file(const char* path) {
  if (!path || !g_fs_ok) return false;
  if (!LittleFS.exists(path)) {
    DBG_PRINT("Missing: ");
    DBG_PRINTLN(path);
    return false;
  }
  DBG_PRINT("Open: ");
  DBG_PRINTLN(path);
  AudioFileSourceFS file(LittleFS, path);
  AudioGeneratorMP3 mp3;
  if (!g_out) return false;
  if (!mp3.begin(&file, g_out)) {
    DBG_PRINT("MP3 begin failed: ");
    DBG_PRINTLN(path);
    return false;
  }
  while (mp3.isRunning()) {
    if (g_gain_update_due) {
      g_gain_update_due = false;
      if (g_out) {
        g_out->SetGain(read_volume_gain());
      }
    }
    if (!mp3.loop()) mp3.stop();
    delay(1);
  }
  return true;
}

void list_littlefs_root() {
  File root = LittleFS.open("/", "r");
  if (!root) {
    DBG_PRINTLN("LittleFS root open failed");
    return;
  }
  DBG_PRINTLN("LittleFS listing:");
  File f = root.openNextFile();
  while (f) {
    DBG_PRINT(" - ");
    DBG_PRINT(f.name());
    if (!f.isDirectory()) {
      DBG_PRINT(" (");
      DBG_PRINT(f.size());
      DBG_PRINT(" bytes)");
    }
    DBG_PRINTLN();
    f = root.openNextFile();
  }
  root.close();
}

void setup() {
  DBG_BEGIN(115200);
  DBG_WAIT_FOR_SERIAL(2000);
  DBG_PRINTLN("Speaking Clock boot");

  if (!g_rtc.begin()) {
    DBG_PRINTLN("RTC init failed");
  }

  g_fs_ok = LittleFS.begin(false);
  if (!g_fs_ok) {
    DBG_PRINTLN("LittleFS init failed");
  } else {
    DBG_PRINTLN("LittleFS init OK");
    list_littlefs_root();
    app_state_begin(g_fs_ok);
  }

  g_wifi_portal.begin();
  g_wifi_portal.set_rtc_callback(set_rtc_from_browser);
  g_wifi_portal.set_rtc_now_callback(rtc_now_cb);
  g_wifi_portal.set_battery_callback(read_battery_voltage);

  g_out = new AudioOutputI2S();
  g_out->SetPinout(kPinI2sBclk, kPinI2sLrc, kPinI2sData);
  g_out->SetChannels(1);
  g_out->SetOutputModeMono(true);
  g_out->SetGain(read_volume_gain());

  pinMode(kPinTriggerButton, INPUT_PULLUP);
  pinMode(kPinConfigButton, INPUT_PULLUP);
  pinMode(kPinBatteryAdc, INPUT);
  pinMode(kPinVolumePotAdc, INPUT);
  pinMode(kPinPowerOff, OUTPUT);
  digitalWrite(kPinPowerOff, LOW);
  
  DBG_PRINTF("Flash size: %u bytes\n", ESP.getFlashChipSize());
  DBG_PRINTF("PSRAM size: %u bytes\n", ESP.getPsramSize());

  g_gain_timer = timerBegin(0, 80, true);
  if (g_gain_timer) {
    timerAttachInterrupt(g_gain_timer, &on_gain_timer, true);
    timerAlarmWrite(g_gain_timer, 200000, true);
    timerAlarmEnable(g_gain_timer);
  }

  const bool cfg_pressed = (digitalRead(kPinConfigButton) == LOW);
  if (cfg_pressed) {
    play_wifi_on();
    g_wifi_portal.start();
  } else {
    uint32_t prev_epoch = 0;
    const bool has_prev = load_last_time(&prev_epoch);
    RtcDateTime rtc_dt{};
    if (g_rtc.read_datetime(&rtc_dt)) {
      const uint32_t now_epoch = rtc_to_epoch_utc(rtc_dt);
      const uint32_t diff = has_prev ? (now_epoch - prev_epoch) : 0;
      DBG_PRINTF("Time delta (startup) = %u s\n", diff);
      const bool do_date = has_prev && (diff <= 20);
      speak_time_once();
      if (do_date) {
        delay(500);
        speak_date_once();
      }
      save_last_time(now_epoch);
      digitalWrite(kPinPowerOff, HIGH);
    }
  }
}

void loop() {
  static int prev_time_state = HIGH;
  const int time_state = digitalRead(kPinTriggerButton);

  if (prev_time_state == HIGH && time_state == LOW) {
    DBG_PRINTLN("Button: TIME");
    uint32_t prev_epoch = 0;
    const bool has_prev = load_last_time(&prev_epoch);
    RtcDateTime rtc_dt{};
    if (g_rtc.read_datetime(&rtc_dt)) {
      const uint32_t now_epoch = rtc_to_epoch_utc(rtc_dt);
      const uint32_t diff = has_prev ? (now_epoch - prev_epoch) : 0;
      DBG_PRINTF("Time delta (trigger) = %u s\n", diff);
      const bool do_date = has_prev && (diff <= 20);
      speak_time_once();
      if (do_date) {
        delay(500);
        speak_date_once();
      }
      save_last_time(now_epoch);
      digitalWrite(kPinPowerOff, HIGH);
    }
  }

  prev_time_state = time_state;

  serial_cli_handle(g_time_speech,
                    g_date_speech,
                    g_fs_ok,
                    play_mp3_file,
                    current_language,
                    set_language,
                    read_battery_voltage,
                    read_battery_adc_voltage);
  g_wifi_portal.loop();
}
