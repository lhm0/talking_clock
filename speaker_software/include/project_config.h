#pragma once

// Global project configuration (ESP32-S3 build)

// Audio
// I2S output sample rate
constexpr int kAudioOutputSampleRateHz = 48000;
// MP3 output gain range (0.0 .. 1.0 recommended)
constexpr float kMp3GainMin = 0.1f;
constexpr float kMp3GainMax = 1.0f;
// Speech language selection
enum class SpeechLanguage : uint8_t {
  kGerman = 0,
  kEnglish = 1,
};
constexpr SpeechLanguage kSpeechLanguage = SpeechLanguage::kGerman;
// Base paths for audio files in LittleFS (MP3 files).
constexpr const char* kAudioBasePathDe = "/mp3";
constexpr const char* kAudioBasePathEn = "/mp3_en";
inline const char* audio_base_path_for(SpeechLanguage lang) {
  return (lang == SpeechLanguage::kEnglish) ? kAudioBasePathEn : kAudioBasePathDe;
}
inline const char* audio_base_path() {
  return audio_base_path_for(kSpeechLanguage);
}

// Debug serial logging (can be overridden via build flag)
#ifndef ENABLE_SERIAL_DEBUG
#define ENABLE_SERIAL_DEBUG 1
#endif

// Timezone / DST configuration
// RTC runs on UTC; local time is derived via POSIX TZ string.
constexpr bool kRtcIsUtc = true;
constexpr const char* kTimeZonePosix = "CET-1CEST,M3.5.0/02,M10.5.0/03";
constexpr int kTimeZoneOffsetMinutes = 60; // CET default (fallback)
constexpr bool kEnableEuDst = true;        // fallback rule if POSIX TZ is empty

// Software clock start (used while RTC is disconnected)
constexpr uint16_t kSoftStartYear = 2026;
constexpr uint8_t kSoftStartMonth = 1;
constexpr uint8_t kSoftStartDay = 29;
constexpr uint8_t kSoftStartHour = 13;
constexpr uint8_t kSoftStartMinute = 58;
constexpr uint8_t kSoftStartSecond = 0;

// RTC
constexpr int kRtcI2cFrequencyHz = 100000;

// WiFi / Web
constexpr const char* kWifiApSsid = "SpeakingClock";
constexpr const char* kWifiHostname = "clock";
constexpr uint32_t kWifiConnectTimeoutMs = 10000;
constexpr uint16_t kWifiConfigPortalTimeoutSec = 180;

// Battery measurement (ADC)
constexpr float kBatteryVoltageScale = 1.68f; // 68k/100k divider, measure across 100k
