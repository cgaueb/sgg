#include "headers/TextureManager.h"
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <GL/glew.h>

namespace graphics {
    Texture *TextureManager::createTexture(const std::string &name, bool useLodepng,
                                           const std::function<void(Texture &)> &customBuildFunction = nullptr) {
        Texture *texture = getTexture(name);
        if (texture) {
            return texture;
        }
        texture = new Texture(name, useLodepng);

        if (customBuildFunction) {
            texture->setCustomBuildFunction([texture, customBuildFunction]() { customBuildFunction(*texture); });
        } else {
            texture->setCustomBuildFunction(nullptr);
        }
        texture->buildGLTexture() ? textures[name] = texture : nullptr;
        return texture;
    }

    Texture *TextureManager::getTexture(const std::string &name) {
        auto it = textures.find(name);
        if (it != textures.end()) {
            return it->second;
        }
        return nullptr;
    }

    Texture *TextureManager::getTexture(const unsigned int textureID) const {
        for (const auto &[fst, snd]: textures) {
            if (Texture *texture = snd; texture->getID() == textureID) {
                return texture;
            }
        }
        return nullptr;
    }

    int TextureManager::getBoundSlotOfTexture(Texture *texture) const {
        for (unsigned int i = 0; i < maxSlots; ++i) {
            if (boundTextures[i] == texture) {
                return i;
            }
        }
        return -1;
    }

    void TextureManager::bindTexture(Texture *texture, int slot) {
        if (!texture || texture->getID() == 0) {
            throw std::invalid_argument("Invalid texture: Cannot bind a null or uninitialized texture.");
        }
        if (slot >= maxSlots) {
            throw std::out_of_range("Texture slot exceeds maximum allowed slots.");
        }

        if (boundTextures[slot] != nullptr) {
            unbindTexture(slot);
        }

        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture->getID());
        boundTextures[slot] = texture;
    }

    void TextureManager::unbindTexture(int slot) {
        if (slot >= maxSlots) {
            std::cerr << "Texture slot exceeds maximum allowed slots." << std::endl;
            return;
        }
        if (boundTextures[slot]) {
            // Unbind the texture
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, 0);
            boundTextures[slot] = nullptr;
        }
    }

    void TextureManager::unbindTexture(Texture *texture) {
        if (!texture) {
            std::cerr << "Attempted to unbind a null texture." << std::endl;
            return;
        }

        for (unsigned int i = 0; i < maxSlots; ++i) {
            if (boundTextures[i] == texture) {
                unbindTexture(i);
                return;
            }
        }
        std::cerr << "Attempted to unbind a texture that was not bound." << std::endl;
    }

    void TextureManager::unbindAllTextures() {
        for (unsigned int i = 0; i < maxSlots; ++i) {
            if (boundTextures[i]) {
                unbindTexture(i);
            }
        }
    }

    std::unordered_map<std::string, Texture *> TextureManager::getTextures() {
        return textures;
    }

    std::vector<Texture *> TextureManager::getBoundTextures() {
        return boundTextures;
    }

    TextureManager::~TextureManager() {
        for (auto &[file, texture]: textures) {
            delete texture;
        }
        textures.clear();
    }
}
