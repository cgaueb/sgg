#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <mutex>
#include "core/graphics/textures/headers/Texture.h"

namespace graphics {

    // Cache entry stores raw texture data for later GL texture recreation
    struct TextureCache {
        std::string filename;
        std::vector<unsigned char> buffer;
        unsigned int width;
        unsigned int height;
        unsigned int channels;
        bool useLodepng;
        bool useNearestNeighbor;
        std::function<void(Texture&)> customBuildFunction;

        TextureCache(const Texture& texture, bool lodepng, bool nearestNeighbor)
            : filename(texture.getFilename())
            , buffer(texture.getBuffer())  // Use const getter
            , width(texture.getWidth())
            , height(texture.getHeight())
            , channels(texture.getChannels())
            , useLodepng(lodepng)
            , useNearestNeighbor(nearestNeighbor) {}
    };

    class TextureManager {
    public:
        static TextureManager& getInstance() {
            static TextureManager instance;
            return instance;
        }

        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;
        TextureManager(TextureManager&&) = delete;
        TextureManager& operator=(TextureManager&&) = delete;

        Texture* createTexture(std::string_view name, bool useLodepng = true,
                              const std::function<void(Texture&)>& customBuildFunction = nullptr);
        Texture* getTexture(std::string_view name) const;
        Texture* getTexture(unsigned int textureID) const;
        int getBoundSlotOfTexture(const Texture* texture) const;
        void bindTexture(Texture* texture, unsigned int slot);
        void unbindTexture(unsigned int slot);
        void unbindTexture(const Texture* texture);
        void unbindAllTextures();
        bool isTextureLoaded(std::string_view name) const;
        bool isSlotBound(unsigned int slot) const;
        size_t getTextureCount() const { return m_textures.size(); }
        unsigned int getMaxSlots() const { return MAX_TEXTURE_SLOTS; }
        size_t getCacheSize() const { return m_cache.size(); }
        std::vector<Texture*> getBoundTextures() const;
        std::unordered_map<std::string, Texture*> getTextures() const;
        void removeTexture(std::string_view name);
        void clearAllTextures();
        void setLogCallback(std::function<void(std::string_view)> callback) {
            m_logCallback = std::move(callback);
        }

    private:
        TextureManager() : m_boundTextures(MAX_TEXTURE_SLOTS, nullptr) {
            m_boundTextures.reserve(MAX_TEXTURE_SLOTS);
        }
        ~TextureManager();

        static const unsigned int MAX_TEXTURE_SLOTS = 32;
        static const size_t MAX_CACHE_SIZE = 100;

        std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
        std::unordered_map<unsigned int, Texture*> m_textureIDMap;
        std::vector<Texture*> m_boundTextures;
        std::vector<std::pair<std::string, TextureCache>> m_cache;  // Changed: store data, not GL textures
        mutable std::mutex m_mutex;
        std::function<void(std::string_view)> m_logCallback;
        bool m_shouldLog = false;

        void validateSlot(unsigned int slot) const;
        void validateTexture(const Texture* texture) const;
        void addToCache(const std::string& name, const Texture& texture, bool useLodepng,
                       bool useNearestNeighbor, const std::function<void(Texture&)>& customBuildFunc);
        std::unique_ptr<Texture> restoreFromCache(const std::string& name);
        void log(std::string_view message) const {
            if (m_shouldLog == false) return;
            if (m_logCallback) m_logCallback(message);
            else std::cerr << message << std::endl;
        }
    };

} // namespace graphics