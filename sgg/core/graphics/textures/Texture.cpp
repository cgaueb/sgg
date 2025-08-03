#include "core/graphics/textures/headers/Texture.h"
#include "core/utils/headers/Lodepng.h"
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <cmath>
#include <algorithm>

namespace graphics {
    void Texture::makePowerOfTwo() {
        if (isPowerOfTwo(m_width) && isPowerOfTwo(m_height)) {
            std::cerr << "Texture " << m_filename << " is already power of two (" << m_width << "x" << m_height << ")"
                    << std::endl;
            return;
        }

        const unsigned int newWidth = nextPowerOfTwo(m_width);
        const unsigned int newHeight = nextPowerOfTwo(m_height);
        const size_t newSize = static_cast<size_t>(newWidth) * newHeight * 4;

        if (newWidth == 0 || newHeight == 0 || newSize == 0) {
            std::cerr << "Invalid dimensions for texture " << m_filename << ": " << newWidth << "x" << newHeight <<
                    std::endl;
            return;
        }

        if (m_buffer.size() < static_cast<size_t>(m_width) * m_height * 4) {
            std::cerr << "Invalid buffer size for texture " << m_filename << ": expected "
                    << (m_width * m_height * 4) << ", got " << m_buffer.size() << std::endl;
            return;
        }

        std::vector<unsigned char> resizedBuffer(newSize);

        if (m_useNearestNeighbor) {
            const double xScale = static_cast<double>(m_width) / newWidth;
            const double yScale = static_cast<double>(m_height) / newHeight;

            for (unsigned int y = 0; y < newHeight; ++y) {
                const unsigned int srcY = static_cast<unsigned int>(y * yScale);
                if (srcY >= m_height) {
                    std::cerr << "Out-of-bounds srcY " << srcY << " in makePowerOfTwo for " << m_filename << std::endl;
                    return;
                }
                const size_t srcRowBase = static_cast<size_t>(srcY) * m_width * 4;
                const size_t destRowBase = static_cast<size_t>(y) * newWidth * 4;

                for (unsigned int x = 0; x < newWidth; ++x) {
                    const unsigned int srcX = static_cast<unsigned int>(x * xScale);
                    if (srcX >= m_width) {
                        std::cerr << "Out-of-bounds srcX " << srcX << " in makePowerOfTwo for " << m_filename <<
                                std::endl;
                        return;
                    }
                    const size_t srcIdx = srcRowBase + srcX * 4;
                    const size_t destIdx = destRowBase + x * 4;

                    if (srcIdx + 3 >= m_buffer.size() || destIdx + 3 >= newSize) {
                        std::cerr << "Buffer overflow in makePowerOfTwo for " << m_filename
                                << ": srcIdx=" << srcIdx << ", destIdx=" << destIdx << std::endl;
                        return;
                    }

                    resizedBuffer[destIdx] = m_buffer[srcIdx];
                    resizedBuffer[destIdx + 1] = m_buffer[srcIdx + 1];
                    resizedBuffer[destIdx + 2] = m_buffer[srcIdx + 2];
                    resizedBuffer[destIdx + 3] = m_buffer[srcIdx + 3];
                }
            }
        } else {
            const double xScale = m_width > 1 ? static_cast<double>(m_width - 1) / newWidth : 1.0;
            const double yScale = m_height > 1 ? static_cast<double>(m_height - 1) / newHeight : 1.0;

            for (unsigned int y = 0; y < newHeight; ++y) {
                const double srcY = y * yScale;
                const unsigned int y0 = static_cast<unsigned int>(srcY);
                const unsigned int y1 = std::min(y0 + 1, m_height - 1);
                if (y0 >= m_height) {
                    std::cerr << "Out-of-bounds y0 " << y0 << " in makePowerOfTwo for " << m_filename << std::endl;
                    return;
                }
                const float yWeight = static_cast<float>(srcY - y0);
                const size_t row0Base = static_cast<size_t>(y0) * m_width * 4;
                const size_t row1Base = static_cast<size_t>(y1) * m_width * 4;
                const size_t destRowBase = static_cast<size_t>(y) * newWidth * 4;

                for (unsigned int x = 0; x < newWidth; ++x) {
                    const double srcX = x * xScale;
                    const unsigned int x0 = static_cast<unsigned int>(srcX);
                    const unsigned int x1 = std::min(x0 + 1, m_width - 1);
                    if (x0 >= m_width) {
                        std::cerr << "Out-of-bounds x0 " << x0 << " in makePowerOfTwo for " << m_filename << std::endl;
                        return;
                    }
                    const size_t idx00 = row0Base + x0 * 4;
                    const size_t idx01 = row0Base + x1 * 4;
                    const size_t idx10 = row1Base + x0 * 4;
                    const size_t idx11 = row1Base + x1 * 4;
                    const size_t destIdx = destRowBase + x * 4;

                    if (idx00 + 3 >= m_buffer.size() || idx01 + 3 >= m_buffer.size() ||
                        idx10 + 3 >= m_buffer.size() || idx11 + 3 >= m_buffer.size() ||
                        destIdx + 3 >= newSize) {
                        std::cerr << "Buffer overflow in makePowerOfTwo for " << m_filename
                                << ": idx00=" << idx00 << ", idx01=" << idx01
                                << ", idx10=" << idx10 << ", idx11=" << idx11
                                << ", destIdx=" << destIdx << std::endl;
                        return;
                    }

                    for (int c = 0; c < 4; ++c) {
                        const float val00 = static_cast<float>(m_buffer[idx00 + c]);
                        const float val01 = static_cast<float>(m_buffer[idx01 + c]);
                        const float val10 = static_cast<float>(m_buffer[idx10 + c]);
                        const float val11 = static_cast<float>(m_buffer[idx11 + c]);
                        const float xWeight = static_cast<float>(srcX - x0);
                        const float top = val00 + xWeight * (val01 - val00);
                        const float bottom = val10 + xWeight * (val11 - val10);
                        resizedBuffer[destIdx + c] = static_cast<unsigned char>(top + yWeight * (bottom - top) + 0.5f);
                    }
                }
            }
        }

        m_buffer = std::move(resizedBuffer);
        m_width = newWidth;
        m_height = newHeight;
        std::cerr << "Resized texture " << m_filename << " to " << newWidth << "x" << newHeight << std::endl;
    }

    bool Texture::load(std::string_view file) {
        m_buffer.clear(); // Ensure buffer is empty before loading
        const unsigned int error = lodepng::decode(m_buffer, m_width, m_height, std::string(file));
        if (error) {
            std::cerr << "LodePNG error " << error << ": " << lodepng_error_text(error) << " for " << file << std::endl;
            return false;
        }

        if (m_buffer.empty() || m_width == 0 || m_height == 0) {
            std::cerr << "Invalid texture data after loading " << file << std::endl;
            return false;
        }

        m_channels = 4;
        std::cerr << "Loaded texture " << file << ": " << m_width << "x" << m_height << std::endl;
        return true;
    }

    bool Texture::buildGLTexture() {
        if (m_customBuildFunction) {
            m_customBuildFunction();
            std::cerr << "Built custom texture for " << m_filename << std::endl;
            return true;
        }

        if (!isValid()) {
            std::cerr << "Cannot build GL texture: invalid data for " << m_filename << std::endl;
            return false;
        }

        // Check if texture is already built
        if (m_id != 0) {
            std::cerr << "GL texture already exists for " << m_filename << " with ID " << m_id << std::endl;
            return true; // Already built, return success
        }

        GLint maxTextureUnits;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
        if (maxTextureUnits < 1) {
            std::cerr << "Invalid OpenGL texture units: " << maxTextureUnits << " for " << m_filename << std::endl;
            return false;
        }

        // Generate new texture only if one doesn't exist
        glGenTextures(1, &m_id);
        if (m_id == 0) {
            std::cerr << "Failed to generate GL texture for " << m_filename << std::endl;
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, m_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer.data());

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error " << err << " during texture upload for " << m_filename << std::endl;
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &m_id);
            m_id = 0;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cerr << "Built GL texture ID " << m_id << " for " << m_filename << std::endl;
        return true;
    }

    bool Texture::rebuildGLTexture() {
        if (m_id != 0) {
            std::cerr << "Deleting existing GL texture ID " << m_id << " for rebuild of " << m_filename << std::endl;
            glDeleteTextures(1, &m_id);
            m_id = 0;
        }
        // Now build a new one
        return buildGLTexture();
    }

    Texture::Texture(std::string filename, bool useLodepng, bool useNearestNeighbor)
        : m_id(0), m_filename(std::move(filename)), m_useLodepng(useLodepng), m_useNearestNeighbor(useNearestNeighbor),
          m_width(0), m_height(0), m_channels(0) {
        if (m_filename.size() >= 4 && m_filename.compare(m_filename.size() - 4, 4, ".ttf") == 0) {
            std::cerr << "Skipping texture load for .ttf file: " << m_filename << std::endl;
            return;
        }

        if (!useLodepng) {
            std::cerr << "Non-LodePNG loading not supported for " << m_filename << std::endl;
            return;
        }

        if (!load(m_filename)) {
            std::cerr << "Error loading texture: " << m_filename << std::endl;
            return;
        }

        makePowerOfTwo();
    }

    Texture::~Texture() {
        if (m_id != 0) {
            glDeleteTextures(1, &m_id);
            std::cerr << "Deleted GL texture ID " << m_id << " for " << m_filename << std::endl;
            m_id = 0; // Prevent double deletion
        }
    }
} // namespace graphics
