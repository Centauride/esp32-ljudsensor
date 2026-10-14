#pragma once
#include <Arduino.h>

// Keeps the home WiFi name and password in the board's flash memory, so they
// survive restarts and never have to be written in the source code.
class WifiCredentials {
public:
  // Fills ssid/password and returns true if a network has been saved.
  bool load(String &ssid, String &password);

  // Saves a new network. It counts as "unverified" until a connection with it
  // has actually worked (see markVerified).
  void save(const String &ssid, const String &password);

  // True once a connection with the saved network has worked at least once.
  bool isVerified();
  void markVerified();

  // Forgets the saved network (the next boot opens the setup portal).
  void clear();
};
