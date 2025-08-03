/*
 * Simple-SDL2-Audio (Optimized Implementation)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "core/audio/headers/Audio.h"

// Device structure for better organization
typedef struct {
    SDL_AudioDeviceID device_id;
    SDL_AudioSpec desired_spec;
    Audio* audio_queue;  // Root of audio queue (placeholder node)
    uint8_t master_volume;
    uint32_t active_sound_count;
    bool initialized;
} AudioDevice;

// Global device instance
static AudioDevice g_device = {0};

// Forward declarations
static void audioCallback(void* userdata, uint8_t* stream, int len);
static void addAudioToQueue(Audio* new_audio);
static void addMusicToQueue(Audio* new_audio);
static Audio* cloneAudio(const Audio* source, bool is_music, uint8_t volume);
static void fadeOutCurrentMusic(void);

// Inline helper functions for better performance
static inline uint32_t min_u32(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

static inline uint8_t scaleVolume(uint8_t volume, uint8_t master) {
    return (uint8_t)((volume * master) / AUDIO_MAX_VOLUME);
}

AudioInitResult initAudio(void) {
    // Check if already initialized
    if (g_device.initialized) {
        return AUDIO_INIT_SUCCESS;
    }

    // Verify SDL audio subsystem is initialized
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        fprintf(stderr, "Error: SDL audio subsystem not initialized\n");
        return AUDIO_INIT_ERROR_SDL_NOT_INITIALIZED;
    }

    // Initialize device structure
    memset(&g_device, 0, sizeof(AudioDevice));
    g_device.master_volume = AUDIO_MAX_VOLUME;

    // Create placeholder root node for audio queue
    g_device.audio_queue = (Audio*)calloc(1, sizeof(Audio));
    if (!g_device.audio_queue) {
        fprintf(stderr, "Error: Failed to allocate memory for audio queue\n");
        return AUDIO_INIT_ERROR_MEMORY;
    }

    // Configure desired audio specification
    g_device.desired_spec.freq = AUDIO_FREQUENCY;
    g_device.desired_spec.format = AUDIO_FORMAT;
    g_device.desired_spec.channels = AUDIO_CHANNELS;
    g_device.desired_spec.samples = AUDIO_SAMPLES;
    g_device.desired_spec.callback = audioCallback;
    g_device.desired_spec.userdata = &g_device;

    // Open audio device
    g_device.device_id = SDL_OpenAudioDevice(
        NULL, 0, &g_device.desired_spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE
    );

    if (g_device.device_id == 0) {
        fprintf(stderr, "Warning: Failed to open audio device: %s\n", SDL_GetError());
        free(g_device.audio_queue);
        return AUDIO_INIT_ERROR_DEVICE_OPEN_FAILED;
    }

    g_device.initialized = true;
    unpauseAudio();

    return AUDIO_INIT_SUCCESS;
}

void endAudio(void) {
    if (!g_device.initialized) {
        return;
    }

    pauseAudio();

    // Free all audio in queue
    if (g_device.audio_queue) {
        freeAudio(g_device.audio_queue);
        g_device.audio_queue = NULL;
    }

    // Close audio device
    SDL_CloseAudioDevice(g_device.device_id);

    // Reset device state
    memset(&g_device, 0, sizeof(AudioDevice));
}

void pauseAudio(void) {
    if (g_device.initialized) {
        SDL_PauseAudioDevice(g_device.device_id, 1);
    }
}

void unpauseAudio(void) {
    if (g_device.initialized) {
        SDL_PauseAudioDevice(g_device.device_id, 0);
    }
}

bool isAudioInitialized(void) {
    return g_device.initialized;
}

Audio* createAudio(const char* filename, bool is_music, uint8_t volume) {
    if (!filename) {
        fprintf(stderr, "Error: Filename is NULL\n");
        return NULL;
    }

    Audio* audio = (Audio*)calloc(1, sizeof(Audio));
    if (!audio) {
        fprintf(stderr, "Error: Failed to allocate memory for Audio\n");
        return NULL;
    }

    // Load WAV file
    if (!SDL_LoadWAV(filename, &audio->spec, &audio->buffer_original, &audio->length_original)) {
        fprintf(stderr, "Error: Failed to load WAV file '%s': %s\n", filename, SDL_GetError());
        free(audio);
        return NULL;
    }

    // Initialize audio properties
    audio->buffer = audio->buffer_original;
    audio->length = audio->length_original;
    audio->volume = volume;
    audio->state = AUDIO_STATE_STOPPED;
    audio->is_music = is_music;
    audio->should_loop = is_music;  // Music loops by default
    audio->owns_buffer = true;
    audio->next = NULL;

    // Clear callback data (not used in queue system)
    audio->spec.callback = NULL;
    audio->spec.userdata = NULL;

    return audio;
}

void freeAudio(Audio* audio) {
    while (audio) {
        Audio* next = audio->next;

        if (audio->owns_buffer && audio->buffer_original) {
            SDL_FreeWAV(audio->buffer_original);
        }

        free(audio);
        audio = next;
    }
}

void playSound(const char* filename, uint8_t volume) {
    if (!g_device.initialized || !filename) {
        return;
    }

    // Check sound limit
    if (g_device.active_sound_count >= AUDIO_MAX_SOUNDS) {
        return;
    }

    Audio* audio = createAudio(filename, false, volume);
    if (audio) {
        audio->state = AUDIO_STATE_PLAYING;
        addAudioToQueue(audio);
        g_device.active_sound_count++;
    }
}

void playMusic(const char* filename, uint8_t volume) {
    if (!g_device.initialized || !filename) {
        return;
    }

    Audio* audio = createAudio(filename, true, volume);
    if (audio) {
        audio->state = AUDIO_STATE_PLAYING;
        addMusicToQueue(audio);
    }
}

void playSoundFromMemory(const Audio* source, uint8_t volume) {
    if (!g_device.initialized || !source) {
        return;
    }

    if (g_device.active_sound_count >= AUDIO_MAX_SOUNDS) {
        return;
    }

    Audio* audio = cloneAudio(source, false, volume);
    if (audio) {
        audio->state = AUDIO_STATE_PLAYING;
        addAudioToQueue(audio);
        g_device.active_sound_count++;
    }
}

void playMusicFromMemory(const Audio* source, uint8_t volume) {
    if (!g_device.initialized || !source) {
        return;
    }

    Audio* audio = cloneAudio(source, true, volume);
    if (audio) {
        audio->state = AUDIO_STATE_PLAYING;
        addMusicToQueue(audio);
    }
}

void stopAllSounds(void) {
    if (!g_device.initialized) {
        return;
    }

    SDL_LockAudioDevice(g_device.device_id);

    Audio* current = g_device.audio_queue->next;
    while (current) {
        if (!current->is_music) {
            current->state = AUDIO_STATE_STOPPED;
            current->length = 0;
        }
        current = current->next;
    }

    SDL_UnlockAudioDevice(g_device.device_id);
}

void stopMusic(void) {
    if (!g_device.initialized) {
        return;
    }

    SDL_LockAudioDevice(g_device.device_id);

    Audio* current = g_device.audio_queue->next;
    while (current) {
        if (current->is_music) {
            current->state = AUDIO_STATE_STOPPED;
            current->length = 0;
        }
        current = current->next;
    }

    SDL_UnlockAudioDevice(g_device.device_id);
}

void setMasterVolume(uint8_t volume) {
    g_device.master_volume = min_u32(volume, AUDIO_MAX_VOLUME);
}

uint8_t getMasterVolume(void) {
    return g_device.master_volume;
}

int getActiveSoundCount(void) {
    return (int)g_device.active_sound_count;
}

// Static helper functions implementation
static void addAudioToQueue(Audio* new_audio) {
    SDL_LockAudioDevice(g_device.device_id);

    // Find end of queue and append
    Audio* current = g_device.audio_queue;
    while (current->next) {
        current = current->next;
    }
    current->next = new_audio;

    SDL_UnlockAudioDevice(g_device.device_id);
}

static void addMusicToQueue(Audio* new_audio) {
    SDL_LockAudioDevice(g_device.device_id);

    // Fade out existing music
    fadeOutCurrentMusic();

    // Add new music to queue
    Audio* current = g_device.audio_queue;
    while (current->next) {
        current = current->next;
    }
    current->next = new_audio;

    SDL_UnlockAudioDevice(g_device.device_id);
}

static void fadeOutCurrentMusic(void) {
    Audio* current = g_device.audio_queue->next;
    while (current) {
        if (current->is_music && current->state == AUDIO_STATE_PLAYING) {
            current->state = AUDIO_STATE_FADING_OUT;
        }
        current = current->next;
    }
}

static Audio* cloneAudio(const Audio* source, bool is_music, uint8_t volume) {
    Audio* clone = (Audio*)malloc(sizeof(Audio));
    if (!clone) {
        fprintf(stderr, "Error: Failed to allocate memory for audio clone\n");
        return NULL;
    }

    // Copy audio data
    memcpy(clone, source, sizeof(Audio));

    // Override specific properties
    clone->volume = volume;
    clone->is_music = is_music;
    clone->should_loop = is_music;
    clone->owns_buffer = false;  // Clone doesn't own the buffer
    clone->next = NULL;
    clone->state = AUDIO_STATE_STOPPED;

    return clone;
}

static void audioCallback(void* userdata, uint8_t* stream, int len) {
    AudioDevice* device = (AudioDevice*)userdata;
    Audio* current = device->audio_queue;
    Audio* previous = current;
    uint32_t bytes_to_mix;
    bool music_fading = false;

    // Clear output buffer
    SDL_memset(stream, 0, len);

    // Skip placeholder root node
    current = current->next;

    while (current) {
        if (current->length > 0 && current->state != AUDIO_STATE_STOPPED) {
            // Handle fading music
            if (current->state == AUDIO_STATE_FADING_OUT && current->is_music) {
                music_fading = true;
                if (current->volume > 0) {
                    current->volume--;
                } else {
                    current->length = 0;
                    current->state = AUDIO_STATE_STOPPED;
                }
            }

            // Skip new music if old music is still fading
            bytes_to_mix = 0;
            if (!(music_fading && current->is_music && current->state == AUDIO_STATE_PLAYING)) {
                bytes_to_mix = min_u32((uint32_t)len, current->length);
            }

            if (bytes_to_mix > 0) {
                uint8_t final_volume = scaleVolume(current->volume, device->master_volume);
                SDL_MixAudioFormat(stream, current->buffer, AUDIO_FORMAT, bytes_to_mix, final_volume);

                current->buffer += bytes_to_mix;
                current->length -= bytes_to_mix;
            }

            previous = current;
            current = current->next;
        }
        // Handle looping audio
        else if (current->should_loop && current->state == AUDIO_STATE_PLAYING && current->length == 0) {
            current->buffer = current->buffer_original;
            current->length = current->length_original;
            current = current->next;
        }
        // Remove finished audio
        else {
            previous->next = current->next;

            if (!current->is_music) {
                device->active_sound_count--;
            }

            Audio* to_free = current;
            current = previous->next;

            to_free->next = NULL;
            freeAudio(to_free);
        }
    }
}