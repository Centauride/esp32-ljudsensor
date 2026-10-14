#include "SettingsServer.h"
#include "Config.h"
#include <WiFi.h>
#include <ESPmDNS.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html data-theme="dark">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Sound Level Thresholds</title>
  <script>
    // Apply the saved theme before first paint (dark is the default)
    try {
      const saved = localStorage.getItem('theme');
      if (saved) document.documentElement.dataset.theme = saved;
    } catch (e) {}
  </script>
  <style>
    :root {
      color-scheme: dark;
      --bg: #121417;
      --fg: #e8eaed;
      --muted: #9aa0a6;
      --card: #1e2227;
      --border: #333a42;
      --chart-bg: #181b1f;
      --line: #f1f3f4;
      --accent: #4f9cf9;
    }
    :root[data-theme="light"] {
      color-scheme: light;
      --bg: #ffffff;
      --fg: #1a1a1a;
      --muted: #555555;
      --card: #f0f0f0;
      --border: #dddddd;
      --chart-bg: #ffffff;
      --line: #222222;
      --accent: #2f7de1;
    }
    body { font-family: sans-serif; max-width: 640px; margin: 40px auto; padding: 0 16px;
           background: var(--bg); color: var(--fg); }
    .header { display: flex; justify-content: space-between; align-items: center; gap: 12px; }
    h1 { font-size: 1.2rem; margin: 0; }
    label.field { display: block; margin-top: 24px; font-weight: bold; }
    input[type=range] { width: 100%; accent-color: var(--accent); }
    .value { font-weight: normal; color: var(--muted); }
    #chart { width: 100%; height: 240px; display: block; margin-top: 16px;
             border: 1px solid var(--border); border-radius: 8px; background: var(--chart-bg); }
    .legend { font-size: 0.8rem; color: var(--muted); margin-top: 6px; }
    .level-box { margin-top: 24px; padding: 16px; border-radius: 8px; background: var(--card); text-align: center; }
    .level-number { font-size: 2rem; font-weight: bold; }

    button { margin-top: 24px; padding: 10px 16px; border: 1px solid var(--border); border-radius: 8px;
             background: var(--card); color: var(--fg); font-size: 0.95rem; cursor: pointer; }
    button:hover { border-color: var(--accent); }

    /* dark mode switch */
    .switch { display: inline-flex; align-items: center; gap: 8px; position: relative;
              font-size: 0.85rem; color: var(--muted); cursor: pointer; user-select: none; }
    .switch input { position: absolute; opacity: 0; width: 0; height: 0; }
    .track { width: 40px; height: 22px; border-radius: 11px; background: #888;
             position: relative; transition: background 0.2s; }
    .track::after { content: ""; position: absolute; top: 2px; left: 2px; width: 18px; height: 18px;
                    border-radius: 50%; background: #fff; transition: transform 0.2s; }
    .switch input:checked + .track { background: var(--accent); }
    .switch input:checked + .track::after { transform: translateX(18px); }
    .switch input:focus-visible + .track { outline: 2px solid var(--accent); outline-offset: 2px; }
  </style>
</head>
<body>
  <div class="header">
    <h1>Sound Level Monitor</h1>
    <label class="switch">
      <span>Dark mode</span>
      <input type="checkbox" id="themeToggle">
      <span class="track"></span>
    </label>
  </div>

  <canvas id="chart"></canvas>
  <div class="legend">Last 60 seconds. Zones: green = quiet, orange = normal, red = too loud. Dashed lines are the thresholds.</div>

  <div class="level-box">
    <div>Current smoothed level</div>
    <div class="level-number" id="currentLevel">-</div>
  </div>

  <label class="field">Quiet &rarr; Normal threshold <span class="value" id="quietVal"></span></label>
  <input type="range" id="quietSlider" min="0" max="2000000" step="1000">

  <label class="field">Normal &rarr; Loud threshold <span class="value" id="loudVal"></span></label>
  <input type="range" id="loudSlider" min="0" max="2000000" step="1000">

  <button id="resetBtn" type="button">Reset thresholds</button>

  <script>
    const quietSlider = document.getElementById('quietSlider');
    const loudSlider = document.getElementById('loudSlider');
    const quietVal = document.getElementById('quietVal');
    const loudVal = document.getElementById('loudVal');
    const currentLevel = document.getElementById('currentLevel');
    const themeToggle = document.getElementById('themeToggle');
    const resetBtn = document.getElementById('resetBtn');
    const canvas = document.getElementById('chart');
    const ctx = canvas.getContext('2d');

    const POLL_MS = 250;
    const HISTORY = 240;          // points kept (about 60 s at 250 ms)
    const history = [];
    const SLIDER_MAX = 2000000;
    let minGap = 10000;           // replaced by the firmware's value on first load

    // ----- theme switch -----
    themeToggle.checked = document.documentElement.dataset.theme !== 'light';
    themeToggle.onchange = () => {
      const theme = themeToggle.checked ? 'dark' : 'light';
      document.documentElement.dataset.theme = theme;
      try { localStorage.setItem('theme', theme); } catch (e) {}
      draw();
    };

    // ----- chart -----
    function draw() {
      const css = getComputedStyle(document.documentElement);
      const lineColor = css.getPropertyValue('--line').trim() || '#222';
      const labelColor = css.getPropertyValue('--muted').trim() || '#555';

      const dpr = window.devicePixelRatio || 1;
      const w = canvas.clientWidth;
      const h = canvas.clientHeight;
      if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
        canvas.width = Math.round(w * dpr);
        canvas.height = Math.round(h * dpr);
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, w, h);

      const quiet = Number(quietSlider.value);
      const loud = Number(loudSlider.value);
      const dataMax = history.length ? Math.max(...history) : 0;
      const yMax = Math.max(dataMax, quiet, loud, 1000) * 1.15;   // auto-scale
      const y = v => h - (v / yMax) * h;

      // colored zones
      ctx.fillStyle = 'rgba(46, 204, 113, 0.18)';
      ctx.fillRect(0, y(quiet), w, h - y(quiet));
      ctx.fillStyle = 'rgba(255, 140, 0, 0.22)';
      ctx.fillRect(0, y(loud), w, y(quiet) - y(loud));
      ctx.fillStyle = 'rgba(231, 76, 60, 0.22)';
      ctx.fillRect(0, 0, w, y(loud));

      // level line, newest sample on the right edge
      ctx.strokeStyle = lineColor;
      ctx.lineWidth = 2;
      ctx.beginPath();
      const step = w / (HISTORY - 1);
      history.forEach((v, i) => {
        const x = w - (history.length - 1 - i) * step;
        if (i === 0) ctx.moveTo(x, y(v)); else ctx.lineTo(x, y(v));
      });
      ctx.stroke();

      // threshold lines
      ctx.lineWidth = 1;
      ctx.setLineDash([6, 4]);
      ctx.strokeStyle = '#ff8c00';
      ctx.beginPath(); ctx.moveTo(0, y(quiet)); ctx.lineTo(w, y(quiet)); ctx.stroke();
      ctx.strokeStyle = '#e74c3c';
      ctx.beginPath(); ctx.moveTo(0, y(loud)); ctx.lineTo(w, y(loud)); ctx.stroke();
      ctx.setLineDash([]);

      // axis labels
      ctx.fillStyle = labelColor;
      ctx.font = '11px sans-serif';
      ctx.textAlign = 'left';
      ctx.fillText(Math.round(yMax), 4, 12);
      ctx.fillText('0', 4, h - 4);
      ctx.textAlign = 'right';
      ctx.fillText(quiet, w - 4, y(quiet) - 3);
      ctx.fillText(loud, w - 4, y(loud) - 3);
    }

    // ----- threshold sliders -----
    let sendTimer = null;
    function scheduleSend() {
      clearTimeout(sendTimer);
      sendTimer = setTimeout(() => {
        fetch(`/set?quiet=${quietSlider.value}&loud=${loudSlider.value}`);
      }, 150); // small debounce so dragging doesn't spam requests
    }

    function updateLabels() {
      quietVal.textContent = quietSlider.value;
      loudVal.textContent = loudSlider.value;
    }

    // The loud threshold always stays at least minGap above the quiet one:
    // raising the quiet slider pushes it up, and it can't be dragged below that limit.
    function keepGap() {
      const minLoud = Number(quietSlider.value) + minGap;
      if (Number(loudSlider.value) < minLoud) loudSlider.value = minLoud;
    }

    function onSliderInput() {
      keepGap();
      updateLabels();
      scheduleSend();
      draw();
    }
    quietSlider.oninput = onSliderInput;
    loudSlider.oninput = onSliderInput;
    window.onresize = draw;

    // Move the sliders to whatever values the firmware reports
    function applyServerValues(data) {
      minGap = data.minGap;
      quietSlider.max = SLIDER_MAX - minGap;   // leaves room for loud to stay above it
      quietSlider.value = data.thresholdQuiet;
      loudSlider.value = data.thresholdLoud;
      updateLabels();
    }

    resetBtn.onclick = async () => {
      clearTimeout(sendTimer);                 // drop any pending slider update
      try {
        const res = await fetch('/reset');
        applyServerValues(await res.json());
        draw();
      } catch (e) { /* ignore, user can press again */ }
    };

    // ----- live polling -----
    async function poll() {
      try {
        const res = await fetch('/status');
        const data = await res.json();
        currentLevel.textContent = data.level;

        history.push(data.level);
        if (history.length > HISTORY) history.shift();

        // only set slider positions on first load, so we don't fight the user's drag
        if (!poll.initialized) {
          applyServerValues(data);
          poll.initialized = true;
        }
        draw();
      } catch (e) { /* ignore transient fetch errors */ }
      setTimeout(poll, POLL_MS);
    }
    poll();
  </script>
</body>
</html>
)rawliteral";

bool SettingsServer::begin(LedIndicator &ledIndicator, const String &ssid, const String &password) {
  _ssid = ssid;
  _password = password;

  WiFi.mode(WIFI_STA);
  // Some ESP32-C3 mini boards have weak antennas and fail to connect at full
  // TX power; lowering it is a known fix. Remove if it causes range problems.
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(_ssid.c_str(), _password.c_str());

  Serial.print("Connecting to WiFi");
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startMs > WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println();
      Serial.print("WiFi connection FAILED, status code: ");
      Serial.println(WiFi.status());
      Serial.println("  1 = SSID not found (wrong name, or network is 5GHz-only)");
      Serial.println("  4 = connect failed (usually wrong password)");
      Serial.println("  6 = disconnected / auth problem");

      Serial.print("Name I was looking for: \"");
      Serial.print(_ssid);
      Serial.println("\"");
      Serial.println("Networks the ESP32 can actually see:");
      int found = WiFi.scanNetworks();
      if (found <= 0) {
        Serial.println("  (none found)");
      }
      for (int i = 0; i < found; i++) {
        Serial.printf("  \"%s\"  signal %d dBm  channel %d\n",
                      WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
      }
      WiFi.scanDelete();
      Serial.print("Will keep retrying in the background every ");
      Serial.print(WIFI_RETRY_INTERVAL_MS / 1000);
      Serial.println(" s, or press the BOOT button to retry now.");
      _lastAttemptMs = millis();
      return false;
    }
    delay(150);
    ledIndicator.showLoadingStep();
    Serial.print(".");
  }
  Serial.println();
  startServer();
  return true;
}

void SettingsServer::startServer() {
  Serial.print("Connected. Open this in a browser: http://");
  Serial.println(WiFi.localIP());
  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("Or use the name: http://" MDNS_HOSTNAME ".local");
  }

  _server.on("/", [this]() { handleRoot(); });
  _server.on("/status", [this]() { handleStatus(); });
  _server.on("/set", [this]() { handleSet(); });
  _server.on("/reset", [this]() { handleReset(); });
  _server.begin();
  _running = true;
}

void SettingsServer::maintain() {
  if (_running || _ssid.length() == 0) return;  // once up, the ESP32 re-joins by itself if the link drops

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected in the background.");
    startServer();
    return;
  }

  if (millis() - _lastAttemptMs >= WIFI_RETRY_INTERVAL_MS) {
    retryNow();
  }
}

void SettingsServer::retryNow() {
  if (_running || _ssid.length() == 0) return;
  Serial.println("Retrying WiFi connection...");
  WiFi.disconnect();
  WiFi.begin(_ssid.c_str(), _password.c_str());  // returns immediately, result is picked up by maintain()
  _lastAttemptMs = millis();
}

void SettingsServer::handleClient() {
  if (!_running) return;
  _server.handleClient();
}

void SettingsServer::handleRoot() {
  _server.send_P(200, "text/html", INDEX_HTML);
}

void SettingsServer::handleStatus() {
  String json = "{";
  json += "\"level\":" + String(_currentLevel) + ",";
  json += "\"thresholdQuiet\":" + String(thresholdQuietToNormal) + ",";
  json += "\"thresholdLoud\":" + String(thresholdNormalToLoud) + ",";
  json += "\"minGap\":" + String(MIN_THRESHOLD_GAP);
  json += "}";
  _server.send(200, "application/json", json);
}

void SettingsServer::handleSet() {
  if (_server.hasArg("quiet")) {
    long v = _server.arg("quiet").toInt();
    thresholdQuietToNormal = v < 0 ? 0 : (uint32_t)v;
  }
  if (_server.hasArg("loud")) {
    long v = _server.arg("loud").toInt();
    thresholdNormalToLoud = v < 0 ? 0 : (uint32_t)v;
  }
  enforceThresholdGap();
  _server.send(200, "text/plain", "OK");
}

void SettingsServer::handleReset() {
  resetThresholdsToDefaults();
  handleStatus();  // reply with the restored values so the page can move its sliders
}
