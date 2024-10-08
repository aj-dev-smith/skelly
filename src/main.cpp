#include <ESP32Servo.h>
#include <driver/dac.h>

Servo jawServo;

const int JAW_OPEN = 25;
const int JAW_CLOSED = 0;
const int SERVO_PIN = 4;        // GPIO4 for servo control
const int DAC_PIN = 25;         // GPIO25 (DAC1) for audio output

void setup() {
  Serial.begin(115200);

  // Initialize the servo
  jawServo.attach(SERVO_PIN);
  jawServo.write(JAW_CLOSED);
  delay(500);

  // Initialize DAC output
  dac_output_enable(DAC_CHANNEL_1); // DAC_CHANNEL_1 corresponds to GPIO25
}

void playTone(int frequency, int duration) {
  int sampleRate = 8000; // Sample rate in Hz
  int samples = duration * (sampleRate / 1000); // Total number of samples
  double increment = (2 * PI * frequency) / sampleRate;
  double angle = 0;

  for (int i = 0; i < samples; i++) {
    uint8_t dac_value = (uint8_t)((sin(angle) + 1) * 127); // Convert sine wave to 0-255
    dac_output_voltage(DAC_CHANNEL_1, dac_value);
    angle += increment;
    delayMicroseconds(1000000 / sampleRate);
  }
}

void stopTone() {
  dac_output_voltage(DAC_CHANNEL_1, 128); // Midpoint voltage (no sound)
}

void simulateTalking() {
  int movementPattern[] = {1, 0, 1, 0, 1, 1, 0, 1};
  int patternLength = sizeof(movementPattern) / sizeof(movementPattern[0]);

  for (int i = 0; i < patternLength; i++) {
    if (movementPattern[i] == 1) {
      // Open the jaw and play a tone
      jawServo.write(JAW_OPEN);
      playTone(1000, 200);  // Play a 1000 Hz tone for 200 ms
    } else {
      // Close the jaw and stop the tone
      jawServo.write(JAW_CLOSED);
      stopTone();
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
