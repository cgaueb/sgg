#include "core/audio/headers/AudioManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// Static member initialization
AudioManager* AudioManager::instance_ = nullptr;

AudioManager::AudioManager() {
    // Set up static instance for callbacks
    instance_ = this;

    // Initialize SDL_mixer
    if (Mix_OpenAudio(DEFAULT_FREQUENCY, DEFAULT_FORMAT, DEFAULT_CHANNELS, DEFAULT_CHUNK_SIZE) < 0) {
        setError("Failed to initialize SDL_mixer: " + std::string(Mix_GetError()));
        return;
    }

    // Set up music finished callback
    Mix_HookMusicFinished(musicFinishedCallback);

    // Allocate mixing channels (default 8, increase for more simultaneous sounds)
    Mix_AllocateChannels(16);

    initialized_ = true;

    std::cout << "AudioManager initialized successfully\n";
    std::cout << "Frequency: " << DEFAULT_FREQUENCY << " Hz\n";
    std::cout << "Channels: " << DEFAULT_CHANNELS << "\n";
    std::cout << "Chunk size: " << DEFAULT_CHUNK_SIZE << " bytes\n";
}

AudioManager::~AudioManager() {
    if (initialized_) {
        clearAllAudio();
        Mix_HookMusicFinished(nullptr);
        Mix_CloseAudio();

        std::cout << "AudioManager shut down\n";
    }

    if (instance_ == this) {
        instance_ = nullptr;
    }
}

bool AudioManager::playSound(const std::string& filename, float volume, bool looping) {
    if (!initialized_) {
        setError("AudioManager not initialized");
        return false;
    }

    // Check cache first
    auto it = sound_cache_.find(filename);
    SoundPtr* sound_ptr = nullptr;

    if (it != sound_cache_.end()) {
        sound_ptr = &it->second;
    } else {
        // Load and cache the sound
        auto new_sound = loadSound(filename);
        if (!new_sound) {
            return false;
        }

        auto [inserted_it, success] = sound_cache_.emplace(filename, std::move(new_sound));
        if (success) {
            sound_ptr = &inserted_it->second;
        }
    }

    if (!sound_ptr || !sound_ptr->get()) {
        setError("Failed to get sound: " + filename);
        return false;
    }

    // Set volume and play
    int final_volume = normalizeVolume(volume, sound_master_volume_);
    Mix_VolumeChunk(sound_ptr->get(), final_volume);

    int channel = Mix_PlayChannel(-1, sound_ptr->get(), looping ? -1 : 0);
    if (channel < 0) {
        logError("Mix_PlayChannel", filename);
        return false;
    }

    return true;
}

bool AudioManager::preloadSound(const std::string& filename) {
    if (!initialized_) {
        setError("AudioManager not initialized");
        return false;
    }

    // Check if already cached
    if (sound_cache_.find(filename) != sound_cache_.end()) {
        return true;  // Already loaded
    }

    auto sound = loadSound(filename);
    if (!sound) {
        return false;
    }

    sound_cache_[filename] = std::move(sound);
    return true;
}

void AudioManager::unloadSound(const std::string& filename) {
    sound_cache_.erase(filename);
}

void AudioManager::stopAllSounds() {
    if (initialized_) {
        Mix_HaltChannel(-1);
    }
}

void AudioManager::setSoundVolume(const std::string& filename, float volume) {
    auto it = sound_cache_.find(filename);
    if (it != sound_cache_.end() && it->second) {
        int final_volume = normalizeVolume(volume, sound_master_volume_);
        Mix_VolumeChunk(it->second.get(), final_volume);
    }
}

bool AudioManager::playMusic(const std::string& filename, float volume, bool looping, int fade_ms) {
    if (!initialized_) {
        setError("AudioManager not initialized");
        return false;
    }

    // Stop current music if different
    if (current_music_ != filename && music_state_ != MusicState::STOPPED) {
        stopMusic(fade_ms / 2);  // Quick fade out
    }

    // Check cache first
    auto it = music_cache_.find(filename);
    MusicPtr* music_ptr = nullptr;

    if (it != music_cache_.end()) {
        music_ptr = &it->second;
    } else {
        // Load and cache the music
        auto new_music = loadMusic(filename);
        if (!new_music) {
            return false;
        }

        auto [inserted_it, success] = music_cache_.emplace(filename, std::move(new_music));
        if (success) {
            music_ptr = &inserted_it->second;
        }
    }

    if (!music_ptr || !music_ptr->get()) {
        setError("Failed to get music: " + filename);
        return false;
    }

    // Set volume
    int final_volume = normalizeVolume(volume, music_master_volume_);
    Mix_VolumeMusic(final_volume);

    // Play with or without fade
    int result;
    if (fade_ms > 0) {
        result = Mix_FadeInMusic(music_ptr->get(), looping ? -1 : 1, fade_ms);
        music_state_ = MusicState::FADING_IN;
    } else {
        result = Mix_PlayMusic(music_ptr->get(), looping ? -1 : 1);
        music_state_ = MusicState::PLAYING;
    }

    if (result < 0) {
        logError("Mix_PlayMusic/Mix_FadeInMusic", filename);
        return false;
    }

    current_music_ = filename;
    return true;
}

bool AudioManager::preloadMusic(const std::string& filename) {
    if (!initialized_) {
        setError("AudioManager not initialized");
        return false;
    }

    // Check if already cached
    if (music_cache_.find(filename) != music_cache_.end()) {
        return true;  // Already loaded
    }

    auto music = loadMusic(filename);
    if (!music) {
        return false;
    }

    music_cache_[filename] = std::move(music);
    return true;
}

void AudioManager::unloadMusic(const std::string& filename) {
    // Stop music if it's currently playing
    if (current_music_ == filename) {
        stopMusic();
    }
    music_cache_.erase(filename);
}

void AudioManager::stopMusic(int fade_ms) {
    if (!initialized_ || music_state_ == MusicState::STOPPED) {
        return;
    }

    if (fade_ms > 0) {
        Mix_FadeOutMusic(fade_ms);
        music_state_ = MusicState::FADING_OUT;
    } else {
        Mix_HaltMusic();
        music_state_ = MusicState::STOPPED;
        current_music_.clear();
    }
}

void AudioManager::pauseMusic() {
    if (initialized_ && music_state_ == MusicState::PLAYING) {
        Mix_PauseMusic();
        music_state_ = MusicState::PAUSED;
    }
}

void AudioManager::resumeMusic() {
    if (initialized_ && music_state_ == MusicState::PAUSED) {
        Mix_ResumeMusic();
        music_state_ = MusicState::PLAYING;
    }
}

AudioManager::MusicState AudioManager::getMusicState() const {
    if (!initialized_) {
        return MusicState::STOPPED;
    }

    // Update state based on SDL_mixer state
    if (Mix_PlayingMusic()) {
        if (Mix_PausedMusic()) {
            return MusicState::PAUSED;
        }
        return music_state_;  // Could be PLAYING, FADING_IN, or FADING_OUT
    }

    return MusicState::STOPPED;
}

void AudioManager::setMasterVolume(float volume) {
    master_volume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update all current volumes
    setSoundMasterVolume(sound_master_volume_);
    setMusicMasterVolume(music_master_volume_);
}

void AudioManager::setSoundMasterVolume(float volume) {
    sound_master_volume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update all cached sound volumes
    for (const auto& [filename, sound] : sound_cache_) {
        if (sound) {
            int current_volume = Mix_VolumeChunk(sound.get(), -1);  // Get current volume
            float normalized = static_cast<float>(current_volume) / MAX_VOLUME;
            int new_volume = normalizeVolume(normalized, sound_master_volume_);
            Mix_VolumeChunk(sound.get(), new_volume);
        }
    }
}

void AudioManager::setMusicMasterVolume(float volume) {
    music_master_volume_ = std::clamp(volume, 0.0f, 1.0f);

    if (initialized_) {
        int current_volume = Mix_VolumeMusic(-1);  // Get current volume
        float normalized = static_cast<float>(current_volume) / MAX_VOLUME;
        int new_volume = normalizeVolume(normalized, music_master_volume_);
        Mix_VolumeMusic(new_volume);
    }
}

void AudioManager::clearAllAudio() {
    if (initialized_) {
        stopAllSounds();
        stopMusic();
    }

    sound_cache_.clear();
    music_cache_.clear();
    current_music_.clear();
    music_state_ = MusicState::STOPPED;

    std::cout << "All audio resources cleared\n";
}

AudioManager::SoundPtr AudioManager::loadSound(const std::string& filename) {
    Mix_Chunk* chunk = Mix_LoadWAV(filename.c_str());
    if (!chunk) {
        logError("Mix_LoadWAV", filename);
        return SoundPtr{}; // empty unique_ptr
    }

    std::cout << "Loaded sound: " << filename << "\n";
    return SoundPtr(chunk);
}

AudioManager::MusicPtr AudioManager::loadMusic(const std::string& filename) {
    Mix_Music* music = Mix_LoadMUS(filename.c_str());
    if (!music) {
        logError("Mix_LoadMUS", filename);
        return MusicPtr{}; // empty unique_ptr
    }

    std::cout << "Loaded music: " << filename << "\n";
    return MusicPtr(music);
}

int AudioManager::normalizeVolume(float volume, float master_volume) const noexcept {
    float combined = std::clamp(volume * master_volume * master_volume_, 0.0f, 1.0f);
    return static_cast<int>(std::round(combined * MAX_VOLUME));
}

void AudioManager::setError(const std::string& error) const {
    last_error_ = error;
    std::cerr << "AudioManager Error: " << error << "\n";
}

void AudioManager::logError(const std::string& operation, const std::string& filename) const {
    std::string error = operation + " failed for '" + filename + "': " + Mix_GetError();
    setError(error);
}

void AudioManager::musicFinishedCallback() {
    if (instance_) {
        instance_->onMusicFinished();
    }
}

void AudioManager::onMusicFinished() {
    music_state_ = MusicState::STOPPED;
    current_music_.clear();
}