#include <SD.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define SPEAKER_PIN 26
#ifndef FRIED_SAMPLE_RATE
#define FRIED_SAMPLE_RATE 44100
#endif

File audioFile;
volatile bool audioRunning = false;
float volume = 0.5f;

TaskHandle_t audioTaskHandle = NULL;

void audioTask(void *pvParameters)
{
  const uint64_t sampleInterval = 1000000 / FRIED_SAMPLE_RATE; // ~22.7us for 44.1kHz
  const uint64_t yieldInterval = 1000000;  // yield every 1000ms

  uint64_t previousMicros = esp_timer_get_time();
  uint64_t lastYieldMicros = previousMicros;

  while (audioRunning)
  {
    uint64_t currentMicros = esp_timer_get_time();

    // Play audio sample
    if (currentMicros - previousMicros >= sampleInterval)
    {
      previousMicros += sampleInterval;

      if (audioFile.available())
      {
        uint8_t sample = audioFile.read();
        int centered = (int)sample - 128;
        centered = centered * volume;
        sample = (uint8_t)(centered + 128);
        dacWrite(SPEAKER_PIN, sample);
      }
      else
      {
        audioFile.seek(0);  // loop the audio file
      }
    }

    // Yield occasionally to feed watchdog
    if (currentMicros - lastYieldMicros >= yieldInterval)
    {
      vTaskDelay(1);  // ~1ms delay
      lastYieldMicros = currentMicros;
    }
  }

  dacWrite(SPEAKER_PIN, 0);
  vTaskDelete(NULL);
}


void setup_audio(const char *path)
{
  pinMode(SPEAKER_PIN, OUTPUT);

  if (!SD.begin())
  {
    Serial.println("SD init failed!");
    while (1)
      ;
  }

  audioFile = SD.open(path);
  if (!audioFile)
  {
    Serial.println("Failed to open audio file");
    while (1)
      ;
  }
  Serial.println("Audio setup complete!");

  audioRunning = true;

  xTaskCreatePinnedToCore(
      audioTask,       // task function
      "AudioTask",     // name
      2048,            // stack size
      NULL,            // parameters
      1,               // priority
      &audioTaskHandle, // task handle
      0                // core 0
  );
}

void stop_audio()
{
  if (audioRunning)
  {
    audioRunning = false;
  }
}
void set_audio_volume(float newVolume) {
  if (newVolume < 0.0f) newVolume = 0.0f;
  if (newVolume > 1.0f) newVolume = 1.0f;
  volume = newVolume;
}

