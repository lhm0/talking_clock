#pragma once

// Default pin configuration for a breadboard setup on ESP32-S3.
// Adjust as needed once wiring is finalized.

// I2C (DS3231) - keep away from ADC pins
constexpr int kPinI2cSda = 14;
constexpr int kPinI2cScl = 15;

// I2S (MAX98357A)
constexpr int kPinI2sBclk = 5;
constexpr int kPinI2sLrc  = 6;
constexpr int kPinI2sData = 7;

// Trigger button (active low)
constexpr int kPinTriggerButton = 8;

// Config button (active low)
constexpr int kPinConfigButton = 9;

// Power-off control (active high)
constexpr int kPinPowerOff = 10;

// Battery voltage ADC input (adjust as needed)
constexpr int kPinBatteryAdc = 4;

// Volume potentiometer ADC input (0..3.3V)
constexpr int kPinVolumePotAdc = 1;
