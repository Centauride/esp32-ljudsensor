#include "WifiCredentials.h"
#include <Preferences.h>

static const char *PREFS_NAMESPACE = "wifi";

bool WifiCredentials::load(String &ssid, String &password) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("pass", "");
  prefs.end();
  return ssid.length() > 0;
}

void WifiCredentials::save(const String &ssid, const String &password) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.putBool("ok", false);  // not verified until a connection succeeds
  prefs.end();
}

bool WifiCredentials::isVerified() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  bool ok = prefs.getBool("ok", false);
  prefs.end();
  return ok;
}

void WifiCredentials::markVerified() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  if (!prefs.getBool("ok", false)) {
    prefs.putBool("ok", true);
  }
  prefs.end();
}

void WifiCredentials::clear() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}
