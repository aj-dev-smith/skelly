#include <Arduino.h>
#include <ESP32Servo.h>
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

// Create an instance of the A2DP sink
AnalogAudioStream out;                   // For audio output via internal DAC
BluetoothA2DPSink a2dp_sink(out);        // Pass the `out` to the A2DP sink


// Servo configuration
Servo jawServo;
const int JAW_OPEN = 25;     // Adjust as necessary
const int JAW_CLOSED = 0;    // Adjust as necessary
const int SERVO_PIN = 4;     // GPIO4 for servo control

// Audio processing variables
volatile int16_t audio_sample = 0;
volatile bool new_audio_sample = false;

// Callback function to process received audio data
void audio_data_callback(const uint8_t *data, uint32_t length) {
  // Cast the data to 16-bit samples
  const int16_t *samples = reinterpret_cast<const int16_t*>(data);
  uint32_t num_samples = length / sizeof(int16_t);

  // Process only the left channel for servo control
  for (uint32_t i = 0; i < num_samples; i += 2) { // i += 2 to skip right channel
    audio_sample = samples[i]; // Left channel
    new_audio_sample = true;
  }

  // Write the audio data to the AnalogAudioStream for playback
  out.write(data, length);
}


void setup() {
  Serial.begin(115200);
  Serial.print("Starting setup");
  while (!Serial);

  // Initialize the servo
  jawServo.attach(SERVO_PIN);
  jawServo.write(JAW_CLOSED);

  // Set the audio data callback
  a2dp_sink.set_stream_reader(audio_data_callback);

  // Start the Bluetooth A2DP sink
  a2dp_sink.start("Skeleton Head"); // This is the Bluetooth name that will appear on your phone

  Serial.print("Complete setup.  Bluetooth connection available as 'Skeleton Head'");
}


void loop() {
  if (new_audio_sample) {
    // Get the absolute value to represent amplitude
    int amplitude = abs(audio_sample);

    // Map the amplitude to the servo position
    int servoPosition = map(amplitude, 0, 1500, JAW_CLOSED, JAW_OPEN);
    servoPosition = constrain(servoPosition, JAW_CLOSED, JAW_OPEN);

    // Move the servo
    jawServo.write(servoPosition);

    // Reset the flag
    new_audio_sample = false;

    // Optional: Debugging output
    Serial.print("Amplitude: ");
    Serial.print(amplitude);
    Serial.print(" | Servo Position: ");
    Serial.println(servoPosition);
  }
}
