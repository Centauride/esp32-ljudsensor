#include "WifiProvisioner.h"
#include "Config.h"
#include <WiFi.h>

static const char PORTAL_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sound Meter Setup</title>
<style>
  body { font-family: sans-serif; background: #121417; color: #e8eaed; max-width: 480px;
         margin: 0 auto; padding: 24px 16px; line-height: 1.4; }
  h1 { font-size: 1.4rem; }
  p { color: #9aa0a6; }
  label { display: block; margin-top: 20px; font-weight: bold; }
  input, select { width: 100%; box-sizing: border-box; padding: 12px; margin-top: 6px; font-size: 1rem;
                  border-radius: 8px; border: 1px solid #333a42; background: #1e2227; color: #e8eaed; }
  button { width: 100%; margin-top: 24px; padding: 14px; font-size: 1.05rem; border: 0;
           border-radius: 8px; background: #4f9cf9; color: #fff; }
  .msg { background: #4a1f1f; border: 1px solid #a33; padding: 12px; border-radius: 8px; margin-top: 16px; }
  .show { display: flex; align-items: center; gap: 8px; margin-top: 10px; color: #9aa0a6; }
  .show input { width: auto; margin: 0; }
  a { color: #4f9cf9; }
</style>
</head>
<body>
<h1>Set up Wi-Fi</h1>
<p>Choose your home Wi-Fi network and enter its password. The sound meter will connect to it.</p>
%BANNER%
<form method="post" action="/save">
  <label for="pick">Wi-Fi network</label>
  <select id="pick" onchange="if (this.value) document.getElementById('ssid').value = this.value">
    %OPTIONS%
  </select>
  <input type="text" id="ssid" name="ssid" value="%SSID%" placeholder="or type the network name"
         autocapitalize="none" autocorrect="off" required>
  <p><a href="/rescan">Network not listed? Refresh the list</a></p>

  <label for="password">Password</label>
  <input type="password" id="password" name="password" autocapitalize="none" autocorrect="off">
  <div class="show">
    <input type="checkbox" id="show"
           onclick="document.getElementById('password').type = this.checked ? 'text' : 'password'">
    <span>Show password</span>
  </div>

  <button type="submit">Save and connect</button>
</form>
</body>
</html>
)rawliteral";

bool WifiProvisioner::run(LedIndicator &led, WifiCredentials &credentials,
                          const String &prefillSsid, const String &message, unsigned long timeoutMs) {
  _credentials = &credentials;
  _prefillSsid = prefillSsid;
  _message = message;
  _saved = false;

  Serial.println("Setup portal open: join the WiFi network \"" SETUP_AP_NAME "\" with your phone");

  WiFi.disconnect();          // drop any half-finished attempt on the old network
  WiFi.mode(WIFI_AP_STA);     // access point for the phone + station mode for scanning
  scanNetworks();             // scan first: scanning later would briefly kick the phone off
  // Some ESP32-C3 mini boards misbehave at full TX power (weak antenna), so
  // keep it lower here too, like when connecting to the home network.
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  bool apStarted = false;
  for (int attempt = 1; attempt <= 3 && !apStarted; attempt++) {
    apStarted = WiFi.softAP(SETUP_AP_NAME);  // open network, no password
    if (!apStarted) {
      Serial.print("Starting the access point failed (attempt ");
      Serial.print(attempt);
      Serial.println("), retrying...");
      WiFi.softAPdisconnect(true);
      delay(300);
      WiFi.mode(WIFI_AP_STA);
    }
  }
  delay(100);  // let the access point come up before asking for its address
  Serial.print("Access point ");
  Serial.print(apStarted ? "started" : "FAILED to start");
  Serial.print(", address ");
  Serial.println(WiFi.softAPIP());

  // Answer every DNS lookup with our own address, so phones detect the
  // "sign in to network" situation and open the setup page by themselves.
  _dns.start(53, "*", WiFi.softAPIP());

  if (!_routesAdded) {
    _server.on("/", [this]() { handleRoot(); });
    _server.on("/rescan", [this]() { handleRescan(); });
    _server.on("/save", HTTP_POST, [this]() { handleSave(); });
    _server.onNotFound([this]() { handleNotFound(); });
    _routesAdded = true;
  }
  _server.begin();

  unsigned long startMs = millis();
  unsigned long lastLogMs = 0;
  while (true) {
    if (_saved && millis() - _savedAtMs > 1500) break;       // let the confirmation page finish loading
    if (!_saved && millis() - startMs > timeoutMs) break;    // nobody came: give up

    if (millis() - lastLogMs > 5000) {
      lastLogMs = millis();
      Serial.print("Setup portal waiting. Phones connected: ");
      Serial.println(WiFi.softAPgetStationNum());
    }

    _dns.processNextRequest();
    _server.handleClient();
    led.showSetupPulse();
    delay(20);
  }

  _server.stop();
  _dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  return _saved;
}

void WifiProvisioner::scanNetworks() {
  _networkOptions = "<option value=\"\">Choose a network</option>";

  int found = WiFi.scanNetworks();
  if (found < 0) found = 0;

  int added = 0;
  for (int i = 0; i < found && added < 15; i++) {
    String name = WiFi.SSID(i);
    if (name.length() == 0) continue;                         // hidden network
    String escaped = htmlEscape(name);
    if (_networkOptions.indexOf("value=\"" + escaped + "\"") >= 0) continue;  // same name on another channel
    _networkOptions += "<option value=\"" + escaped + "\">" + escaped + "</option>";
    added++;
  }
  WiFi.scanDelete();
}

void WifiProvisioner::sendPage(const String &banner, const String &ssidValue) {
  String bannerHtml;
  if (banner.length() > 0) {
    bannerHtml = "<div class=\"msg\">" + htmlEscape(banner) + "</div>";
  }

  String page = PORTAL_PAGE;
  page.replace("%BANNER%", bannerHtml);
  page.replace("%OPTIONS%", _networkOptions);
  page.replace("%SSID%", htmlEscape(ssidValue));
  _server.send(200, "text/html", page);
}

void WifiProvisioner::handleRoot() {
  sendPage(_message, _prefillSsid);
}

void WifiProvisioner::handleRescan() {
  scanNetworks();
  _server.sendHeader("Location", "/", true);
  _server.send(302, "text/plain", "");
}

void WifiProvisioner::handleSave() {
  String ssid = _server.arg("ssid");
  ssid.trim();  // phone keyboards like to add a trailing space
  String password = _server.arg("password");

  if (ssid.length() == 0 || ssid.length() > 32) {
    sendPage("Please enter the name of your Wi-Fi network.", ssid);
    return;
  }
  if (password.length() > 0 && password.length() < 8) {
    sendPage("Wi-Fi passwords are at least 8 characters long. Please check it and try again.", ssid);
    return;
  }
  if (password.length() > 63) {
    sendPage("That password is too long for a Wi-Fi network (the limit is 63 characters).", ssid);
    return;
  }

  _credentials->save(ssid, password);
  _saved = true;
  _savedAtMs = millis();

  String page =
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>Saved</title>"
    "<style>body{font-family:sans-serif;background:#121417;color:#e8eaed;max-width:480px;"
    "margin:0 auto;padding:24px 16px;line-height:1.5}h1{font-size:1.4rem}</style></head><body>"
    "<h1>Saved</h1>"
    "<p>The sound meter is restarting and will join <b>" + htmlEscape(ssid) + "</b>.</p>"
    "<p>1. Reconnect your phone to your normal Wi-Fi.<br>"
    "2. Open <b>http://" MDNS_HOSTNAME ".local</b> in your browser.</p>"
    "<p>If the lights go back to slow blue pulsing, the password was wrong. "
    "Join <b>" SETUP_AP_NAME "</b> again to retry.</p>"
    "</body></html>";
  _server.send(200, "text/html", page);
}

void WifiProvisioner::handleNotFound() {
  // Any other address (the phone's "is there internet?" checks, for example)
  // is sent to the setup page.
  _server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
  _server.send(302, "text/plain", "");
}

String WifiProvisioner::htmlEscape(const String &text) {
  String out;
  out.reserve(text.length() + 8);
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];
    switch (c) {
      case '&':  out += "&amp;";  break;
      case '<':  out += "&lt;";   break;
      case '>':  out += "&gt;";   break;
      case '"':  out += "&quot;"; break;
      case '\'': out += "&#39;";  break;
      default:   out += c;
    }
  }
  return out;
}
