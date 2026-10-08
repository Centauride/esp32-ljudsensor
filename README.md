# ESP32-C3 Sound Level Indicator

Reads ambient sound level from an INMP441 I2S microphone and displays it on a
3-LED NeoPixel bar graph (green / orange / red). Thresholds can be tuned live
from a web page hosted by the ESP32 itself.

## Hardware

- ESP32-C3 (mini dev board)
- INMP441 I2S microphone
- 3x addressable LEDs (NeoPixel / WS2812-compatible)

### Wiring

| INMP441 | ESP32-C3 |
|---|---|
| VDD | 3.3V |
| GND | GND |
| L/R | GND |
| WS  | GPIO4 |
| SCK | GPIO2 |
| SD  | GPIO3 |

| LED strip | ESP32-C3 |
|---|---|
| Data in | GPIO5 |

> This specific INMP441 board outputs audio on the **right** I2S channel even
> with L/R tied to GND — if you swap mic boards, you may need to flip
> `I2S_CHANNEL_FMT_ONLY_RIGHT` back to `ONLY_LEFT` in `AudioSensor.cpp`.

## Setup

1. Open this folder as a PlatformIO project (`platformio.ini` at the root).
2. In `Config.h`, set your WiFi credentials:
   ```cpp
   #define WIFI_SSID       "your_wifi_name"
   #define WIFI_PASSWORD   "your_wifi_password"
   ```
3. Build and upload (PlatformIO sidebar → Upload, or the → icon in the status bar).
4. Open Serial Monitor (115200 baud) — it prints the IP address to visit, e.g.:
   ```
   Connected. Open this in a browser: http://192.168.1.42
   ```
5. Visit that address from any device on the same WiFi network to see live
   sound level + adjust the two thresholds with sliders.

## Project structure

| File | Responsibility |
|---|---|
| `Config.h` / `Config.cpp` | Pins, colors, WiFi credentials, tunable thresholds |
| `AudioSensor.h` / `.cpp` | I2S mic driver + smoothed RMS sound level |
| `LedIndicator.h` / `.cpp` | Drives the 3-LED bar graph from a sound level |
| `SettingsServer.h` / `.cpp` | Hosts the web UI for live threshold tuning |
| `main.cpp` | Wires everything together in `setup()` / `loop()` |

## How the LED bar works

- **LED 0** — always green (power/baseline indicator)
- **LED 1** — orange once sound crosses the "quiet → normal" threshold
- **LED 2** — red once sound crosses the "normal → loud" threshold

## Tuning thresholds

Easiest from the web UI (see Setup above) — drag the sliders while watching
the live level reading update in real time.