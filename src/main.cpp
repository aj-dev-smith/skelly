#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/dac.h>
#include <ESP32Servo.h>

// Wi-Fi credentials
const char* ssid = "SuperNova";
const char* password = "Shana&AJ2021";

// WAV file URL
const char* wavURL = "https://raw.githubusercontent.com/aj-dev-smith/skelly/main/docs/laugh.wav";

// DAC and Servo pins
const int DAC_PIN = 25;      // GPIO25 (DAC1)
const int SERVO_PIN = 4;     // GPIO4 for servo control

// Servo jaw positions
const int JAW_OPEN_MAX = 25;
const int JAW_CLOSED = 0;

// Variables for audio data
#define BUFFER_SIZE 1024
uint8_t wavHeaderData[44];      // Renamed from wavHeader
uint8_t* audioData = NULL;
uint32_t audioDataSize = 0;

// WAV header structure
struct WavHeader {
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint16_t numChannels;
  uint32_t dataSize;
  uint8_t* dataStart;
};

WavHeader wavHeader;  // This is now unambiguous

volatile uint32_t sampleIndex = 0;

// Timer for audio playback
hw_timer_t* timer = NULL;

// Servo object
Servo jawServo;

// Variables for jaw movement
volatile uint32_t sampleSum = 0;
volatile uint16_t sampleCount = 0;
const uint16_t SAMPLES_PER_WINDOW = 100;

void IRAM_ATTR onTimer() {
  if (sampleIndex < wavHeader.dataSize) {
    uint8_t sampleValue;

    if (wavHeader.bitsPerSample == 8) {
      // 8-bit audio
      sampleValue = wavHeader.dataStart[sampleIndex];
    } else {
      // 16-bit audio, need to convert to 8-bit
      uint16_t sample16 = ((uint16_t*)wavHeader.dataStart)[sampleIndex];
      sampleValue = sample16 >> 8;  // Convert to 8-bit
    }

    dac_output_voltage(DAC_CHANNEL_1, sampleValue);
    sampleIndex++;

    // Accumulate sample values for jaw movement
    sampleSum += abs(sampleValue - 128);  // Centered at 128
    sampleCount++;

    if (sampleCount >= SAMPLES_PER_WINDOW) {
      // Compute average amplitude
      uint16_t avgAmplitude = sampleSum / sampleCount;

      // Map amplitude to servo angle
      int jawAngle = map(avgAmplitude, 0, 127, JAW_CLOSED, JAW_OPEN_MAX);
      jawServo.write(jawAngle);

      // Reset counters
      sampleSum = 0;
      sampleCount = 0;
    }
  } else {
    // Stop playback
    timerAlarmDisable(timer);
    dac_output_voltage(DAC_CHANNEL_1, 128);  // Midpoint voltage
    jawServo.write(JAW_CLOSED);  // Close the jaw
  }
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" Connected!");
}

bool downloadWavFile() {
  HTTPClient http;
  http.begin(wavURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    int totalLength = http.getSize();

    // Read the WAV header
    WiFiClient* stream = http.getStreamPtr();
    int bytesRead = stream->readBytes(wavHeaderData, 44);

    if (bytesRead != 44) {
      Serial.println("Failed to read WAV header");
      http.end();
      return false;
    }

    // Parse WAV header to get audio data size
    audioDataSize = totalLength - 44;

    // Check if audio data size is manageable
    if (audioDataSize > 400000) {
      Serial.println("Audio file too large to fit in memory");
      http.end();
      return false;
    }

    // Allocate memory for audio data
    audioData = (uint8_t*)malloc(audioDataSize);
    if (!audioData) {
      Serial.println("Failed to allocate memory for audio data");
      http.end();
      return false;
    }

    // Read audio data
    uint32_t bytesRemaining = audioDataSize;
    uint32_t bytesReceived = 0;
    while (bytesRemaining > 0) {
      size_t toRead = bytesRemaining > BUFFER_SIZE ? BUFFER_SIZE : bytesRemaining;
      int bytesRead = stream->readBytes(&audioData[bytesReceived], toRead);
      if (bytesRead <= 0) {
        Serial.println("Failed to read audio data");
        free(audioData);
        http.end();
        return false;
      }
      bytesReceived += bytesRead;
      bytesRemaining -= bytesRead;
      Serial.print(".");  // Progress indicator
    }
    Serial.println("\nDownload complete");
    http.end();
    return true;
  } else {
    Serial.printf("HTTP GET failed, error: %d\n", httpCode);
    http.end();
    return false;
  }
}

bool parseWavHeader(WavHeader& header) {
  // Check "RIFF" chunk descriptor
  if (strncmp((char*)wavHeaderData, "RIFF", 4) != 0) {
    Serial.println("Not a valid WAV file (RIFF header missing)");
    return false;
  }

  // Check "WAVE" format
  if (strncmp((char*)(wavHeaderData + 8), "WAVE", 4) != 0) {
    Serial.println("Not a valid WAV file (WAVE format missing)");
    return false;
  }

  // Find "fmt " subchunk
  uint32_t subchunk1ID = *(uint32_t*)(wavHeaderData + 12);
  if (subchunk1ID != 0x20746d66) {  // "fmt " in little endian
    Serial.println("Invalid WAV file (fmt chunk missing)");
    return false;
  }

  // Parse format details
  uint16_t audioFormat = *(uint16_t*)(wavHeaderData + 20);
  header.numChannels = *(uint16_t*)(wavHeaderData + 22);
  header.sampleRate = *(uint32_t*)(wavHeaderData + 24);
  header.bitsPerSample = *(uint16_t*)(wavHeaderData + 34);

  if (audioFormat != 1) {
    Serial.println("Unsupported WAV format (not PCM)");
    return false;
  }

  if (header.bitsPerSample != 8 && header.bitsPerSample != 16) {
    Serial.println("Unsupported bits per sample");
    return false;
  }

  // Find "data" subchunk
  uint32_t subchunk2ID = *(uint32_t*)(wavHeaderData + 36);
  if (subchunk2ID != 0x61746164) {  // "data" in little endian
    Serial.println("Invalid WAV file (data chunk missing)");
    return false;
  }

  header.dataSize = *(uint32_t*)(wavHeaderData + 40);
  header.dataStart = audioData;

  return true;
}

void startAudioPlayback() {
  sampleIndex = 0;

  // Configure timer
  uint32_t timerFrequency = wavHeader.sampleRate;
  timer = timerBegin(0, 80, true);  // Prescaler 80 for 1 MHz base (80 MHz / 80)
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000 / timerFrequency, true);  // Alarm every sample
  timerAlarmEnable(timer);
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();

  // Initialize DAC output
  dac_output_enable(DAC_CHANNEL_1);

  // Initialize servo
  jawServo.attach(SERVO_PIN);
  jawServo.write(JAW_CLOSED);

  Serial.println("Starting WAV file download...");

  // Download WAV file
  if (downloadWavFile()) {
    Serial.println("WAV file downloaded successfully.");
    if (parseWavHeader(wavHeader)) {
      Serial.println("WAV header parsed successfully.");
      Serial.printf("Sample Rate: %u Hz\n", wavHeader.sampleRate);
      Serial.printf("Bits Per Sample: %u\n", wavHeader.bitsPerSample);
      Serial.printf("Number of Channels: %u\n", wavHeader.numChannels);
      Serial.printf("Data Size: %u bytes\n", wavHeader.dataSize);

      Serial.println("Starting playback...");
      startAudioPlayback();
    } else {
      Serial.println("Failed to parse WAV header.");
    }
  } else {
    Serial.println("Failed to download WAV file.");
  }
}


void loop() {
  // Do nothing, playback handled in timer ISR
}