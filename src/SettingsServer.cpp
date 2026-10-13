#include "SettingsServer.h"
#include "Config.h"
#include <WiFi.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Sound Level Thresholds</title>
  <style>
    body { font-family: sans-serif; max-width: 640px; margin: 40px auto; padding: 0 16px; }
    h1 { font-size: 1.2rem; }
    label { display: block; margin-top: 24px; font-weight: bold; }
    input[type=range] { width: 100%; }
    .value { font-weight: normal; color: #555; }
    #chart { width: 100%; height: 240px; display: block; margin-top: 16px;
             border: 1px solid #ddd; border-radius: 8px; background: #fff; }
    .legend { font-size: 0.8rem; color: #555; margin-top: 6px; }
    .level-box { margin-top: 24px; padding: 16px; border-radius: 8px; background: #f0f0f0; text-align: center; }
    .level-number { font-size: 2rem; font-weight: bold; }
  </style>
</head>
<body>
  <h1>Sound Level Monitor</h1>

  <canvas id="chart"></canvas>
  <div class="legend">Last 60 seconds. Zones: green = quiet, orange = normal, red = too loud. Dashed lines are the thresholds.</div>

  <div class="level-box">
    <div>Current smoothed level</div>
    <div class="level-number" id="currentLevel">-</div>
  </div>

  <label>Quiet &rarr; Normal threshold <span class="value" id="quietVal"></span></label>
  <input type="range" id="quietSlider" min="0" max="2000000" step="1000">

  <label>Normal &rarr; Loud threshold <span class="value" id="loudVal"></span></label>
  <input type="range" id="loudSlider" min="0" max="2000000" step="1000">

  <script>
    const quietSlider = document.getElementById('quietSlider');
    const loudSlider = document.getElementById('loudSlider');
    const quietVal = document.getElementById('quietVal');
    const loudVal = document.getElementById('loudVal');
    const currentLevel = document.getElementById('currentLevel');
    const canvas = document.getElementById('chart');
    const ctx = canvas.getContext('2d');

    const POLL_MS = 250;
    const HISTORY = 240;          // points kept (about 60 s at 250 ms)
    const history = [];

    function draw() {
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
      ctx.fillStyle = 'rgba(46, 204, 113, 0.15)';
      ctx.fillRect(0, y(quiet), w, h - y(quiet));
      ctx.fillStyle = 'rgba(255, 140, 0, 0.18)';
      ctx.fillRect(0, y(loud), w, y(quiet) - y(loud));
      ctx.fillStyle = 'rgba(231, 76, 60, 0.18)';
      ctx.fillRect(0, 0, w, y(loud));

      // level line, newest sample on the right edge
      ctx.strokeStyle = '#222';
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
      ctx.fillStyle = '#555';
      ctx.font = '11px sans-serif';
      ctx.textAlign = 'left';
      ctx.fillText(Math.round(yMax), 4, 12);
      ctx.fillText('0', 4, h - 4);
      ctx.textAlign = 'right';
      ctx.fillText(quiet, w - 4, y(quiet) - 3);
      ctx.fillText(loud, w - 4, y(loud) - 3);
    }

    let sendTimer = null;
    function scheduleSend() {
      clearTimeout(sendTimer);
      sendTimer = setTimeout(() => {
        fetch(`/set?quiet=${quietSlider.value}&loud=${loudSlider.value}`);
      }, 150); // small debounce so dragging doesn't spam requests
    }

    quietSlider.oninput = () => { quietVal.textContent = quietSlider.value; scheduleSend(); draw(); };
    loudSlider.oninput  = () => { loudVal.textContent = loudSlider.value; scheduleSend(); draw(); };
    window.onresize = draw;

    async function poll() {
      try {
        const res = await fetch('/status');
        const data = await res.json();
        currentLevel.textContent = data.level;

        history.push(data.level);
        if (history.length > HISTORY) history.shift();

        // only set slider positions on first load, so we don't fight the user's drag
        if (!poll.initialized) {
          quietSlider.value = data.thresholdQuiet;
          loudSlider.value = data.thresholdLoud;
          quietVal.textContent = data.thresholdQuiet;
          loudVal.textContent = data.thresholdLoud;
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

bool SettingsServer::begin(LedIndicator &ledIndicator) {
  WiFi.mode(WIFI_STA);
  // Some ESP32-C3 mini boards have weak antennas and fail to connect at full
  // TX power; lowering it is a known fix. Remove if it causes range problems.
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
      Serial.print(WIFI_SSID);
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
      return false;
    }
    delay(150);
    ledIndicator.showLoadingStep();
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. Open this in a browser: http://");
  Serial.println(WiFi.localIP());

  _server.on("/", [this]() { handleRoot(); });
  _server.on("/status", [this]() { handleStatus(); });
  _server.on("/set", [this]() { handleSet(); });
  _server.begin();
  _running = true;
  return true;
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
  json += "\"thresholdLoud\":" + String(thresholdNormalToLoud);
  json += "}";
  _server.send(200, "application/json", json);
}

void SettingsServer::handleSet() {
  if (_server.hasArg("quiet")) {
    thresholdQuietToNormal = _server.arg("quiet").toInt();
  }
  if (_server.hasArg("loud")) {
    thresholdNormalToLoud = _server.arg("loud").toInt();
  }
  _server.send(200, "text/plain", "OK");
}