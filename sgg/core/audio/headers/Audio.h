/*
 * Simple-SDL2-Audio (Optimized)
 *
 * Copyright 2016 Jake Besworth
 * Optimizations added 2025
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SIMPLE_AUDIO_H
#define SIMPLE_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include <stdbool.h>

// Audio configuration constants
#define AUDIO_FORMAT        AUDIO_S16LSB
#define AUDIO_FREQUENCY     48000
#define AUDIO_CHANNELS      2
#define AUDIO_SAMPLES       4096
#define AUDIO_MAX_SOUNDS    25
#define AUDIO_MAX_VOLUME    128

// Audio states for better readability
typedef enum {
    AUDIO_STATE_STOPPED = 0,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_FADING_OUT,
    AUDIO_STATE_PAUSED
} AudioState;

// Optimized Audio structure with better memory alignment
typedef struct Audio {
    // Audio data (keep together for cache efficiency)
    uint8_t* buffer;
    uint8_t* buffer_original;
    uint32_t length;
    uint32_t length_original;

    // Audio properties
    SDL_AudioSpec spec;

    // Flags and state (packed for memory efficiency)
    uint8_t volume;
    AudioState state : 3;
    bool is_music : 1;
    bool should_loop : 1;
    bool owns_buffer : 1;
    bool _padding : 2;  // Explicit padding for clarity

    // Linked list pointer
    struct Audio* next;
} Audio;

// Audio system initialization result
typedef enum {
    AUDIO_INIT_SUCCESS = 0,
    AUDIO_INIT_ERROR_MEMORY,
    AUDIO_INIT_ERROR_SDL_NOT_INITIALIZED,
    AUDIO_INIT_ERROR_DEVICE_OPEN_FAILED
} AudioInitResult;

// Core audio system functions
AudioInitResult initAudio(void);
void endAudio(void);
void pauseAudio(void);
void unpauseAudio(void);
bool isAudioInitialized(void);

// Audio creation and management
Audio* createAudio(const char* filename, bool is_music, uint8_t volume);
void freeAudio(Audio* audio);

// Playback functions with clearer naming
void playSound(const char* filename, uint8_t volume);
void playMusic(const char* filename, uint8_t volume);
void playSoundFromMemory(const Audio* audio, uint8_t volume);
void playMusicFromMemory(const Audio* audio, uint8_t volume);

// Additional utility functions
void stopAllSounds(void);
void stopMusic(void);
void setMasterVolume(uint8_t volume);
uint8_t getMasterVolume(void);
int getActiveSoundCount(void);

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_AUDIO_H