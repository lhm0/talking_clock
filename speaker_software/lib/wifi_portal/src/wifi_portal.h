#pragma once

#include <Arduino.h>

class WifiPortal {
 public:
  using RtcSetCallback = void (*)(uint64_t epoch_ms, int16_t tz_offset_min);
  using RtcNowCallback = bool (*)(uint32_t* epoch_utc, const char** tz_posix);
  using BatteryReadCallback = float (*)();

  void begin();
  void start();
  void loop();
  bool is_active() const { return active_; }
  void set_rtc_callback(RtcSetCallback cb) { rtc_set_cb_ = cb; }
  void set_rtc_now_callback(RtcNowCallback cb) { rtc_now_cb_ = cb; }
  void set_battery_callback(BatteryReadCallback cb) { battery_cb_ = cb; }

 private:
  void start_sta();
  void start_ap();
  void stop_services();
  void setup_routes();
  void handle_http();
  void handle_file_request();
  void handle_captive_portal();

  bool active_ = false;
  bool ap_mode_ = false;
  bool config_requested_ = false;
  bool wm_active_ = false;
  void* wm_ = nullptr;
  unsigned long config_request_ms_ = 0;
  RtcSetCallback rtc_set_cb_ = nullptr;
  RtcNowCallback rtc_now_cb_ = nullptr;
  BatteryReadCallback battery_cb_ = nullptr;
};
