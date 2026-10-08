// #include <Arduino.h>

// // 1. Define I²S pins (corresponding to hardware connections)
// #define I2S_SCK 2
// #define I2S_WS 3 // TODO: SD = 3
// #define I2S_SD 4 // TODO: WS = 4

// // 2. Define sound threshold and LED pin
// const int soundThreshold = 500;  // Adjust based on environment (higher = more sensitive to loud sounds)
// const int ledPin = 13;           // Arduino's built-in LED pin (for easy debugging)

// // 3. Initialize I2S object
// Adafruit_I2S i2s;

// void setup() {
//   Serial.begin(115200);          // Initialize serial communication at 115200 baud rate (for data display)
//   pinMode(ledPin, OUTPUT);       // Set LED pin as output

//   // 4. Initialize INMP441 (configure sample rate, channels, bit depth)
//   if (!i2s.begin(I2S_SCK, I2S_WS, I2S_SD, I2S_NO_MCLK, 16000, 16, 1)) {
//     Serial.println("INMP441 initialization failed! Please check wiring.");
//     while (1);  // Halt if initialization fails, prompting user to troubleshoot
//   }
//   Serial.println("INMP441 + Arduino Sound Monitor started");
//   Serial.print("Sound threshold: ");
//   Serial.println(soundThreshold);
// }

// void loop() {
//   int16_t audioData;  // Variable to store 16-bit audio data from microphone
//   size_t bytesRead;   // Variable to store number of bytes read

//   // 5. Read audio data from INMP441
//   bytesRead = i2s.read(&audioData, sizeof(audioData));

//   // 6. Process data: take absolute value to eliminate positive/negative signal difference
//   int soundIntensity = abs(audioData);

//   // 7. Display sound intensity on serial monitor (for real-time observation)
//   Serial.print("Current sound intensity: ");
//   Serial.println(soundIntensity);

//   // 8. Sound threshold check: flash LED if intensity exceeds threshold
//   if (soundIntensity > soundThreshold) {
//     digitalWrite(ledPin, HIGH);  // Turn LED on
//     delay(100);                  // Keep on for 100ms
//     digitalWrite(ledPin, LOW);   // Turn LED off
//     delay(100);                  // Keep off for 100ms (creates blinking effect)
//   } else {
//     digitalWrite(ledPin, LOW);   // Keep LED off if below threshold
//   }

//   delay(50);  // Control loop frequency to prevent serial monitor spam
// }