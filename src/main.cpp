#include <Arduino.h>
#include "./include/Config.h"
#include "./include/AudioSensor.h"
#include "./include/LedIndicator.h"

AudioSensor audioSensor;
LedIndicator ledIndicator;

void setup() {
  Serial.begin(115200);
  delay(1000);

  ledIndicator.begin();
  audioSensor.begin();

  Serial.println("SETUP FINISHED");
}

void loop() {
  uint32_t level = audioSensor.update();

  Serial.print("level=");
  Serial.println(level);

  ledIndicator.update(level);

  delay(100);
}
