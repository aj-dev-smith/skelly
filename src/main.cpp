#include <Arduino.h>
#include <ESP32Servo.h>
#include <I2S.h>

// Servo configuration
Servo jawServo;
const int JAW_OPEN = 25;    // Adjust as necessary
const int JAW_CLOSED = 0;   // Adjust as necessary
const int SERVO_PIN = 5;    // GPIO5 for servo control

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize the servo
  jawServo.attach(SERVO_PIN);
  jawServo.write(JAW_CLOSED);

  // Configure I2S for PDM microphone
  I2S.setAllPins(-1, 42, 41, -1, -1);
  if (!I2S.begin(PDM_MONO_MODE, 16000, 16)) {
    Serial.println("Failed to initialize I2S!");
    while (1);
  }
}

void loop() {
  const int SAMPLE_BUFFER_SIZE = 1024;
  int16_t samples[SAMPLE_BUFFER_SIZE];
  size_t bytesRead = I2S.read(samples, SAMPLE_BUFFER_SIZE);

  if (bytesRead > 0) {
    long sumAbs = 0;
    int numSamples = bytesRead / sizeof(int16_t);

    // Calculate the sum of absolute values to get the amplitude
    for (int i = 0; i < numSamples; i++) {
      sumAbs += abs(samples[i]);
    }

    // Calculate the average amplitude
    int32_t average = sumAbs / numSamples;

    // Map the amplitude to servo position
    int servoPosition = map(average, 1100, 2700, JAW_CLOSED, JAW_OPEN); // Adjust '500' as needed

    // Constrain servo position within limits
    servoPosition = constrain(servoPosition, JAW_CLOSED, JAW_OPEN);

    // Move the servo
    jawServo.write(servoPosition);

    // Debugging output
    Serial.print("Average Amplitude: ");
    Serial.print(average);
    Serial.print(" | Servo Position: ");
    Serial.println(servoPosition);
  }

  // Small delay
  delay(10);
}
