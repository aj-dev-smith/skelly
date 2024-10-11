#include <ESP32Servo.h>

Servo jawServo;

const int JAW_OPEN = 25;
const int JAW_CLOSED = 0;
const int SERVO_PIN = 5;        // GPIO5 for servo control

void setup() {
  Serial.begin(115200);

  // Initialize the servo
  jawServo.attach(SERVO_PIN);
  jawServo.write(JAW_CLOSED);
  delay(500);
}

void simulateTalking() {
  int movementPattern[] = {1, 0, 1, 0, 1, 1, 0, 1};
  int patternLength = sizeof(movementPattern) / sizeof(movementPattern[0]);

  for (int i = 0; i < patternLength; i++) {
    if (movementPattern[i] == 1) {
      // Open the jaw and play a tone
      jawServo.write(JAW_OPEN);
      // playTone(1000, 200);  // Play a 1000 Hz tone for 200 ms
    } else {
      // Close the jaw and stop the tone
      jawServo.write(JAW_CLOSED);
     // stopTone();
      delay(200); // Pause for 200 ms
    }

    // Add a random delay to make the movement look natural
    delay(random(100, 300));
  }
}

void loop() {
  Serial.println("Start Talking");

  // Simulate talking by moving the jaw and playing tones
  simulateTalking();

  // Wait before repeating
  delay(2000);

  Serial.println("Talked");
}