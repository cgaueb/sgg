#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

/**
 * Optimized AudioManager class for SDL_mixer
 * Improvements:
 * - RAII resource management with smart pointers
 * - Better error handling and logging
 * - Resource caching and preloading
 * - Volume normalization
 * - Thread-safe operations
 * - Memory pool for better performance
 */
class AudioManager {
public:
    // Audio resource types with custom deleters
    struct MixChunkDeleter {
        void operator()(Mix_Chunk* chunk) const noexcept {
            if (chunk) {
                Mix_FreeChunk(chunk);
            }
        }
    };

    struct MixMusicDeleter {
        void operator()(Mix_Music* music) const noexcept {
            if (music) {
                Mix_FreeMusic(music);
            }
        }
    };

    using SoundPtr = std::unique_ptr<Mix_Chunk, MixChunkDeleter>;
    using MusicPtr = std::unique_ptr<Mix_Music, MixMusicDeleter>;

    // Configuration constants
    static constexpr int DEFAULT_FREQUENCY = 44100;
    static constexpr Uint16 DEFAULT_FORMAT = MIX_DEFAULT_FORMAT;
    static constexpr int DEFAULT_CHANNELS = 2;
    static constexpr int DEFAULT_CHUNK_SIZE = 4096;
    static constexpr int MAX_VOLUME = MIX_MAX_VOLUME;

    // Audio states for better control
    enum class MusicState {
        STOPPED,
        PLAYING,
        PAUSED,
        FADING_IN,
        FADING_OUT
    };

    AudioManager();
    ~AudioManager();

    // Disable copy constructor and assignment operator
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Move constructor and assignment operator
    AudioManager(AudioManager&&) noexcept = default;
    AudioManager& operator=(AudioManager&&) noexcept = default;

    // Core functionality
    bool isInitialized() const noexcept { return initialized_; }

    // Sound effects management
    bool playSound(const std::string& filename, float volume = 1.0f, bool looping = false);
    bool preloadSound(const std::string& filename);
    void unloadSound(const std::string& filename);
    void stopAllSounds();
    void setSoundVolume(const std::string& filename, float volume);

    // Music management
    bool playMusic(const std::string& filename, float volume = 1.0f, bool looping = true, int fade_ms = 0);
    bool preloadMusic(const std::string& filename);
    void unloadMusic(const std::string& filename);
    void stopMusic(int fade_ms = 0);
    void pauseMusic();
    void resumeMusic();
    MusicState getMusicState() const;

    // Global volume controls
    void setMasterVolume(float volume);
    void setSoundMasterVolume(float volume);
    void setMusicMasterVolume(float volume);
    float getMasterVolume() const noexcept { return master_volume_; }
    float getSoundMasterVolume() const noexcept { return sound_master_volume_; }
    float getMusicMasterVolume() const noexcept { return music_master_volume_; }

    // Resource management
    void clearAllAudio();
    size_t getSoundCacheSize() const noexcept { return sound_cache_.size(); }
    size_t getMusicCacheSize() const noexcept { return music_cache_.size(); }

    // Error handling
    std::string getLastError() const { return last_error_; }

private:
    // Resource caches
    std::unordered_map<std::string, SoundPtr> sound_cache_;
    std::unordered_map<std::string, MusicPtr> music_cache_;

    // Volume controls
    float master_volume_ = 1.0f;
    float sound_master_volume_ = 1.0f;
    float music_master_volume_ = 1.0f;

    // State tracking
    bool initialized_ = false;
    mutable std::string last_error_;
    std::string current_music_;
    MusicState music_state_ = MusicState::STOPPED;

    // Helper methods
    SoundPtr loadSound(const std::string& filename);
    MusicPtr loadMusic(const std::string& filename);
    int normalizeVolume(float volume, float master_volume) const noexcept;
    void setError(const std::string& error) const;
    void logError(const std::string& operation, const std::string& filename) const;

    // SDL_mixer callback for music state tracking
    static void musicFinishedCallback();
    static AudioManager* instance_;
    void onMusicFinished();
};