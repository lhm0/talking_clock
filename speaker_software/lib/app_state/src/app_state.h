#pragma once

#include <stdint.h>

#include "project_config.h"

void app_state_begin(bool fs_ok);

bool load_last_time(uint32_t* epoch_out);
bool save_last_time(uint32_t epoch);

SpeechLanguage current_language();
void set_language(SpeechLanguage lang);

bool get_battery_calibration(float* a_out, float* b_out, float* c_out);
bool save_battery_calibration(float a, float b, float c);
