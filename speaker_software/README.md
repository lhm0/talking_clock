# Speaker Software (ESP32-S3 Pico)

This folder contains the ESP32-S3 implementation.

## Structure

- `platformio.ini` - PlatformIO environments and build flags
- `src/` - Application entry point (`main.cpp`)
- `include/` - Project-wide config and board pin maps
- `lib/` - Reusable modules (hardware abstraction + device drivers)
- `test/` - Placeholder for unit tests
- `docs/` - Notes and integration docs

## Modules

- `lib/hal/` - Hardware abstraction interfaces
- `lib/time_speech/` - Time/date playlist generators + timezone/DST
- `lib/rtc_ds3231/` - DS3231 RTC driver

## Notes

- Audio files are stored in LittleFS under `/mp3`.
- Audio uses ESP8266Audio (legacy i2s.h backend).
