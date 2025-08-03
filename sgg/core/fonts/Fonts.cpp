#include "core/fonts/headers/Fonts.h"
#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <cmath>
#include <cstring>  // For memcpy

#include "core/graphics/textures/headers/Texture.h"
#include "core/graphics/textures/headers/TextureManager.h"

// Platform-specific OpenGL bindings
#ifdef __APPLE__
#define sggBindVertexArray glBindVertexArrayAPPLE
#define sggGenVertexArrays glGenVertexArraysAPPLE
#else
#define sggBindVertexArray glBindVertexArray
#define sggGenVertexArrays glGenVertexArrays
#endif

// Optimized shaders - same functionality, better formatted
static constexpr const char* FONT_VERTEX_SHADER = R"(
#version 120
attribute vec4 coord;
varying vec2 texcoord;
uniform mat4 projection;
uniform mat4 modelview;

void main(void) {
    gl_Position = projection * modelview * vec4(coord.xy, 0, 1);
    texcoord = coord.zw;
}
)";

static constexpr const char* FONT_FRAGMENT_SHADER = R"(
#version 120
varying vec2 texcoord;
uniform vec4 color1;
uniform vec4 color2;
uniform sampler2D tex;
uniform vec2 gradient;

void main(void) {
    vec4 color = mix(color1, color2, dot(texcoord, gradient));
    gl_FragColor = vec4(1, 1, 1, texture2D(tex, texcoord).r) * color;
}
)";

// Static member initialization
FT_Library FontLib::m_ft = nullptr;

// Optimized initialization
bool FontLib::init() {
    // Initialize FreeType library
    if (FT_Init_FreeType(&m_ft)) {
        std::cerr << "ERROR [FontLib::init]: Failed to initialize FreeType library\n";
        return false;
    }

    // Create shader program using modern Shader interface
    try {
        m_font_shader = Shader(FONT_VERTEX_SHADER, FONT_FRAGMENT_SHADER);

        if (!m_font_shader.isReady()) {
            std::cerr << "ERROR [FontLib::init]: Font shader failed to initialize properly\n";
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR [FontLib::init]: Font shader creation failed: " << e.what() << '\n';
        return false;
    }

    // Get attribute location using modern interface
    auto attrib_position_opt = m_font_shader.getAttributeLocation("coord");
    if (!attrib_position_opt.has_value()) {
        std::cerr << "ERROR [FontLib::init]: Failed to get 'coord' attribute location\n";
        return false;
    }
    const unsigned int attrib_position = attrib_position_opt.value();

    // Setup legacy VAO/VBO for compatibility
    sggGenVertexArrays(1, &m_font_vao);
    sggBindVertexArray(m_font_vao);

    glGenBuffers(1, &m_font_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_font_vbo);
    glEnableVertexAttribArray(attrib_position);
    glVertexAttribPointer(attrib_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Static quad data for legacy rendering
    static constexpr GLfloat box[4][4] = {
        {-1, 1, 0, 0},
        {1, 1, 1, 0},
        {-1, -1, 0, 1},
        {1, -1, 1, 1},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);

    // Setup atlas text rendering VAO/VBO
    sggGenVertexArrays(1, &m_text_vao);
    sggBindVertexArray(m_text_vao);

    glGenBuffers(1, &m_text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_text_vbo);
    glEnableVertexAttribArray(attrib_position);
    glVertexAttribPointer(attrib_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Pre-allocate buffer for up to 2048 characters (8KB)
    static constexpr size_t MAX_CHARS = 2048;
    static constexpr size_t BUFFER_SIZE = sizeof(TextVertex) * 4 * MAX_CHARS;
    glBufferData(GL_ARRAY_BUFFER, BUFFER_SIZE, nullptr, GL_DYNAMIC_DRAW);

    // Reserve memory for vertex buffers to avoid reallocations
    m_vertex_buffer.reserve(MAX_CHARS * 6);  // 6 vertices per character
    m_gl_buffer.reserve(MAX_CHARS * 24);     // 4 floats * 6 vertices per character

    // Precache common uniforms for better performance
    m_font_shader.precacheUniforms({
        "tex", "color1", "color2", "gradient", "projection", "modelview"
    });

    m_curr_font = m_fonts.end();
    return true;
}

// Optimized text submission
void FontLib::submitText(const TextRecord& text) {
    if (!isValidFont()) return;

    // Create a copy and set the font pointer
    TextRecord record = text;
    record.font = &(m_curr_font->second);
    m_content.emplace_back(std::move(record));
}

// Optimized OpenGL state management
ATTR_GNU_HOT void FontLib::setupOpenGLState() noexcept {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_font_shader.bind();  // Use modern bind() method
    glActiveTexture(GL_TEXTURE0 + 31);
}

[[gnu::hot]] void FontLib::cleanupOpenGLState() noexcept {
    m_font_shader.unbind();  // Use modern unbind() method
}

// Heavily optimized drawText function
[[gnu::hot]] void FontLib::drawText(const TextRecord& entry) {
    if (!entry.font) return;

    Font& font = *entry.font;

    // Lazy atlas generation
    if (!font.atlas.initialized) [[unlikely]] {
        if (!generateAtlas(font)) {
            std::cerr << "ERROR [FontLib::drawText]: Failed to generate atlas for font: "
                      << font.fontname << '\n';
            return;
        }
    }

    // Build text mesh - reuse member buffer to avoid allocations
    m_vertex_buffer.clear();
    buildTextMesh(m_vertex_buffer, entry.text, font, glm::vec2(0.0f), entry.size);

    // Early exit if no vertices to render
    if (m_vertex_buffer.empty()) [[unlikely]] {
        return;
    }

    // Convert to interleaved format - reuse member buffer
    const size_t vertexCount = m_vertex_buffer.size();
    const size_t dataSize = vertexCount * 4; // 4 floats per vertex

    // Ensure GL buffer has enough capacity
    if (m_gl_buffer.size() < dataSize) {
        m_gl_buffer.resize(dataSize);
    }
    // Fast interleaved conversion
    for (size_t i = 0; i < vertexCount; ++i) {
        const size_t baseIndex = i * 4;
        const TextVertex& vertex = m_vertex_buffer[i];

        m_gl_buffer[baseIndex]     = vertex.position.x;
        m_gl_buffer[baseIndex + 1] = vertex.position.y;
        m_gl_buffer[baseIndex + 2] = vertex.texcoord.x;
        m_gl_buffer[baseIndex + 3] = vertex.texcoord.y;
    }

    // Setup OpenGL state (inlined for performance)
    glBindTexture(GL_TEXTURE_2D, font.atlas.texture_id);

    // Set shader uniforms using modern interface
    m_font_shader["tex"] = 31;
    m_font_shader["color1"] = entry.color1;
    m_font_shader["color2"] = entry.use_gradient ? entry.color2 : entry.color1;
    m_font_shader["gradient"] = entry.gradient;
    m_font_shader["projection"] = entry.proj;
    m_font_shader["modelview"] = glm::translate(glm::vec3(entry.pos.x, entry.pos.y, 0.0f)) * entry.mv;

    // Upload vertex data
    sggBindVertexArray(m_text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_text_vbo);

    const size_t bytesToUpload = dataSize * sizeof(GLfloat);
    static constexpr size_t MAX_SUBDATA_SIZE = 2048 * 24 * sizeof(GLfloat); // Our pre-allocated size

    if (bytesToUpload <= MAX_SUBDATA_SIZE) [[likely]] {
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytesToUpload, m_gl_buffer.data());
    } else {
        // Fallback for very large text
        glBufferData(GL_ARRAY_BUFFER, bytesToUpload, m_gl_buffer.data(), GL_DYNAMIC_DRAW);
    }

    // Render
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

// Optimized commit function
void FontLib::commitText() {
    if (m_content.empty()) return;

    setupOpenGLState();
    // Process all text records
    for (const auto& item : m_content) {
        drawText(item);
    }

    // Ensure scissor test is disabled before cleanup
    glDisable(GL_SCISSOR_TEST);

    cleanupOpenGLState();
    m_content.clear();
}

// Optimized font loading
bool FontLib::setCurrentFont(const std::string& fontname) {
    // Check if font already exists
    m_curr_font = m_fonts.find(fontname);
    if (m_curr_font != m_fonts.end()) [[likely]] {
        return true;
    }

    // Create new font
    Font font(fontname);

    // Load FreeType face
    if (FT_New_Face(m_ft, fontname.c_str(), 0, &font.face)) {
        std::cerr << "ERROR [FontLib::setCurrentFont]: Failed to load font: " << fontname << '\n';
        return false;
    }

    FT_Set_Pixel_Sizes(font.face, 0, m_font_res);

    // Create legacy texture for compatibility
    auto& textureManager = graphics::TextureManager::getInstance();
    graphics::Texture* tex = textureManager.createTexture(fontname, false, [](graphics::Texture& tex) {
        glGenTextures(1, tex.getIDPointer());
        glBindTexture(GL_TEXTURE_2D, tex.getID());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    });

    font.font_tex = tex->getID();

    // Initialize atlas
    font.atlas = FontAtlas(m_atlas_size, m_atlas_size);

    // Insert font and set as current
    auto result = m_fonts.emplace(fontname, std::move(font));
    m_curr_font = result.first;

    return true;
}

// Optimized atlas generation with better memory management - FIXED VERSION
bool FontLib::generateAtlas(Font& font) {
    if (font.atlas.initialized) [[unlikely]] {
        return true;
    }

    // Generate texture
    glGenTextures(1, &font.atlas.texture_id);
    if (font.atlas.texture_id == 0) {
        std::cerr << "ERROR [FontLib::generateAtlas]: Failed to generate texture\n";
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, font.atlas.texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Pre-allocate atlas data
    const size_t atlasSize = static_cast<size_t>(font.atlas.width) * font.atlas.height;
    std::vector<unsigned char> atlasData(atlasSize, 0);

    // Atlas packing variables
    int x = 0, y = 0, rowHeight = 0;
    static constexpr int PADDING = 1;
    static constexpr unsigned char FIRST_CHAR = 32;
    static constexpr unsigned char LAST_CHAR = 126;
    static constexpr size_t CHAR_COUNT = LAST_CHAR - FIRST_CHAR + 1;

    // Pre-load all glyph metrics to optimize layout
    struct GlyphMetrics {
        unsigned int width, height;
        int left, top;
        float advance;
        std::vector<unsigned char> bitmap;

        GlyphMetrics() = default;
        GlyphMetrics(unsigned int w, unsigned int h, int l, int t, float a)
            : width(w), height(h), left(l), top(t), advance(a) {}
    };

    std::vector<GlyphMetrics> metrics;
    metrics.reserve(CHAR_COUNT);

    // Load all glyph metrics
    for (unsigned char c = FIRST_CHAR; c <= LAST_CHAR; ++c) {
        if (FT_Load_Char(font.face, c, FT_LOAD_RENDER)) {
            continue;
        }

        FT_GlyphSlot glyph = font.face->glyph;
        GlyphMetrics metric(
            glyph->bitmap.width,
            glyph->bitmap.rows,
            glyph->bitmap_left,
            glyph->bitmap_top,
            glyph->advance.x / 64.0f
        );

        // Copy bitmap data if present
        if (metric.width > 0 && metric.height > 0) {
            const size_t bitmapSize = metric.width * metric.height;
            metric.bitmap.resize(bitmapSize);
            std::memcpy(metric.bitmap.data(), glyph->bitmap.buffer, bitmapSize);
        }

        metrics.emplace_back(std::move(metric));
    }

    // Pack glyphs into atlas
    size_t metricIndex = 0;
    for (unsigned char c = FIRST_CHAR; c <= LAST_CHAR; ++c, ++metricIndex) {
        if (metricIndex >= metrics.size()) break;

        const auto& metric = metrics[metricIndex];

        // FIXED: Always store glyph info, even for non-renderable characters like spaces
        GlyphInfo info;

        if (metric.width == 0 || metric.height == 0 || metric.bitmap.empty()) {
            // For non-renderable characters (like space), store advance info only
            info = GlyphInfo(
                0, 0,  // width, height
                metric.left, metric.top, metric.advance,
                glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f)  // dummy UV coords
            );
            font.glyphs[c] = info;
            continue;
        }

        // Check if we need a new row
        if (x + static_cast<int>(metric.width) + PADDING >= font.atlas.width) {
            x = 0;
            y += rowHeight + PADDING;
            rowHeight = 0;
        }

        // Check atlas bounds
        if (y + static_cast<int>(metric.height) + PADDING >= font.atlas.height) {
            std::cerr << "WARNING [FontLib::generateAtlas]: Atlas too small for all glyphs\n";
            break;
        }

        // Copy glyph to atlas - optimized row-by-row copy
        for (unsigned int row = 0; row < metric.height; ++row) {
            const size_t dstOffset = (y + row) * font.atlas.width + x;
            const size_t srcOffset = row * metric.width;
            std::memcpy(atlasData.data() + dstOffset,
                       metric.bitmap.data() + srcOffset,
                       metric.width);
        }

        // Store glyph info
        const float invWidth = 1.0f / font.atlas.width;
        const float invHeight = 1.0f / font.atlas.height;

        info = GlyphInfo(
            metric.width, metric.height,
            metric.left, metric.top, metric.advance,
            glm::vec2(x * invWidth, y * invHeight),
            glm::vec2((x + metric.width) * invWidth, (y + metric.height) * invHeight)
        );

        font.glyphs[c] = info;

        // Update position
        x += metric.width + PADDING;
        rowHeight = std::max(rowHeight, static_cast<int>(metric.height));
    }

    // Upload to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font.atlas.width, font.atlas.height,
                 0, GL_RED, GL_UNSIGNED_BYTE, atlasData.data());

    font.atlas.initialized = true;
    return true;
}

// Optimized mesh building - FIXED VERSION
[[gnu::hot]] void FontLib::buildTextMesh(std::vector<TextVertex>& vertices,
                                        const std::string& text,
                                        const Font& font,
                                        const glm::vec2& position,
                                        const glm::vec2& size) const {
    if (text.empty()) {
        vertices.clear();
        return;
    }

    const float scale_x = size.x / static_cast<float>(m_font_res);
    const float scale_y = size.y / static_cast<float>(m_font_res);

    // Count visible characters for precise allocation
    size_t visibleChars = 0;
    for (char c : text) {
        if (font.glyphs.find(static_cast<unsigned char>(c)) != font.glyphs.end()) {
            ++visibleChars;
        }
    }

    // Reserve exact space needed
    vertices.clear();
    vertices.reserve(visibleChars * 6);

    float x = position.x;
    const float y = position.y;

    // Process each character
    for (char c : text) {
        const auto it = font.glyphs.find(static_cast<unsigned char>(c));

        // Handle space character specially
        if (c == ' ') {
            // Use a default space width if space glyph doesn't exist
            if (it == font.glyphs.end()) {
                x += (m_font_res * 0.25f) * scale_x; // Default space width
            } else {
                x += it->second.advance * scale_x;
            }
            continue;
        }

        // Skip other non-renderable characters
        if (it == font.glyphs.end()) {
            continue;
        }

        const GlyphInfo& glyph = it->second;

        // Calculate quad dimensions
        const float w = glyph.width * scale_x;
        const float h = glyph.height * scale_y;
        const float xpos = x + (glyph.bearing_x * scale_x);
        const float ypos = y - (glyph.bearing_y * scale_y);

        // Add 6 vertices for two triangles (quad)
        // Triangle 1: top-left, bottom-left, bottom-right
        vertices.emplace_back(glm::vec2(xpos, ypos), glyph.uv_min);
        vertices.emplace_back(glm::vec2(xpos, ypos + h), glm::vec2(glyph.uv_min.x, glyph.uv_max.y));
        vertices.emplace_back(glm::vec2(xpos + w, ypos + h), glyph.uv_max);

        // Triangle 2: top-left, bottom-right, top-right
        vertices.emplace_back(glm::vec2(xpos, ypos), glyph.uv_min);
        vertices.emplace_back(glm::vec2(xpos + w, ypos + h), glyph.uv_max);
        vertices.emplace_back(glm::vec2(xpos + w, ypos), glm::vec2(glyph.uv_max.x, glyph.uv_min.y));

        // Advance cursor
        x += glyph.advance * scale_x;
    }
}

// Additional utility methods - FIXED VERSION
glm::vec2 FontLib::measureText(const std::string& text, const glm::vec2& size) const {
    if (!isValidFont() || text.empty()) {
        return glm::vec2(0.0f);
    }

    const Font& font = m_curr_font->second;
    const float scale_x = size.x / static_cast<float>(m_font_res);
    const float scale_y = size.y / static_cast<float>(m_font_res);

    float width = 0.0f;
    float max_bearing_y = 0.0f;
    float min_bearing_y = 0.0f;

    for (char c : text) {
        const auto it = font.glyphs.find(static_cast<unsigned char>(c));

        // Handle space character specially
        if (c == ' ') {
            if (it == font.glyphs.end()) {
                width += (m_font_res * 0.25f) * scale_x; // Default space width
            } else {
                width += it->second.advance * scale_x;
            }
            continue;
        }

        if (it == font.glyphs.end()) {
            continue;
        }

        const GlyphInfo& glyph = it->second;
        width += glyph.advance * scale_x;

        // Track vertical extents
        const float bearing_y = glyph.bearing_y * scale_y;
        const float glyph_bottom = bearing_y - (glyph.height * scale_y);

        max_bearing_y = std::max(max_bearing_y, bearing_y);
        min_bearing_y = std::min(min_bearing_y, glyph_bottom);
    }

    const float height = max_bearing_y - min_bearing_y;
    return glm::vec2(width, height);
}

void FontLib::optimizeMemory() {
    // Shrink vectors to fit their current size
    m_content.shrink_to_fit();
    m_vertex_buffer.shrink_to_fit();
    m_gl_buffer.shrink_to_fit();
}