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
volatile bool audioPaused = false;
float volume = 0.5f;

TaskHandle_t audioTaskHandle = NULL;
uint64_t currentSampleRate = FRIED_SAMPLE_RATE;

void audioTask(void *pvParameters)
{
  const uint64_t yieldInterval = 1000000; // 1000ms
  uint64_t previousMicros = esp_timer_get_time();
  uint64_t lastYieldMicros = previousMicros;
  uint64_t sampleInterval = 1000000 / currentSampleRate;

  while (audioRunning)
  {
    uint64_t currentMicros = esp_timer_get_time();

    if (!audioPaused && currentMicros - previousMicros >= sampleInterval)
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
        audioFile.seek(0); // loop
      }
    }

    // Yield occasionally
    if (currentMicros - lastYieldMicros >= yieldInterval)
    {
      vTaskDelay(1); // ~1ms
      lastYieldMicros = currentMicros;
    }
  }

  dacWrite(SPEAKER_PIN, 0);
  vTaskDelete(NULL);
}

void stop_audio()
{
  audioRunning = false;
  audioPaused = false;

  if (audioTaskHandle != NULL)
  {
    vTaskDelete(audioTaskHandle);
    audioTaskHandle = NULL;
  }

  if (audioFile)
  {
    audioFile.close();
  }

  dacWrite(SPEAKER_PIN, 0);
}

void start_audio(const char *path, uint32_t sampleRate = FRIED_SAMPLE_RATE)
{
  stop_audio(); // in case something's already playing

  pinMode(SPEAKER_PIN, OUTPUT);

  if (!SD.begin())
  {
    Serial.println("SD init failed!");
    while (1);
  }

  audioFile = SD.open(path);
  if (!audioFile)
  {
    Serial.println("Failed to open audio file");
    while (1);
  }

  currentSampleRate = sampleRate;
  audioPaused = false;
  audioRunning = true;

  xTaskCreatePinnedToCore(
      audioTask,
      "AudioTask",
      2048,
      NULL,
      1,
      &audioTaskHandle,
      0
  );
}

void pause_audio()
{
  audioPaused = true;
}

void resume_audio()
{
  audioPaused = false;
}

void set_audio_volume(float newVolume)
{
  if (newVolume < 0.0f) newVolume = 0.0f;
  if (newVolume > 1.0f) newVolume = 1.0f;
  volume = newVolume;
}
