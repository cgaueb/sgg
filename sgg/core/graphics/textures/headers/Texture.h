#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace graphics {
    class TextureManager;

    class Texture {
    public:
        explicit Texture(std::string filename, bool useLodepng = true, bool useNearestNeighbor = false);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = delete;
        Texture& operator=(Texture&&) = delete;

        bool buildGLTexture();

        bool rebuildGLTexture();

        void setCustomBuildFunction(std::function<void()> buildFunction) {
            m_customBuildFunction = std::move(buildFunction);
        }

        [[nodiscard]] const std::string& getFilename() const { return m_filename; }
        [[nodiscard]] unsigned int getID() const { return m_id; }
        [[nodiscard]] unsigned int getWidth() const { return m_width; }
        [[nodiscard]] unsigned int getHeight() const { return m_height; }
        [[nodiscard]] unsigned int getChannels() const { return m_channels; }

        void setID(unsigned int texture_id) { m_id = texture_id; }

        unsigned int* getChannelsPointer() { return &m_channels; }
        unsigned int* getWidthPointer() { return &m_width; }
        unsigned int* getHeightPointer() { return &m_height; }
        unsigned int* getIDPointer() { return &m_id; }
        std::vector<unsigned char>* getBufferPointer() { return &m_buffer; }
        [[nodiscard]] const std::vector<unsigned char>& getBuffer() const { return m_buffer; }

        [[nodiscard]] bool isValid() const {
            return !m_buffer.empty() && m_width > 0 && m_height > 0;
        }

    private:
        unsigned int m_id;
        std::string m_filename;
        bool m_useLodepng;
        bool m_useNearestNeighbor;
        unsigned int m_width;
        unsigned int m_height;
        unsigned int m_channels;
        std::vector<unsigned char> m_buffer;
        std::function<void()> m_customBuildFunction;

        void makePowerOfTwo();
        bool load(std::string_view file);

        static bool isPowerOfTwo(unsigned int n) {
            return n > 0 && (n & (n - 1)) == 0;
        }

        static unsigned int nextPowerOfTwo(unsigned int n) {
            if (n <= 1) return 1;
            --n;
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            return ++n;
        }
    };
}