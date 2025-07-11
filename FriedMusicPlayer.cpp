#include <SD.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
//#include "FriedMusicPlayer.h"

#define SPEAKER_PIN 26
#ifndef FRIED_SAMPLE_RATE
#define FRIED_SAMPLE_RATE 44100
#endif

#define MAX_SFX 8

bool sdInitialized = false;

// --- Audio and SFX State ---
File audioFile;
volatile bool audioRunning = false;
volatile bool audioPaused = false;
float volume = 0.5f;
float sfxVolume = 0.5f;

TaskHandle_t audioTaskHandle = NULL;
uint64_t currentSampleRate = FRIED_SAMPLE_RATE;

// --- SFX Buffers ---
struct SoundEffect {
  uint8_t* data;
  size_t length;
};

SoundEffect sfxList[MAX_SFX];
size_t sfxCount = 0;

// Active SFX playback (shared with audioTask)
volatile const uint8_t* activeSfx = nullptr;
volatile size_t activeSfxLen = 0;
volatile size_t activeSfxIndex = 0;
volatile bool activeSfxPlaying = false;

void load_sd()
{
if (!sdInitialized && !SD.begin()) {
  Serial.println("SD init failed in load_sfx!");
  while(1);
}
sdInitialized = true;
}

// --- Load SFX ---
int load_sfx(const char* path) {
  load_sd();
  if (sfxCount >= MAX_SFX) return -1;

  File file = SD.open(path);
  if (!file) return -1;

  size_t size = file.size();
  uint8_t* buffer = (uint8_t*)malloc(size);
  if (!buffer) {
    file.close();
    return -1;
  }

  file.read(buffer, size);
  file.close();

  sfxList[sfxCount].data = buffer;
  sfxList[sfxCount].length = size;

  return sfxCount++;
}

// --- Trigger SFX ---
void play_sfx(uint8_t index) {
  if (index >= sfxCount) return;

  activeSfx = sfxList[index].data;
  activeSfxLen = sfxList[index].length;
  activeSfxIndex = 0;
  activeSfxPlaying = true;
}

// --- Audio Task ---
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

      uint8_t musicSample = 128;
      if (audioFile.available())
      {
        musicSample = audioFile.read();
      }
      else
      {
        audioFile.seek(0); // loop
      }

      int mixed = (int)musicSample - 128;

      // Mix in SFX
      if (activeSfxPlaying && activeSfxIndex < activeSfxLen) {
        int sfxSample = (int)activeSfx[activeSfxIndex++] - 128;
        sfxSample = sfxSample * sfxVolume;
        mixed += sfxSample;

        if (activeSfxIndex >= activeSfxLen) {
          activeSfxPlaying = false;
        }
      }

      mixed = mixed * volume;
      mixed = constrain(mixed, -128, 127);
      uint8_t output = (uint8_t)(mixed + 128);
      dacWrite(SPEAKER_PIN, output);
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

// --- Controls ---
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

void start_audio(const char *path, uint64_t sampleRate = FRIED_SAMPLE_RATE)
{
  stop_audio(); // in case something's already playing

  pinMode(SPEAKER_PIN, OUTPUT);

  load_sd();

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

void pause_audio() { audioPaused = true; }
void resume_audio() { audioPaused = false; }

void set_audio_volume(float newVolume)
{
  if (newVolume < 0.0f) newVolume = 0.0f;
  if (newVolume > 1.0f) newVolume = 1.0f;
  volume = newVolume;
}

void set_sfx_volume(float newVolume) {
  if (newVolume < 0.0f) newVolume = 0.0f;
  if (newVolume > 1.0f) newVolume = 1.0f;
  sfxVolume = newVolume;
}
