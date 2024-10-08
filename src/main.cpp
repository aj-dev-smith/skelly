#include <ESP32Servo.h>
#include <WiFi.h>
#include <Audio.h>

// Wi-Fi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Audio settings
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

Servo jawServo;

// Jaw positions
const int JAW_CLOSED = 0;
const int JAW_MAX_OPEN = 25;
const int SERVO_PIN = 4;

// Audio streaming URL
const char* mp3URL = "http://www.example.com/laugh.mp3";

void setup() {
  Serial.begin(115200);

  // Set up Wi-Fi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // Set up the servo
  jawServo.attach(SERVO_PIN);
  jawServo.write(JAW_CLOSED);

  // Set up audio output
  out = new AudioOutputI2S(0, 1); // Using DAC channel 1 (GPIO 25)
  out->SetGain(0.5); // Adjust volume as needed

  // Set up audio file from HTTP stream
  file = new AudioFileSourceHTTPStream(mp3URL);
  mp3 = new AudioGeneratorMP3();

  mp3->begin(file, out);
}

void loop() {
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
    } else {
      // Get current volume level
      int level = out->getLevel(); // Assuming getLevel() gives audio amplitude level
      
      // Map volume level to jaw position
      int jawPosition = map(level, 0, 1023, JAW_CLOSED, JAW_MAX_OPEN);
      jawServo.write(jawPosition);

      delay(10); // Small delay to avoid overwhelming the servo
    }
  } else {
    Serial.println("Playback finished, restarting...");
    delay(2000);
    mp3->begin(file, out);
  }
}
