#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/glew.h"
#include "core/graphics/shaders/headers/Shader.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <memory>
#include "core/utils/headers/CompilerTraits.h"

// Forward declarations for better compile times
class TextureManager;

// Compact glyph information structure - optimized memory layout
struct GlyphInfo {
    // Group floating point values together for better cache alignment
    float width;         // Width of the glyph
    float height;        // Height of the glyph
    float bearing_x;     // Horizontal bearing
    float bearing_y;     // Vertical bearing
    float advance;       // Advance width

    // UV coordinates packed together
    glm::vec2 uv_min;    // UV coordinates (top-left)
    glm::vec2 uv_max;    // UV coordinates (bottom-right)

    // Constructor for initialization
    GlyphInfo() noexcept = default;
    GlyphInfo(float w, float h, float bx, float by, float adv,
              const glm::vec2& uv_min, const glm::vec2& uv_max) noexcept
        : width(w), height(h), bearing_x(bx), bearing_y(by), advance(adv),
          uv_min(uv_min), uv_max(uv_max) {}
};

// Font atlas structure - improved memory layout
struct FontAtlas {
    GLuint texture_id = 0;     // OpenGL texture ID
    int width = 0;             // Atlas texture width
    int height = 0;            // Atlas texture height
    bool initialized = false;  // Whether atlas has been generated

    // Constructor
    FontAtlas() noexcept = default;
    FontAtlas(int w, int h) noexcept : width(w), height(h) {}

    // Move semantics for efficient transfers
    FontAtlas(FontAtlas&& other) noexcept
        : texture_id(other.texture_id), width(other.width),
          height(other.height), initialized(other.initialized) {
        other.texture_id = 0;
        other.initialized = false;
    }

    FontAtlas& operator=(FontAtlas&& other) noexcept {
        if (this != &other) {
            // Clean up current texture if it exists
            if (texture_id != 0) {
                glDeleteTextures(1, &texture_id);
            }

            texture_id = other.texture_id;
            width = other.width;
            height = other.height;
            initialized = other.initialized;
            other.texture_id = 0;
            other.initialized = false;
        }
        return *this;
    }

    // Proper destructor for RAII
    ~FontAtlas() noexcept {
        if (texture_id != 0) {
            glDeleteTextures(1, &texture_id);
            texture_id = 0;
        }
    }

    // Disable copy to prevent texture ID duplication
    FontAtlas(const FontAtlas&) = delete;
    FontAtlas& operator=(const FontAtlas&) = delete;
};

// Optimized Font structure with better memory management
struct Font {
    std::string fontname;
    FT_Face face = nullptr;
    GLuint font_tex = 0;       // Legacy texture field (kept for compatibility)
    FontAtlas atlas;           // The font atlas containing all rasterized glyphs

    // Use flat_map or keep unordered_map - unordered_map is better for lookups
    std::unordered_map<unsigned char, GlyphInfo> glyphs;  // ASCII characters only for better performance

    // Constructor
    Font() noexcept = default;
    explicit Font(const std::string& name) noexcept : fontname(name) {}

    // Move semantics
    Font(Font&& other) noexcept
        : fontname(std::move(other.fontname)), face(other.face),
          font_tex(other.font_tex), atlas(std::move(other.atlas)),
          glyphs(std::move(other.glyphs)) {
        other.face = nullptr;
        other.font_tex = 0;
    }

    Font& operator=(Font&& other) noexcept {
        if (this != &other) {
            // Clean up current FreeType face
            if (face != nullptr) {
                FT_Done_Face(face);
            }

            fontname = std::move(other.fontname);
            face = other.face;
            font_tex = other.font_tex;
            atlas = std::move(other.atlas);
            glyphs = std::move(other.glyphs);
            other.face = nullptr;
            other.font_tex = 0;
        }
        return *this;
    }

    // Proper destructor for RAII
    ~Font() noexcept {
        if (face != nullptr) {
            FT_Done_Face(face);
            face = nullptr;
        }
    }

    // Disable copy to prevent FreeType face duplication
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
};

// Optimized TextRecord structure
struct TextRecord {
    glm::vec2 pos;
    glm::vec2 size;
    std::string text;           // Consider using string_view for read-only scenarios
    glm::vec4 color1;
    glm::vec4 color2;
    glm::vec2 gradient;
    glm::mat4 mv;
    glm::mat4 proj;
    Font* font = nullptr;
    bool use_gradient = false;  // Move bool to end for better packing

    // Constructor for common case
    TextRecord(const glm::vec2& position, const glm::vec2& sz,
               const std::string& txt, const glm::vec4& col) noexcept
        : pos(position), size(sz), text(txt), color1(col), color2(col),
          gradient(0.0f, 1.0f), mv(1.0f), proj(1.0f) {}

    TextRecord() noexcept = default;
};

// Optimized vertex structure with better alignment
struct TextVertex {
    glm::vec2 position;   // Position of vertex
    glm::vec2 texcoord;   // Texture coordinate

    TextVertex() noexcept = default;
    TextVertex(const glm::vec2& pos, const glm::vec2& tex) noexcept
        : position(pos), texcoord(tex) {}
};

// Main FontLib class with performance optimizations
class FontLib {
private:
    // Static FreeType library instance
    static FT_Library m_ft;

    // Font management
    std::unordered_map<std::string, Font>::iterator m_curr_font;
    std::unordered_map<std::string, Font> m_fonts;

    // Rendering resources
    Shader m_font_shader;
    GLuint m_font_vbo = 0;
    GLuint m_font_vao = 0;
    GLuint m_text_vbo = 0;          // VBO for text rendering
    GLuint m_text_vao = 0;          // VAO for text rendering

    // Configuration
    GLuint m_font_res = 64;         // Font resolution
    int m_atlas_size = 1024;        // Default atlas size in pixels
    glm::vec2 m_canvas{800, 600};   // Default canvas size

    // Text batching
    std::vector<TextRecord> m_content;

    // Performance optimizations
    mutable std::vector<TextVertex> m_vertex_buffer;  // Reusable vertex buffer
    mutable std::vector<GLfloat> m_gl_buffer;         // Reusable GL data buffer

    // Internal methods
    void drawText(const TextRecord& entry);
    bool generateAtlas(Font& font);
    void buildTextMesh(std::vector<TextVertex>& vertices,
                      const std::string& text,
                      const Font& font,
                      const glm::vec2& position,
                      const glm::vec2& size) const;

    // Helper methods
    ATTR_GNU_HOT inline bool isValidFont() const noexcept {
        return m_curr_font != m_fonts.end();
    }

    void setupOpenGLState() noexcept;
    void cleanupOpenGLState() noexcept;

public:
    FontLib() noexcept = default;

    // Proper destructor for RAII cleanup
    ~FontLib() noexcept {
        // Clean up OpenGL resources
        if (m_font_vao != 0) {
            glDeleteVertexArrays(1, &m_font_vao);
        }
        if (m_text_vao != 0) {
            glDeleteVertexArrays(1, &m_text_vao);
        }
        if (m_font_vbo != 0) {
            glDeleteBuffers(1, &m_font_vbo);
        }
        if (m_text_vbo != 0) {
            glDeleteBuffers(1, &m_text_vbo);
        }

        // Clean up FreeType library
        if (m_ft != nullptr) {
            FT_Done_FreeType(m_ft);
            m_ft = nullptr;
        }
    }

    // Disable copy/move for singleton-like behavior
    FontLib(const FontLib&) = delete;
    FontLib& operator=(const FontLib&) = delete;
    FontLib(FontLib&&) = delete;
    FontLib& operator=(FontLib&&) = delete;

    // Core interface
    bool init();
    void submitText(const TextRecord& text);
    void commitText();

    // Configuration
    void setCanvas(const glm::vec2& sz) noexcept { m_canvas = sz; }
    bool setCurrentFont(const std::string& fontname);

    // Atlas configuration
    void setAtlasSize(int size) noexcept {
        m_atlas_size = (size > 0) ? size : 1024;
    }
    int getAtlasSize() const noexcept { return m_atlas_size; }

    // Font resolution
    void setFontResolution(GLuint res) noexcept {
        m_font_res = (res > 0) ? res : 64;
    }
    GLuint getFontResolution() const noexcept { return m_font_res; }

    // Utility methods
    bool hasFont(const std::string& fontname) const noexcept {
        return m_fonts.find(fontname) != m_fonts.end();
    }

    size_t getFontCount() const noexcept { return m_fonts.size(); }

    // Text measurement (for layout purposes)
    glm::vec2 measureText(const std::string& text, const glm::vec2& size) const;

    // Batch management
    void clearBatch() noexcept { m_content.clear(); }
    size_t getBatchSize() const noexcept { return m_content.size(); }

    // Memory management
    void optimizeMemory();  // Free unused resources

    // Shader access for advanced usage
    const Shader& getShader() const noexcept { return m_font_shader; }
    Shader& getShader() noexcept { return m_font_shader; }
};