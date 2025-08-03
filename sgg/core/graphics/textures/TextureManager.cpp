#include "core/graphics/textures/headers/TextureManager.h"
#include <stdexcept>
#include <algorithm>
#include <GL/glew.h>
#include <array>

namespace graphics {
    Texture *TextureManager::createTexture(std::string_view name, bool useLodepng,
                                           const std::function<void(Texture &)> &customBuildFunction) {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string nameStr(name);

        // Check if texture already exists
        auto it = m_textures.find(nameStr);
        if (it != m_textures.end()) {
            log("Texture already exists: " + nameStr);
            Texture *tex = it->second.get();
            if (!tex || tex->getID() == 0) {
                log("Invalid existing texture: " + nameStr);
                return nullptr;
            }
            return tex;
        }

        // Check cache for raw texture data
        auto cacheIt = std::find_if(m_cache.begin(), m_cache.end(),
                                    [&nameStr](const auto &pair) { return pair.first == nameStr; });

        if (cacheIt != m_cache.end()) {
            log("Cache hit for texture: " + nameStr);
            auto texture = restoreFromCache(nameStr);
            if (!texture) {
                log("Failed to restore texture from cache: " + nameStr);
                m_cache.erase(cacheIt); // Remove corrupted cache entry
                // Fall through to create new texture
            } else {
                Texture *rawPtr = texture.get();
                m_textures.insert(std::make_pair(nameStr, std::move(texture)));
                m_textureIDMap.insert(std::make_pair(rawPtr->getID(), rawPtr));
                log("Restored texture from cache: " + nameStr + " with ID " + std::to_string(rawPtr->getID()));
                return rawPtr;
            }
        }

        // Create new texture
        auto texture = std::make_unique<Texture>(nameStr, useLodepng);
        if (!texture) {
            log("Failed to allocate texture for: " + nameStr);
            return nullptr;
        }

        if (customBuildFunction) {
            texture->setCustomBuildFunction([texturePtr = texture.get(), customBuildFunction]() {
                customBuildFunction(*texturePtr);
            });
        }

        if (!texture->buildGLTexture()) {
            log("Failed to build GL texture for: " + nameStr);
            return nullptr;
        }

        Texture *rawPtr = texture.get();
        if (rawPtr->getID() == 0) {
            log("Invalid texture ID after creation for: " + nameStr);
            return nullptr;
        }

        // Add to cache before moving texture into main storage
        addToCache(nameStr, *rawPtr, useLodepng, false, customBuildFunction);

        m_textures.insert(std::make_pair(nameStr, std::move(texture)));
        m_textureIDMap.insert(std::make_pair(rawPtr->getID(), rawPtr));

        log("Created texture: " + nameStr + " with ID " + std::to_string(rawPtr->getID()));
        return rawPtr;
    }

    Texture *TextureManager::getTexture(std::string_view name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_textures.find(std::string(name));
        if (it != m_textures.end()) {
            log("Retrieved texture: " + std::string(name));
            return it->second.get();
        }
        log("Texture not found: " + std::string(name));
        return nullptr;
    }

    Texture *TextureManager::getTexture(unsigned int textureID) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_textureIDMap.find(textureID);
        if (it != m_textureIDMap.end()) {
            log("Retrieved texture ID: " + std::to_string(textureID));
            return it->second;
        }
        log("Texture ID not found: " + std::to_string(textureID));
        return nullptr;
    }

    int TextureManager::getBoundSlotOfTexture(const Texture *texture) const {
        if (!texture) {
            log("Null texture in getBoundSlotOfTexture");
            return -1;
        }

        auto it = std::find(m_boundTextures.begin(), m_boundTextures.end(), texture);
        if (it != m_boundTextures.end()) {
            int slot = static_cast<int>(std::distance(m_boundTextures.begin(), it));
            log("Found texture ID " + std::to_string(texture->getID()) + " in slot " + std::to_string(slot));
            return slot;
        }
        log("Texture ID " + std::to_string(texture->getID()) + " not bound");
        return -1;
    }

    void TextureManager::bindTexture(Texture *texture, unsigned int slot) {
        std::lock_guard<std::mutex> lock(m_mutex);
        validateTexture(texture);
        validateSlot(slot);

        GLint currentTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
        if (m_boundTextures[slot] == texture && static_cast<unsigned int>(currentTexture) == texture->getID()) {
            log("Texture ID " + std::to_string(texture->getID()) + " already bound to slot " + std::to_string(slot));
            return;
        }

        if (m_boundTextures[slot]) {
            unbindTexture(slot);
        }

        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture->getID());
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            log("OpenGL error " + std::to_string(err) + " while binding texture ID " +
                std::to_string(texture->getID()) + " to slot " + std::to_string(slot));
            return;
        }
        m_boundTextures[slot] = texture;
        log("Bound texture ID " + std::to_string(texture->getID()) + " (" + texture->getFilename() + ") to slot " +
            std::to_string(slot));
    }

    void TextureManager::unbindTexture(unsigned int slot) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (slot >= MAX_TEXTURE_SLOTS) {
            log("Texture slot " + std::to_string(slot) + " exceeds maximum allowed slots (" +
                std::to_string(MAX_TEXTURE_SLOTS) + ")");
            return;
        }

        if (m_boundTextures[slot]) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, 0);
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                log("OpenGL error " + std::to_string(err) + " while unbinding slot " + std::to_string(slot));
            }
            log("Unbound texture from slot " + std::to_string(slot));
            m_boundTextures[slot] = nullptr;
        } else {
            log("Slot " + std::to_string(slot) + " already unbound");
        }
    }

    void TextureManager::unbindTexture(const Texture *texture) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!texture) {
            log("Attempted to unbind a null texture");
            return;
        }

        bool found = false;
        for (unsigned int i = 0; i < MAX_TEXTURE_SLOTS; ++i) {
            if (m_boundTextures[i] == texture) {
                unbindTexture(i);
                found = true;
            }
        }

        if (!found) {
            log("Attempted to unbind texture ID " + std::to_string(texture->getID()) + " that was not bound");
        }
    }

    void TextureManager::unbindAllTextures() {
        // First collect slots that are currently bound under mutex protection
        std::array<unsigned int, MAX_TEXTURE_SLOTS> slotsToUnbind{};
        unsigned int count = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (unsigned int i = 0; i < MAX_TEXTURE_SLOTS; ++i) {
                if (m_boundTextures[i]) {
                    slotsToUnbind[count++] = i;
                }
            }
        }

        // Now unbind outside the mutex to avoid dead-locking with unbindTexture's own locking.
        for (unsigned int idx = 0; idx < count; ++idx) {
            unbindTexture(slotsToUnbind[idx]);
        }

        log("Unbound all textures");
    }

    bool TextureManager::isTextureLoaded(std::string_view name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool loaded = m_textures.find(std::string(name)) != m_textures.end();
        log("Checked texture " + std::string(name) + ": " + (loaded ? "loaded" : "not loaded"));
        return loaded;
    }

    bool TextureManager::isSlotBound(unsigned int slot) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool bound = slot < MAX_TEXTURE_SLOTS && m_boundTextures[slot] != nullptr;
        log("Checked slot " + std::to_string(slot) + ": " + (bound ? "bound" : "not bound"));
        return bound;
    }

    std::vector<Texture *> TextureManager::getBoundTextures() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        log("Retrieved bound textures list");
        return m_boundTextures;
    }

    std::unordered_map<std::string, Texture *> TextureManager::getTextures() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::unordered_map<std::string, Texture *> result;
        result.reserve(m_textures.size());
        for (const auto &pair: m_textures) {
            result.insert(std::make_pair(pair.first, pair.second.get()));
        }
        log("Retrieved " + std::to_string(result.size()) + " textures");
        return result;
    }

    void TextureManager::removeTexture(std::string_view name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_textures.find(std::string(name));
        if (it == m_textures.end()) {
            log("Texture not found for removal: " + std::string(name));
            return;
        }

        Texture *texture = it->second.get();

        // Add to cache before removing (if not already cached)
        auto cacheIt = std::find_if(m_cache.begin(), m_cache.end(),
                                    [&name](const auto &pair) { return pair.first == std::string(name); });

        if (cacheIt == m_cache.end()) {
            addToCache(std::string(name), *texture, true, false, nullptr);
        }

        // Remove from maps and unbind
        m_textureIDMap.erase(texture->getID());
        unbindTexture(texture);
        m_textures.erase(it);

        log("Removed texture: " + std::string(name));
    }

    void TextureManager::clearAllTextures() {
        // First unbind all currently bound textures. This call acquires the mutex internally,
        // so do NOT hold the lock here, otherwise we would attempt to lock the same mutex twice
        // from the same thread which causes a dead-lock.
        unbindAllTextures();

        // Now safely clear all containers under the mutex.
        std::lock_guard<std::mutex> lock(m_mutex);
        m_textureIDMap.clear();
        m_textures.clear();
        m_cache.clear();
        log("Cleared all textures");
    }

    void TextureManager::addToCache(const std::string &name, const Texture &texture, bool useLodepng,
                                    bool useNearestNeighbor, const std::function<void(Texture &)> &customBuildFunc) {
        // Don't cache textures with custom build functions (they might have external dependencies)
        if (customBuildFunc) {
            log("Skipping cache for texture with custom build function: " + name);
            return;
        }

        // Don't cache if texture data is invalid
        if (!texture.isValid()) {
            log("Skipping cache for invalid texture: " + name);
            return;
        }

        // Check if already in cache
        auto cacheIt = std::find_if(m_cache.begin(), m_cache.end(),
                                    [&name](const auto &pair) { return pair.first == name; });

        if (cacheIt != m_cache.end()) {
            log("Texture already in cache: " + name);
            return;
        }

        // Remove oldest cache entry if cache is full
        if (m_cache.size() >= MAX_CACHE_SIZE) {
            log("Cache full, removing oldest texture: " + m_cache.front().first);
            m_cache.erase(m_cache.begin());
        }

        // Add to cache
        TextureCache cacheEntry(texture, useLodepng, useNearestNeighbor);
        cacheEntry.customBuildFunction = customBuildFunc; // Store custom function if provided
        m_cache.emplace_back(name, std::move(cacheEntry));

        log("Added to cache: " + name + " (data only, no GL texture)");
    }

    std::unique_ptr<Texture> TextureManager::restoreFromCache(const std::string &name) {
        auto cacheIt = std::find_if(m_cache.begin(), m_cache.end(),
                                    [&name](const auto &pair) { return pair.first == name; });

        if (cacheIt == m_cache.end()) {
            return nullptr;
        }

        const TextureCache &cacheEntry = cacheIt->second;

        // Create a new texture and populate it with cached data
        auto texture = std::make_unique<Texture>(cacheEntry.filename, cacheEntry.useLodepng,
                                                 cacheEntry.useNearestNeighbor);

        // Directly set the cached data using the pointer accessors
        *texture->getBufferPointer() = cacheEntry.buffer;
        *texture->getWidthPointer() = cacheEntry.width;
        *texture->getHeightPointer() = cacheEntry.height;
        *texture->getChannelsPointer() = cacheEntry.channels;

        if (cacheEntry.customBuildFunction) {
            texture->setCustomBuildFunction([texturePtr = texture.get(), func = cacheEntry.customBuildFunction]() {
                func(*texturePtr);
            });
        }

        // Validate the restored data
        if (!texture->isValid()) {
            log("Invalid data after cache restoration for: " + name);
            return nullptr;
        }

        // Build the GL texture
        if (!texture->buildGLTexture()) {
            log("Failed to rebuild GL texture from cache for: " + name);
            return nullptr;
        }

        // Remove from cache since we're now using it
        m_cache.erase(cacheIt);

        return texture;
    }

    void TextureManager::validateSlot(unsigned int slot) const {
        if (slot >= MAX_TEXTURE_SLOTS) {
            throw std::out_of_range("Texture slot " + std::to_string(slot) +
                                    " exceeds maximum allowed slots (" +
                                    std::to_string(MAX_TEXTURE_SLOTS) + ")");
        }
    }

    void TextureManager::validateTexture(const Texture *texture) const {
        if (!texture) {
            throw std::invalid_argument("Cannot bind a null texture");
        }
        if (texture->getID() == 0) {
            throw std::invalid_argument("Cannot bind an uninitialized texture (ID = 0)");
        }
    }

    TextureManager::~TextureManager() {
        if (glGetError() != GL_INVALID_OPERATION) {
            clearAllTextures();
        }
    }
} // namespace graphics