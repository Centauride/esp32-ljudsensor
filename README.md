# ESP32-C3 Sound Level Indicator

Reads ambient sound level from an INMP441 I2S microphone and displays it on a
3-LED NeoPixel bar graph (green / orange / red). Thresholds can be tuned live
from a web page hosted by the ESP32 itself. WiFi is set up from a phone, with
no code changes.

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
> with L/R tied to GND. If you swap mic boards, you may need to flip
> `I2S_CHANNEL_FMT_ONLY_RIGHT` back to `ONLY_LEFT` in `AudioSensor.cpp`.

## Build and upload

Open this folder as a PlatformIO project (`platformio.ini` at the root), with
headers in `include/` and `.cpp` files in `src/`, then Upload. There are no
credentials to put in the code.

## First-time WiFi setup

1. Power the board. With no WiFi saved, the LEDs pulse slowly in **blue**.
2. On your phone, join the WiFi network **SoundMeter-Setup** (no password).
   The setup page opens by itself. If it doesn't, open `http://192.168.4.1`.
3. Pick your home network, type its password and press **Save and connect**.
4. The board restarts and joins your network (a purple dot runs across the LEDs).
5. Put your phone back on your usual WiFi and open `http://soundmeter.local`
   to see the live graph and adjust the thresholds.

If the password was wrong, the LEDs go back to slow blue pulsing and the setup
network appears again: just repeat the steps.

If nobody uses the setup page for 10 minutes, the meter carries on without WiFi
(the LEDs still show the sound level). Restart it to open the portal again.

### Changing network / starting over

- **Short press** of the BOOT button: try to reconnect to the saved network.
- **Hold BOOT for 5 seconds** until the LEDs flash red, then let go: the saved
  network is erased and the board restarts into the setup portal.

Don't hold BOOT while plugging the board in or pressing RESET: that puts the
board into upload mode.

If the saved network stops working later (for example after a power cut while
the router is still starting), the meter keeps showing sound levels and retries
in the background every 30 seconds.

## Project structure

| File | Responsibility |
|---|---|
| `Config.h` / `Config.cpp` | Pins, colors, timeouts, portal name, tunable thresholds |
| `AudioSensor.h` / `.cpp` | I2S mic driver + smoothed RMS sound level |
| `LedIndicator.h` / `.cpp` | Drives the 3-LED bar and the setup/loading animations |
| `WifiCredentials.h` / `.cpp` | Saves the WiFi name and password in flash |
| `WifiProvisioner.h` / `.cpp` | The setup portal (own WiFi network + setup page) |
| `SettingsServer.h` / `.cpp` | Connects to WiFi and serves the live graph and sliders |
| `main.cpp` | Wires everything together in `setup()` / `loop()` |

## How the LED bar works

- **LED 0**: always green (power/baseline indicator)
- **LED 1**: orange once sound crosses the "quiet to normal" threshold
- **LED 2**: red once sound crosses the "normal to loud" threshold

Other animations: purple dot running = connecting to WiFi, slow blue pulse =
setup portal open, three red flashes = saved WiFi erased.

## Tuning thresholds

Use the web page (see above): drag the sliders while watching the live level
update in real time.

The red threshold is always kept at least 10000 above the orange one (raising
the orange slider pushes it up automatically), and **Reset thresholds** restores
the defaults. Both the defaults and the minimum gap are set in `Config.h`.
