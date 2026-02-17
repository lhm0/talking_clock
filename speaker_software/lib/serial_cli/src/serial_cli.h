#pragma once

#include <AudioFileSourceFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#include "project_config.h"
#include "time_speech.h"
#include "date_speech.h"

using PlayFileFn = bool (*)(const char* path);
using GetLanguageFn = SpeechLanguage (*)();
using SetLanguageFn = void (*)(SpeechLanguage lang);
using ReadBatteryFn = float (*)();
using ReadBatteryAdcFn = float (*)();

void serial_cli_handle(TimeSpeech& time_speech,
                       DateSpeech& date_speech,
                       bool fs_ok,
                       PlayFileFn play_file,
                       GetLanguageFn get_lang,
                       SetLanguageFn set_lang,
                       ReadBatteryFn read_battery,
                       ReadBatteryAdcFn read_battery_adc);
