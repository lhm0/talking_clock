const wifiButton = document.getElementById("wifi-start");
const wifiClearButton = document.getElementById("wifi-clear");
const rtcButton = document.getElementById("rtc-start");
const statusLine = document.getElementById("status");
const rtcStatusLine = document.getElementById("rtc-status");
const batteryValue = document.getElementById("battery-value");
const rtcTimeEl = document.getElementById("rtc-time");
const browserTimeEl = document.getElementById("browser-time");
const timeDiffEl = document.getElementById("time-diff");
const tzInfoEl = document.getElementById("tz-info");
const dstInfoEl = document.getElementById("dst-info");
const langDeButton = document.getElementById("lang-de");
const langEnButton = document.getElementById("lang-en");
const langStatus = document.getElementById("lang-status");

wifiButton?.addEventListener("click", () => {
  statusLine.textContent = "Starting WiFi setup...";
  window.location.href = "/wifi/start";
});

wifiClearButton?.addEventListener("click", () => {
  statusLine.textContent = "Clearing WiFi credentials...";
  fetch("/wifi/clear", { method: "POST" })
    .then((res) => {
      if (!res.ok) throw new Error("bad");
      return res.json();
    })
    .then((data) => {
      if (data.ok) {
        statusLine.textContent = "WiFi credentials deleted.";
        wifiClearButton.classList.add("hidden");
        wifiButton.classList.remove("hidden");
      } else {
        statusLine.textContent = "Failed to delete WiFi credentials.";
      }
    })
    .catch(() => {
      statusLine.textContent = "Failed to delete WiFi credentials.";
    });
});

rtcButton?.addEventListener("click", () => {
  rtcStatusLine.textContent = "Setting RTC...";
  const now = new Date();
  const payload = new URLSearchParams();
  payload.set("epoch_ms", String(now.getTime()));
  payload.set("tz_offset_min", String(now.getTimezoneOffset()));
  fetch("/rtc/set", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: payload,
  })
    .then((res) => {
      if (!res.ok) throw new Error("bad");
      return res.json();
    })
    .then((data) => {
      if (data.ok) {
        rtcStatusLine.textContent = "RTC updated.";
      } else {
        rtcStatusLine.textContent = "RTC update failed.";
      }
    })
    .catch(() => {
      rtcStatusLine.textContent = "RTC update failed.";
    });
});

function setLangActive(lang) {
  langDeButton?.classList.toggle("active", lang === "DE");
  langEnButton?.classList.toggle("active", lang === "EN");
}

async function refreshLanguage() {
  try {
    const res = await fetch("/lang");
    if (!res.ok) throw new Error("bad");
    const data = await res.json();
    if (!data.ok) throw new Error("bad");
    setLangActive(data.lang);
  } catch (err) {
    setLangActive("");
  }
}

async function setLanguage(lang) {
  langStatus.textContent = "Updating language...";
  const payload = new URLSearchParams();
  payload.set("lang", lang);
  try {
    const res = await fetch("/lang", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: payload,
    });
    if (!res.ok) throw new Error("bad");
    const data = await res.json();
    if (data.ok) {
      langStatus.textContent = "Language updated.";
      setLangActive(lang);
    } else {
      langStatus.textContent = "Language update failed.";
    }
  } catch (err) {
    langStatus.textContent = "Language update failed.";
  }
}

langDeButton?.addEventListener("click", () => setLanguage("DE"));
langEnButton?.addEventListener("click", () => setLanguage("EN"));

async function refreshBattery() {
  try {
    const res = await fetch("/battery");
    if (!res.ok) throw new Error("bad");
    const data = await res.json();
    if (typeof data.voltage === "number") {
      batteryValue.textContent = `${data.voltage.toFixed(2)} V`;
    } else {
      batteryValue.textContent = "-- V";
    }
  } catch (err) {
    batteryValue.textContent = "-- V";
  }
}

function splitTz(posix) {
  if (!posix) return { tz: "--", dst: "--" };
  const m = posix.match(/^([A-Z]{3}[+-]\d+)([A-Z]{3})?,?(.*)$/);
  if (!m) return { tz: posix, dst: "--" };
  const tz = m[1];
  const dst = m[3] && m[3].length ? m[3] : "--";
  return { tz, dst };
}

async function refreshRtc() {
  try {
    const res = await fetch("/rtc/now");
    if (!res.ok) throw new Error("bad");
    const data = await res.json();
    if (!data.ok) throw new Error("bad");
    const rtcMs = data.epoch_utc * 1000;
    const rtcDate = new Date(rtcMs);
    const browserDate = new Date();
    rtcTimeEl.textContent = rtcDate.toISOString().replace("T", " ").replace("Z", "");
    browserTimeEl.textContent = browserDate.toISOString().replace("T", " ").replace("Z", "");
    const diffSec = Math.round((browserDate.getTime() - rtcMs) / 1000);
    timeDiffEl.textContent = `${diffSec} s`;
    const tzParts = splitTz(data.tz);
    tzInfoEl.textContent = tzParts.tz;
    dstInfoEl.textContent = tzParts.dst;
  } catch (err) {
    rtcTimeEl.textContent = "--";
    browserTimeEl.textContent = "--";
    timeDiffEl.textContent = "--";
    tzInfoEl.textContent = "--";
    dstInfoEl.textContent = "--";
  }
}

async function refreshWifiStatus() {
  try {
    const res = await fetch("/wifi/status");
    if (!res.ok) throw new Error("bad");
    const data = await res.json();
    if (data.mode === "sta") {
      wifiButton.classList.add("hidden");
      wifiClearButton.classList.remove("hidden");
    } else {
      wifiClearButton.classList.add("hidden");
      wifiButton.classList.remove("hidden");
    }
  } catch (err) {
    wifiClearButton.classList.add("hidden");
    wifiButton.classList.remove("hidden");
  }
}

refreshBattery();
refreshRtc();
refreshWifiStatus();
refreshLanguage();
setInterval(refreshBattery, 5000);
setInterval(refreshRtc, 5000);
setInterval(refreshWifiStatus, 5000);
setInterval(refreshLanguage, 10000);
