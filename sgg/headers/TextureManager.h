#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H
#include <unordered_map>
#include <string>
#include <vector>
#include "Texture.h"

namespace graphics {

    class TextureManager {
    public:
        static TextureManager& getInstance() {
            if (!instance) instance = new TextureManager;
            return *instance;
        }

        static void destructInstance() {
            if (instance) {
                instance->unbindAllTextures();
                delete instance;
                instance = nullptr;
            }
        }

        Texture* createTexture(const std::string &name, bool useLodepng, const std::function<void(Texture &)> &customBuildFunction);
        Texture* getTexture(const std::string& file);
        Texture* getTexture(unsigned int textureID) const;
        int getBoundSlotOfTexture(Texture* texture) const;
        void bindTexture(Texture *texture, int slot);
        void unbindTexture(int slot);
        void unbindTexture(Texture* texture);
        void unbindAllTextures();
        std::vector<Texture*> getBoundTextures();
        std::unordered_map<std::string, Texture*> getTextures();



        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;

    private:
        TextureManager() = default;
        ~TextureManager();
        static TextureManager* instance;
        static constexpr unsigned int maxSlots = 32;
        std::unordered_map<std::string, Texture*> textures;
        std::vector<Texture*> boundTextures{maxSlots, nullptr};
    };
}

#endif
