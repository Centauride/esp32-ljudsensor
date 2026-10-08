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
    body { font-family: sans-serif; max-width: 420px; margin: 40px auto; padding: 0 16px; }
    h1 { font-size: 1.2rem; }
    label { display: block; margin-top: 24px; font-weight: bold; }
    input[type=range] { width: 100%; }
    .value { font-weight: normal; color: #555; }
    .level-box { margin-top: 32px; padding: 16px; border-radius: 8px; background: #f0f0f0; text-align: center; }
    .level-number { font-size: 2rem; font-weight: bold; }
  </style>
</head>
<body>
  <h1>Sound Threshold Settings</h1>

  <label>Quiet &rarr; Normal threshold <span class="value" id="quietVal"></span></label>
  <input type="range" id="quietSlider" min="0" max="2000000" step="1000">

  <label>Normal &rarr; Loud threshold <span class="value" id="loudVal"></span></label>
  <input type="range" id="loudSlider" min="0" max="2000000" step="1000">

  <div class="level-box">
    <div>Current smoothed level</div>
    <div class="level-number" id="currentLevel">-</div>
  </div>

  <script>
    const quietSlider = document.getElementById('quietSlider');
    const loudSlider = document.getElementById('loudSlider');
    const quietVal = document.getElementById('quietVal');
    const loudVal = document.getElementById('loudVal');
    const currentLevel = document.getElementById('currentLevel');

    let sendTimer = null;
    function scheduleSend() {
      clearTimeout(sendTimer);
      sendTimer = setTimeout(() => {
        fetch(`/set?quiet=${quietSlider.value}&loud=${loudSlider.value}`);
      }, 150); // small debounce so dragging doesn't spam requests
    }

    quietSlider.oninput = () => { quietVal.textContent = quietSlider.value; scheduleSend(); };
    loudSlider.oninput  = () => { loudVal.textContent = loudSlider.value; scheduleSend(); };

    async function poll() {
      try {
        const res = await fetch('/status');
        const data = await res.json();
        currentLevel.textContent = data.level;

        // only set slider positions on first load, so we don't fight the user's drag
        if (!poll.initialized) {
          quietSlider.value = data.thresholdQuiet;
          loudSlider.value = data.thresholdLoud;
          quietVal.textContent = data.thresholdQuiet;
          loudVal.textContent = data.thresholdLoud;
          poll.initialized = true;
        }
      } catch (e) { /* ignore transient fetch errors */ }
      setTimeout(poll, 300);
    }
    poll();
  </script>
</body>
</html>
)rawliteral";

void SettingsServer::begin(LedIndicator &ledIndicator) {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
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
}

void SettingsServer::handleClient() {
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
