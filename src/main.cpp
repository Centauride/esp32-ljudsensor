/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-led-strip
 */


#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_STRIP_PIN  5  // The ESP32 pin GPIO16 connected to NeoPixel
#define NUM_PIXELS     3  // The number of LEDs (pixels) on NeoPixel

Adafruit_NeoPixel strip(NUM_PIXELS, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800);


void setup() {
  strip.begin();           // INITIALIZE NeoPixel strip object
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set brightness (0 to 255)
}

// the loop function runs over and over again forever
void loop() {
  // Fill all pixels with red color (Red, Green, Blue)
  strip.fill(strip.Color(255, 0, 0)); 
  strip.show();            // Send the updated pixel colors to the hardware
  delay(1000);             // Wait 1 second
}


