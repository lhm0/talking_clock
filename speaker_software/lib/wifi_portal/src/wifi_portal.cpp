#include "wifi_portal.h"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "app_state.h"
#include "project_config.h"

#if ENABLE_SERIAL_DEBUG
#define WLOG(...) Serial.println(__VA_ARGS__)
#define WLOGF(...) Serial.printf(__VA_ARGS__)
#else
#define WLOG(...)
#define WLOGF(...)
#endif

namespace {
constexpr uint16_t kHttpPort = 80;
const IPAddress kApIp(192, 168, 4, 1);
DNSServer g_dns;
WebServer g_server(kHttpPort);
WifiPortal* g_instance = nullptr;
}  // namespace

void WifiPortal::begin() {
  g_instance = this;
}

void WifiPortal::start() {
  if (active_) return;
  active_ = true;
  const String ssid = WiFi.SSID();
  WLOGF("WiFi start: saved SSID='%s'\n", ssid.c_str());
  if (ssid.length() == 0) {
    start_ap();
  } else {
    start_sta();
  }
}

void WifiPortal::loop() {
  if (!active_) return;
  if (wm_active_) {
    if (wm_) {
      static_cast<WiFiManager*>(wm_)->process();
    }
    if (WiFi.status() == WL_CONNECTED) {
      wm_active_ = false;
      if (wm_) {
        delete static_cast<WiFiManager*>(wm_);
        wm_ = nullptr;
      }
      start_sta();
    }
    return;
  }

  if (ap_mode_) {
    g_dns.processNextRequest();
  }
  g_server.handleClient();

  if (config_requested_ && !wm_active_) {
    if (millis() - config_request_ms_ < 800) {
      return;
    }
    config_requested_ = false;
    stop_services();
    WiFi.mode(WIFI_AP_STA);
    if (wm_) {
      delete static_cast<WiFiManager*>(wm_);
      wm_ = nullptr;
    }
    WiFiManager* wm = new WiFiManager();
    wm->setAPStaticIPConfig(kApIp, kApIp, IPAddress(255, 255, 255, 0));
    wm->setConfigPortalTimeout(kWifiConfigPortalTimeoutSec);
    wm->setConfigPortalBlocking(false);
    wm->startConfigPortal(kWifiApSsid);
    wm_ = wm;
    wm_active_ = true;
  }
}

void WifiPortal::start_sta() {
  ap_mode_ = false;
  stop_services();

  WLOG("WiFi start_sta");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin();

  const unsigned long start_ms = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - start_ms) < kWifiConnectTimeoutMs) {
    delay(50);
  }

  if (WiFi.status() == WL_CONNECTED) {
    WLOGF("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    if (strlen(kWifiHostname) > 0) {
      WiFi.setHostname(kWifiHostname);
      MDNS.begin(kWifiHostname);
    }
    setup_routes();
    g_server.begin();
  } else {
    WLOG("WiFi STA connect failed -> AP");
    start_ap();
  }
}

void WifiPortal::start_ap() {
  ap_mode_ = true;
  stop_services();

  WLOG("WiFi start_ap");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(kApIp, kApIp, IPAddress(255, 255, 255, 0));
  const bool ap_ok = WiFi.softAP(kWifiApSsid);
  WLOGF("WiFi AP %s, IP=%s\n", ap_ok ? "OK" : "FAIL", WiFi.softAPIP().toString().c_str());

  g_dns.start(53, "*", kApIp);
  setup_routes();
  g_server.begin();
}

void WifiPortal::stop_services() {
  g_server.stop();
  g_dns.stop();
  MDNS.end();
}

void WifiPortal::setup_routes() {
  g_server.on("/generate_204", HTTP_ANY, [this]() { handle_captive_portal(); });
  g_server.on("/hotspot-detect.html", HTTP_ANY, [this]() { handle_captive_portal(); });
  g_server.on("/ncsi.txt", HTTP_ANY, [this]() { handle_captive_portal(); });
  g_server.on("/connecttest.txt", HTTP_ANY, [this]() { handle_captive_portal(); });
  g_server.on("/redirect", HTTP_ANY, [this]() { handle_captive_portal(); });
  g_server.on("/library/test/success.html", HTTP_ANY, [this]() { handle_captive_portal(); });
  g_server.on("/", HTTP_GET, [this]() { handle_http(); });
  g_server.on("/wifi/start", HTTP_GET, [this]() {
    config_requested_ = true;
    config_request_ms_ = millis();
    const bool sta_connected = (WiFi.status() == WL_CONNECTED);
    if (sta_connected) {
      g_server.send(200, "text/html",
                    "<!doctype html><html><head>"
                    "<meta charset='utf-8'/>"
                    "<title>WiFi configuration</title></head>"
                    "<body><p>WiFi configuration is starting.</p>"
                    "<p>Please connect to the access point <strong>SpeakingClock</strong> "
                    "and open <strong>http://192.168.4.1/</strong>.</p>"
                    "</body></html>");
    } else {
      g_server.send(200, "text/html",
                    "<!doctype html><html><head>"
                    "<meta charset='utf-8'/>"
                    "<meta http-equiv='refresh' content='2;url=http://192.168.4.1/'/>"
                    "<title>Starting WiFi</title></head>"
                    "<body><p>Starting WiFi configuration…</p>"
                    "<p>If the page does not open automatically, reconnect to "
                    "<strong>SpeakingClock</strong> and open <strong>http://192.168.4.1/</strong>.</p>"
                    "</body></html>");
    }
  });
  g_server.on("/wifi/start", HTTP_POST, [this]() {
    config_requested_ = true;
    config_request_ms_ = millis();
    const bool sta_connected = (WiFi.status() == WL_CONNECTED);
    if (sta_connected) {
      g_server.send(200, "text/html",
                    "<!doctype html><html><head>"
                    "<meta charset='utf-8'/>"
                    "<title>WiFi configuration</title></head>"
                    "<body><p>WiFi configuration is starting.</p>"
                    "<p>Please connect to the access point <strong>SpeakingClock</strong> "
                    "and open <strong>http://192.168.4.1/</strong>.</p>"
                    "</body></html>");
    } else {
      g_server.send(200, "text/html",
                    "<!doctype html><html><head>"
                    "<meta charset='utf-8'/>"
                    "<meta http-equiv='refresh' content='2;url=http://192.168.4.1/'/>"
                    "<title>Starting WiFi</title></head>"
                    "<body><p>Starting WiFi configuration…</p>"
                    "<p>If the page does not open automatically, reconnect to "
                    "<strong>SpeakingClock</strong> and open <strong>http://192.168.4.1/</strong>.</p>"
                    "</body></html>");
    }
  });
  g_server.on("/wifi/clear", HTTP_POST, [this]() {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    g_server.send(200, "application/json", "{\"ok\":true}");
    start_ap();
  });
  g_server.on("/wifi/status", HTTP_GET, [this]() {
    const bool sta_connected = (WiFi.status() == WL_CONNECTED);
    const char* mode = ap_mode_ ? "ap" : (sta_connected ? "sta" : "idle");
    String json = "{\"mode\":\"";
    json += mode;
    json += "\"}";
    g_server.send(200, "application/json", json);
  });
  g_server.on("/rtc/start", HTTP_GET, [this]() {
    g_server.sendHeader("Location", String("http://") + kApIp.toString() + "/");
    g_server.send(302, "text/plain", "RTC config");
  });
  g_server.on("/rtc/set", HTTP_POST, [this]() {
    if (!rtc_set_cb_) {
      g_server.send(500, "application/json", "{\"ok\":false,\"error\":\"rtc_not_available\"}");
      return;
    }
    const String epoch_str = g_server.arg("epoch_ms");
    const String tz_str = g_server.arg("tz_offset_min");
    if (epoch_str.length() == 0 || tz_str.length() == 0) {
      g_server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_params\"}");
      return;
    }
    const uint64_t epoch_ms = static_cast<uint64_t>(strtoull(epoch_str.c_str(), nullptr, 10));
    const int16_t tz_offset_min = static_cast<int16_t>(atoi(tz_str.c_str()));
    rtc_set_cb_(epoch_ms, tz_offset_min);
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.on("/rtc/now", HTTP_GET, [this]() {
    if (!rtc_now_cb_) {
      g_server.send(500, "application/json", "{\"ok\":false}");
      return;
    }
    uint32_t epoch_utc = 0;
    const char* tz = "";
    if (!rtc_now_cb_(&epoch_utc, &tz)) {
      g_server.send(500, "application/json", "{\"ok\":false}");
      return;
    }
    String json = "{\"ok\":true,\"epoch_utc\":";
    json += epoch_utc;
    json += ",\"tz\":\"";
    json += tz;
    json += "\"}";
    g_server.send(200, "application/json", json);
  });
  g_server.on("/battery", HTTP_GET, [this]() {
    if (!battery_cb_) {
      g_server.send(200, "application/json", "{\"voltage\":null}");
      return;
    }
    const float voltage = battery_cb_();
    String json = "{\"voltage\":";
    json += String(voltage, 2);
    json += "}";
    g_server.send(200, "application/json", json);
  });
  g_server.on("/lang", HTTP_GET, [this]() {
    const char* lang = current_language() == SpeechLanguage::kEnglish ? "EN" : "DE";
    String json = "{\"ok\":true,\"lang\":\"";
    json += lang;
    json += "\"}";
    g_server.send(200, "application/json", json);
  });
  g_server.on("/lang", HTTP_POST, [this]() {
    const String lang = g_server.arg("lang");
    if (lang.length() == 0) {
      g_server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_lang\"}");
      return;
    }
    String up = lang;
    up.toUpperCase();
    if (up == "EN") {
      set_language(SpeechLanguage::kEnglish);
    } else if (up == "DE") {
      set_language(SpeechLanguage::kGerman);
    } else {
      g_server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_lang\"}");
      return;
    }
    g_server.send(200, "application/json", "{\"ok\":true}");
  });
  g_server.onNotFound([this]() { handle_file_request(); });
}

void WifiPortal::handle_captive_portal() {
  if (ap_mode_) {
    g_server.sendHeader("Location", String("http://") + kApIp.toString() + "/");
    g_server.send(302, "text/plain", "Captive portal");
    return;
  }
  g_server.send(200, "text/plain", "OK");
}

void WifiPortal::handle_http() {
  if (!LittleFS.exists("/web/index.html")) {
    g_server.send(500, "text/plain", "Missing /web/index.html");
    return;
  }
  File f = LittleFS.open("/web/index.html", "r");
  g_server.streamFile(f, "text/html");
  f.close();
}

void WifiPortal::handle_file_request() {
  String path = g_server.uri();
  if (path == "/") {
    handle_http();
    return;
  }
  String fs_path = "/web" + path;
  if (!LittleFS.exists(fs_path)) {
    if (ap_mode_) {
      g_server.sendHeader("Location", String("http://") + kApIp.toString() + "/");
      g_server.send(302, "text/plain", "Captive portal");
    } else {
      g_server.send(404, "text/plain", "Not found");
    }
    return;
  }
  File f = LittleFS.open(fs_path, "r");
  String content_type = "text/plain";
  if (fs_path.endsWith(".html")) content_type = "text/html";
  else if (fs_path.endsWith(".css")) content_type = "text/css";
  else if (fs_path.endsWith(".js")) content_type = "application/javascript";
  else if (fs_path.endsWith(".png")) content_type = "image/png";
  else if (fs_path.endsWith(".jpg") || fs_path.endsWith(".jpeg")) content_type = "image/jpeg";
  g_server.streamFile(f, content_type);
  f.close();
}
