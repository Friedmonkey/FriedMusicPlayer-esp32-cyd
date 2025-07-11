#pragma once

void start_audio(const char *path, uint32_t sampleRate = 44100);
void stop_audio();

void pause_audio();
void resume_audio();

//set volume 0.0f to 1.0f
void set_audio_volume(float newVolume);
