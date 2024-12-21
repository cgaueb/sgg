#pragma once
#include <functional>
#include <string>
#include <vector>

namespace graphics {
    class TextureManager;

    class Texture {
    private:
        unsigned int m_id;
        unsigned int boundSlot;
        std::string m_filename;
        bool useLodepng;
        unsigned int m_width = 0, m_height = 0;
        unsigned int m_channels = 0;
        std::vector<unsigned char> m_buffer;

        void makePowerOfTwo();

        bool load(const std::string &file);

        std::function<void()> m_customBuildFunction;

    public:
        Texture(const std::string &filename, bool useLodepng);

        ~Texture();

        bool buildGLTexture();

        void bindToSlot(unsigned int slot) { boundSlot = slot; }

        void setCustomBuildFunction(const std::function<void()> &buildFunction) {
            m_customBuildFunction = buildFunction;
        }

        std::string getFilename() const { return m_filename; };
        unsigned int getBoundSlot() { return boundSlot; }
        unsigned int getID() const { return m_id; }
        void setID(unsigned int texture_id) { m_id = texture_id; }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }

        unsigned int *getChannelsPointer() { return &m_channels;}
        unsigned int *getWidthPointer() { return &m_width; }
        unsigned int *getHeightPointer() { return &m_height; }
        unsigned int *getIDPointer() { return &m_id; }
        std::vector<unsigned char> *getBufferPointer() { return &m_buffer; }
    };
}
