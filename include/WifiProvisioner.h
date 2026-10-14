#pragma once
#include <WebServer.h>
#include <DNSServer.h>
#include "LedIndicator.h"
#include "WifiCredentials.h"

// The WiFi setup portal. The board opens its own temporary WiFi network; a
// phone that joins it is sent to a page where the user picks their home
// network and types its password. The credentials are saved to flash.
class WifiProvisioner {
public:
  // Blocks while the portal is open (the LEDs pulse blue).
  // Returns true as soon as the user has saved credentials, or false if
  // nobody did within timeoutMs. prefillSsid and message are optional: the
  // network name to pre-fill and a notice shown at the top of the page.
  bool run(LedIndicator &led, WifiCredentials &credentials,
           const String &prefillSsid, const String &message, unsigned long timeoutMs);

private:
  WebServer _server{80};
  DNSServer _dns;
  WifiCredentials *_credentials = nullptr;
  String _prefillSsid;
  String _message;
  String _networkOptions;   // <option> tags built from the last scan
  bool _routesAdded = false;
  bool _saved = false;
  unsigned long _savedAtMs = 0;

  void scanNetworks();
  void sendPage(const String &banner, const String &ssidValue);
  void handleRoot();
  void handleRescan();
  void handleSave();
  void handleNotFound();
  static String htmlEscape(const String &text);
};
