#pragma once

int load_sfx(const char* path);
void play_sfx(uint8_t index);

void start_audio(const char *path, uint64_t sampleRate = 44100);
void stop_audio();

void pause_audio();
void resume_audio();

//set volume 0.0f to 1.0f
void set_audio_volume(float newVolume);
void set_sfx_volume(float newVolume);
