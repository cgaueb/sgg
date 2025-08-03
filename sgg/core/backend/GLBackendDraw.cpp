#include "core/backend/GLBackend.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <cctype>
#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>
#include <thread>
#include "core/graphics/rendering/batching/headers/BatchRenderer.h"

#ifdef _WIN32
// Required for EnumDisplaySettings, DEVMODE, and related Windows API constants
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef  _EXPERIMENTAL_FILESYSTEM_
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

#ifdef __APPLE__
#define sggBindVertexArray glBindVertexArrayAPPLE
#define sggGenVertexArrays glGenVertexArraysAPPLE
#else
#define sggBindVertexArray glBindVertexArray
#define sggGenVertexArrays glGenVertexArrays
#endif

// Use constexpr for shader sources
constexpr const char *PrimitivesVertexShader = R"(
    #version 330 core
    in vec4 coord;           // xyz = position, w = legacy/unused
    out vec2 texcoord;       // Used for gradients & texturing
    uniform mat4 MV;
    uniform mat4 P;
    void main(void) {
        gl_Position = P * MV * vec4(coord.xyz, 1.0);
        texcoord = coord.xy + vec2(0.5); // Map local [-0.5,0.5] position range to [0,1] UV space
    }
)";

constexpr const char *SolidFragmentShader = R"(
    #version 330 core
    in vec2 texcoord;
    out vec4 FragColor;
    uniform vec4 color1;
    uniform vec4 color2;
    uniform sampler2D tex;
    uniform int has_texture;
    uniform vec2 gradient;
    void main(void) {
        vec4 color = mix(color1, color2, dot(texcoord, gradient));
        vec4 tex_color = texture(tex, texcoord);
        if (has_texture > 0)
            FragColor = color * tex_color;
        else
            FragColor = color;
    }
)";

namespace graphics {
    namespace {
        // Cached values to avoid recomputation
        constexpr float PI = 3.1415936f;
        constexpr float DEG_TO_RAD = PI / 180.0f;
        constexpr int TEXTURE_UNIT = 30;

        // Pre-computed vertex data as constexpr
        constexpr std::array<std::array<float, 4>, 4> BOX_VERTICES = {
            {
                {{-0.5f, 0.5f, 0.0f, 1.0f}},
                {{0.5f, 0.5f, 1.0f, 1.0f}},
                {{-0.5f, -0.5f, 0.0f, 0.0f}},
                {{0.5f, -0.5f, 1.0f, 0.0f}}
            }
        };

        constexpr std::array<std::array<float, 4>, 4> BOX_OUTLINE_VERTICES = {
            {
                {{-0.5f, 0.5f, 0.0f, 0.0f}},
                {{0.5f, 0.5f, 1.0f, 0.0f}},
                {{0.5f, -0.5f, 1.0f, 1.0f}},
                {{-0.5f, -0.5f, 0.0f, 1.0f}}
            }
        };

        // Static matrices to avoid repeated construction
        thread_local glm::mat4 identity_matrix(1.0f);
        thread_local glm::vec2 default_gradient(1.0f, 0.0f);

        // Helper function for efficient color vector creation
        inline glm::vec4 makeColorVec4(const float *color, float opacity) {
            return glm::vec4(color[0], color[1], color[2], opacity);
        }

        // Fast sine/cosine computation for sectors
        inline void fastSinCos(float angle, float &sinVal, float &cosVal) {
            sinVal = std::sin(angle);
            cosVal = std::cos(angle);
        }
    }

    void GLBackend::initPrimitives() {
        m_flat_shader = Shader(PrimitivesVertexShader, SolidFragmentShader);
        m_flat_shader.use(false);

        // Precache commonly used uniforms for better performance
        m_flat_shader.precacheUniforms({
            "has_texture", "gradient", "color1", "color2", "MV", "P", "tex"
        });

        // Initialize sector vertices once
        std::array<std::array<float, 4>, 2 * CURVE_SUBDIVS> sector_vertices;
        std::array<std::array<float, 4>, 2 * CURVE_SUBDIVS> sector_outline_vertices;

        constexpr float r1 = 0.0f, r2 = 1.0f;
        constexpr float angle_step = PI / CURVE_SUBDIVS;

        for (int i = 0; i < CURVE_SUBDIVS; ++i) {
            const float angle = angle_step * i;
            const float s = static_cast<float>(i) / CURVE_SUBDIVS;
            float sin_val, cos_val;
            fastSinCos(angle, sin_val, cos_val);

            // Inner vertex
            sector_vertices[i * 2][0] = r1 * sin_val;
            sector_vertices[i * 2][1] = r1 * cos_val;
            sector_vertices[i * 2][2] = s;
            sector_vertices[i * 2][3] = 0.0f;

            // Outer vertex
            sector_vertices[i * 2 + 1][0] = r2 * sin_val;
            sector_vertices[i * 2 + 1][1] = r2 * cos_val;
            sector_vertices[i * 2 + 1][2] = s;
            sector_vertices[i * 2 + 1][3] = 1.0f;

            // Outline vertices
            sector_outline_vertices[i][0] = r1 * sin_val;
            sector_outline_vertices[i][1] = r1 * cos_val;
            sector_outline_vertices[i][2] = s;
            sector_outline_vertices[i][3] = 0.0f;

            const int reverse_idx = 2 * CURVE_SUBDIVS - i - 1;
            sector_outline_vertices[reverse_idx][0] = r2 * sin_val;
            sector_outline_vertices[reverse_idx][1] = r2 * cos_val;
            sector_outline_vertices[reverse_idx][2] = s;
            sector_outline_vertices[reverse_idx][3] = 1.0f;
        }

        // Get vertex attribute location once
        const auto attrib_position = m_flat_shader.getAttributeLocation("coord");
        if (!attrib_position.has_value()) {
            std::cerr << "Error: Could not find 'coord' attribute in shader" << std::endl;
            return;
        }
        const GLuint coord_attrib = attrib_position.value();

        // Helper lambda to create VAO/VBO pair with vertex attributes
        auto createVAOWithAttributes = [coord_attrib](GLuint &vao, GLuint &vbo, const void *data, GLsizei size,
                                                      GLenum usage) {
            sggGenVertexArrays(1, &vao);
            sggBindVertexArray(vao);
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            if (data) {
                glBufferData(GL_ARRAY_BUFFER, size, data, usage);
            } else {
                glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
            }
            glEnableVertexAttribArray(coord_attrib);
            glVertexAttribPointer(coord_attrib, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
        };

        // Create all VAOs and VBOs with proper vertex attributes
        createVAOWithAttributes(m_sector_vao, m_sector_vbo, sector_vertices.data(), sizeof(sector_vertices),
                                GL_DYNAMIC_DRAW);
        createVAOWithAttributes(m_sector_outline_vao, m_sector_outline_vbo, sector_outline_vertices.data(),
                                sizeof(sector_outline_vertices), GL_DYNAMIC_DRAW);
        createVAOWithAttributes(m_line_vao, m_line_vbo, nullptr, 2 * 4 * sizeof(float), GL_DYNAMIC_DRAW);
        createVAOWithAttributes(m_rect_vao, m_rect_vbo, BOX_VERTICES.data(), sizeof(BOX_VERTICES), GL_STATIC_DRAW);
        createVAOWithAttributes(m_rect_outline_vao, m_rect_outline_vbo, BOX_OUTLINE_VERTICES.data(),
                                sizeof(BOX_OUTLINE_VERTICES), GL_STATIC_DRAW);

        // Create triangle VAOs that were missing
        createVAOWithAttributes(m_triangle_vao, m_triangle_vbo, nullptr, 3 * 4 * sizeof(float), GL_DYNAMIC_DRAW);
        createVAOWithAttributes(m_triangle_outline_vao, m_triangle_outline_vbo, nullptr, 3 * 4 * sizeof(float),
                                GL_DYNAMIC_DRAW);

        // Unbind to prevent accidental modifications
        sggBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void GLBackend::drawTriangle(float x1, float y1, float z1,
                                 float x2, float y2, float z2,
                                 float x3, float y3, float z3,
                                 const Brush &brush) {
        std::array<std::array<float, 4>, 3> triangle{
            {
                {x1, y1, z1, 1.0f},
                {x2, y2, z2, 1.0f},
                {x3, y3, z3, 1.0f}
            }
        };
        drawTriangleInternal(triangle, brush);
    }

    // Backwards-compatible 2-D wrapper – z = 0
    void GLBackend::drawTriangle(float x1, float y1,
                                 float x2, float y2,
                                 float x3, float y3,
                                 const Brush &brush) {
        drawTriangle(x1, y1, 0.0f,
                     x2, y2, 0.0f,
                     x3, y3, 0.0f,
                     brush);
    }

    // Extracted shared body from old implementation (was inline in previous drawTriangle)
    void GLBackend::drawTriangleInternal(const std::array<std::array<float, 4>, 3> &triangle,
                                         const Brush &brush) {
        if (!(brush.fill_opacity > 0.0f || brush.fill_secondary_opacity > 0.0f)) {
            return;
        }

        Texture *texture = nullptr;
        bool has_texture = false;
        if (!brush.texture.empty()) {
            texture = textureManager->getTexture(brush.texture);
            if (texture) {
                textureManager->bindTexture(texture, TEXTURE_UNIT);
                has_texture = true;
            }
        }

        m_flat_shader["has_texture"] = has_texture ? 1 : 0;
        m_flat_shader["gradient"] = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);
        m_flat_shader["color1"] = makeColorVec4(brush.fill_color, brush.fill_opacity);
        m_flat_shader["color2"] = brush.gradient
                                      ? makeColorVec4(brush.fill_secondary_color, brush.fill_secondary_opacity)
                                      : makeColorVec4(brush.fill_color, brush.fill_opacity);
        m_flat_shader["MV"] = identity_matrix;
        m_flat_shader["tex"] = TEXTURE_UNIT;

        sggBindVertexArray(m_triangle_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_triangle_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle.data(), GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        if (brush.outline_opacity > 0.0f) {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_EQUAL);
            const glm::vec4 outline_color = makeColorVec4(brush.outline_color,
                                                          brush.outline_opacity);
            m_flat_shader["color1"] = outline_color;
            m_flat_shader["color2"] = outline_color;
            m_flat_shader["MV"] = identity_matrix;
            m_flat_shader["has_texture"] = 0;

            glLineWidth(brush.outline_width);
            sggBindVertexArray(m_triangle_outline_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_triangle_outline_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle.data(),
                         GL_STREAM_DRAW);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
            glLineWidth(1.0f);

            glDepthFunc(GL_LEQUAL); // restore default
            glDepthMask(GL_TRUE);
        }

        if (texture) {
            textureManager->unbindTexture(TEXTURE_UNIT);
        }
    }

    // 3-D overload (user supplies cz); internal logic unchanged except translate z.
    void GLBackend::drawRect(float cx, float cy, float cz, float w, float h, const Brush &brush) {
        glFrontFace(GL_CCW);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        const glm::mat4 transform_matrix =
                glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, cz)) *
                m_transformation *
                glm::scale(glm::mat4(1.0f), glm::vec3(w, h, 1.0f));

        if (brush.fill_opacity > 0.0f || brush.fill_secondary_opacity > 0.0f) {
            Texture *texture = nullptr;
            bool has_texture = false;

            if (!brush.texture.empty()) {
                texture = textureManager->getTexture(brush.texture);
                if (texture) {
                    textureManager->bindTexture(texture, TEXTURE_UNIT);
                    has_texture = true;
                }
            }

            // Set shader uniforms using new interface
            m_flat_shader["has_texture"] = has_texture ? 1 : 0;
            m_flat_shader["gradient"] = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);
            m_flat_shader["color1"] = makeColorVec4(brush.fill_color, brush.fill_opacity);
            m_flat_shader["color2"] = brush.gradient
                                          ? makeColorVec4(brush.fill_secondary_color, brush.fill_secondary_opacity)
                                          : makeColorVec4(brush.fill_color, brush.fill_opacity);
            m_flat_shader["MV"] = transform_matrix;
            m_flat_shader["tex"] = TEXTURE_UNIT;

            // Render rectangle - VAO already has vertex attributes configured
            sggBindVertexArray(m_rect_vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (texture) {
                textureManager->unbindTexture(TEXTURE_UNIT);
            }
        }

        if (brush.outline_opacity > 0.0f) {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_EQUAL);

            const glm::vec4 outline_color = makeColorVec4(brush.outline_color, brush.outline_opacity);
            m_flat_shader["color1"] = outline_color;
            m_flat_shader["color2"] = outline_color;
            m_flat_shader["MV"] = transform_matrix;
            m_flat_shader["has_texture"] = 0;

            glLineWidth(brush.outline_width);
            sggBindVertexArray(m_rect_outline_vao);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            glLineWidth(1.0f);

            glDepthFunc(GL_LEQUAL); // restore default
            glDepthMask(GL_TRUE);
        }
    }

    // 2-D wrapper maintains old API (cz = 0)
    void GLBackend::drawRect(float cx, float cy, float w, float h, const Brush &brush) {
        drawRect(cx, cy, 0.0f, w, h, brush);
    }

    // 3-D line overload
    void GLBackend::drawLine(float x1, float y1, float z1,
                             float x2, float y2, float z2,
                             const Brush &brush) {
        m_flat_shader["color1"] = makeColorVec4(brush.outline_color, brush.outline_opacity);
        m_flat_shader["MV"] = identity_matrix;
        m_flat_shader["gradient"] = default_gradient;

        std::array<std::array<float, 4>, 2> line{
            {
                {x1, y1, z1, 1.0f},
                {x2, y2, z2, 1.0f}
            }
        };

        sggBindVertexArray(m_line_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(line), line.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, 2);
    }

    // 2-D wrapper
    void GLBackend::drawLine(float x_1, float y_1, float x_2, float y_2, const Brush &brush) {
        drawLine(x_1, y_1, 0.0f, x_2, y_2, 0.0f, brush);
    }

    // 3-D capable sector rendering
    void GLBackend::drawSector(float cx, float cy, float cz,
                               float start_angle, float end_angle,
                               float radius1, float radius2,
                               const Brush &brush) {
        glFrontFace(GL_CCW);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        const glm::mat4 transform_matrix =
                glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, cz)) * m_transformation;

        m_flat_shader["MV"] = transform_matrix;
        m_flat_shader["gradient"] = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);

        // Pre-compute angle values
        const float start_rad = start_angle * DEG_TO_RAD;
        const float arc_inc = (end_angle - start_angle) * DEG_TO_RAD / CURVE_SUBDIVS;

        // Use stack arrays for better performance
        std::array<std::array<float, 4>, 2 * CURVE_SUBDIVS + 2> sector_vertices;
        std::array<std::array<float, 4>, 2 * CURVE_SUBDIVS + 2> sector_outline_vertices;

        // Generate vertices more efficiently
        for (int i = 0; i <= CURVE_SUBDIVS; ++i) {
            const float angle = start_rad + i * arc_inc;
            const float s = static_cast<float>(i) / CURVE_SUBDIVS;
            float sin_val, cos_val;
            fastSinCos(angle, sin_val, cos_val);

            // Fill vertices
            sector_vertices[i * 2][0] = radius1 * cos_val;
            sector_vertices[i * 2][1] = -radius1 * sin_val;
            sector_vertices[i * 2][2] = s;
            sector_vertices[i * 2][3] = 0.0f;

            sector_vertices[i * 2 + 1][0] = radius2 * cos_val;
            sector_vertices[i * 2 + 1][1] = -radius2 * sin_val;
            sector_vertices[i * 2 + 1][2] = s;
            sector_vertices[i * 2 + 1][3] = 1.0f;

            // Outline vertices
            if (i <= CURVE_SUBDIVS) {
                sector_outline_vertices[i][0] = radius1 * cos_val;
                sector_outline_vertices[i][1] = -radius1 * sin_val;
                sector_outline_vertices[i][2] = s;
                sector_outline_vertices[i][3] = 0.0f;
            }
        }

        // Generate outer outline vertices in reverse
        for (int i = CURVE_SUBDIVS; i >= 0; --i) {
            const float angle = start_rad + i * arc_inc;
            const float s = static_cast<float>(i) / CURVE_SUBDIVS;
            float sin_val, cos_val;
            fastSinCos(angle, sin_val, cos_val);

            const int idx = 2 * CURVE_SUBDIVS - i + 1;
            sector_outline_vertices[idx][0] = radius2 * cos_val;
            sector_outline_vertices[idx][1] = -radius2 * sin_val;
            sector_outline_vertices[idx][2] = s;
            sector_outline_vertices[idx][3] = 1.0f;
        }

        if (brush.fill_opacity > 0.0f || brush.fill_secondary_opacity > 0.0f) {
            Texture *texture = nullptr;
            bool has_texture = false;

            if (!brush.texture.empty()) {
                texture = textureManager->getTexture(brush.texture);
                if (texture) {
                    textureManager->bindTexture(texture, TEXTURE_UNIT);
                    has_texture = true;
                }
            }

            m_flat_shader["has_texture"] = has_texture ? 1 : 0;
            m_flat_shader["color1"] = makeColorVec4(brush.fill_color, brush.fill_opacity);
            m_flat_shader["color2"] = brush.gradient
                                          ? makeColorVec4(brush.fill_secondary_color, brush.fill_secondary_opacity)
                                          : makeColorVec4(brush.fill_color, brush.fill_opacity);
            m_flat_shader["tex"] = TEXTURE_UNIT;

            sggBindVertexArray(m_sector_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_sector_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(sector_vertices), sector_vertices.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * CURVE_SUBDIVS + 2);

            if (texture) {
                textureManager->unbindTexture(TEXTURE_UNIT);
            }
        }

        if (brush.outline_opacity > 0.0f) {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_EQUAL);

            const glm::vec4 outline_color = makeColorVec4(brush.outline_color, brush.outline_opacity);
            m_flat_shader["color1"] = outline_color;
            m_flat_shader["color2"] = outline_color;
            m_flat_shader["has_texture"] = 0;

            glLineWidth(brush.outline_width);
            sggBindVertexArray(m_sector_outline_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_sector_outline_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(sector_outline_vertices), sector_outline_vertices.data(),
                         GL_DYNAMIC_DRAW);

            constexpr float FULL_CIRCLE_THRESHOLD = 0.001f;
            if (std::abs(end_angle - start_angle - 360.0f) > FULL_CIRCLE_THRESHOLD) {
                glDrawArrays(GL_LINE_LOOP, 0, 2 * CURVE_SUBDIVS + 2);
            } else {
                glDrawArrays(GL_LINE_LOOP, CURVE_SUBDIVS + 1, CURVE_SUBDIVS);
            }
            glLineWidth(1.0f);

            glDepthFunc(GL_LEQUAL); // restore default
            glDepthMask(GL_TRUE);
        }
    }

    // 2-D wrapper maintains legacy API (cz = 0)
    void GLBackend::drawSector(float cx, float cy,
                               float start_angle, float end_angle,
                               float radius1, float radius2,
                               const Brush &brush) {
        drawSector(cx, cy, 0.0f, start_angle, end_angle, radius1, radius2, brush);
    }

    void GLBackend::drawText(float pos_x, float pos_y, float size, const std::string &text, const Brush &brush) {
        TextRecord entry;
        entry.text = text;
        entry.pos = glm::vec2(pos_x, pos_y);
        entry.size = glm::vec2(size, size);
        entry.color1 = makeColorVec4(brush.fill_color, brush.fill_opacity);
        entry.color2 = makeColorVec4(brush.fill_secondary_color, brush.fill_secondary_opacity);
        entry.gradient = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);
        entry.use_gradient = brush.gradient;
        entry.mv = m_transformation;
        entry.proj = m_ui_projection;

        //std::cout << "Drawing text at: " << pos_x << ", " << pos_y
          //<< " Canvas: " << m_canvas.x << "x" << m_canvas.y
          //<< " Window: " << m_width << "x" << m_height << std::endl;

        m_fontlib.submitText(entry);
    }

    std::vector<std::string> GLBackend::preloadBitmaps(std::string dir) {
        std::vector<std::string> names;
        names.reserve(100); // Reserve space to avoid reallocations

        try {
            for (const auto &entry: fs::directory_iterator(dir)) {
                const std::string filename = entry.path().string();
                if (filename.length() < 4) continue;

                std::string extension = filename.substr(filename.length() - 4);
                std::transform(extension.begin(), extension.end(), extension.begin(),
                               [](unsigned char c) { return std::tolower(c); });

                if (extension == ".png" && textureManager->getTexture(filename)) {
                    names.emplace_back(filename);
                }
            }
        } catch (const fs::filesystem_error &ex) {
            std::cerr << "Filesystem error in preloadBitmaps: " << ex.what() << '\n';
        }

        return names;
    }

    void GLBackend::draw() {
    m_flat_shader.use(true);

    static bool first_time = true;
    if (first_time) {
        first_time = false;
        resize(m_width, m_height);
        return;
    }

    // EMERGENCY FIX: Force complete OpenGL state reset every frame
    // This is not optimal for performance but should fix coordinate issues

    // 1. Force viewport reset
    glViewport(0, 0, m_width, m_height);

    // 2. Force coordinate system recalculation
    static int emergency_last_width = -1;
    static int emergency_last_height = -1;
    bool force_recalc = (emergency_last_width != m_width || emergency_last_height != m_height);

    if (force_recalc) {
        std::cout << "EMERGENCY: Forcing coordinate recalc " << emergency_last_width << "x" << emergency_last_height
                  << " -> " << m_width << "x" << m_height << std::endl;

        // Force complete resize recalculation
        resize(m_width, m_height);

        // Reset any cached projection matrices
        resetPose();

        emergency_last_width = m_width;
        emergency_last_height = m_height;
    }

    // 3. Ensure clean OpenGL state
    // Fixed-function matrix stack calls removed (core-profile invalid).
    // Projection/View matrices are managed in computeProjection() and resetPose().
    resetPose();
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool use_scissor = false;

    if (m_canvas_mode == CANVAS_SCALE_FIT) {
        glm::vec4 scissor_rect;
        const float true_aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        const float req_aspect = m_requested_canvas.z / m_requested_canvas.w;
        const bool width_constrained = true_aspect > req_aspect;

        scissor_rect.x = width_constrained ? (m_width - m_height * req_aspect) * 0.5f : 0.0f;
        scissor_rect.y = !width_constrained ? (m_height - m_width / req_aspect) * 0.5f : 0.0f;
        scissor_rect.z = width_constrained ? m_height * req_aspect : static_cast<float>(m_width);
        scissor_rect.w = !width_constrained ? m_width / req_aspect : static_cast<float>(m_height);

        glEnable(GL_SCISSOR_TEST);
        glScissor(static_cast<GLint>(scissor_rect.x), static_cast<GLint>(scissor_rect.y),
                  static_cast<GLsizei>(scissor_rect.z), static_cast<GLsizei>(scissor_rect.w));
        use_scissor = true;
    }

    // ----- CPU timing start (measure render/update cost) -----
    const auto cpu_start_time = std::chrono::high_resolution_clock::now();

    // Draw background first, before any predraw setup
    Brush background_brush;
    background_brush.fill_color[0] = m_back_color.r;
    background_brush.fill_color[1] = m_back_color.g;
    background_brush.fill_color[2] = m_back_color.b;
    background_brush.fill_opacity = 1.0f;
    background_brush.outline_opacity = 0.0f;

    const float bg_width = (m_canvas_mode == CANVAS_SCALE_WINDOW) ? static_cast<float>(m_width) : m_requested_canvas.z;
    const float bg_height = (m_canvas_mode == CANVAS_SCALE_WINDOW) ? static_cast<float>(m_height) : m_requested_canvas.w;
    const float half_width = bg_width * 0.5f;
    const float half_height = bg_height * 0.5f;

    drawRect(half_width, half_height, bg_width, bg_height, background_brush);

    if (m_predraw_callback) {
        m_predraw_callback();
    }

    m_flat_shader.use();
    if (use_scissor) {
        glEnable(GL_SCISSOR_TEST);
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // EMERGENCY: Force projection matrix recalculation every frame when size changes
    if (force_recalc || first_time) {
        // Recompute both world and UI projection matrices
        computeProjection();
        computeUIProjection();
    }
    
    m_flat_shader["P"] = m_projection;

    // EMERGENCY: Force font library to reset everything
    glm::vec2 canvas_size;
    if (m_canvas_mode == CANVAS_SCALE_WINDOW) {
        canvas_size = glm::vec2(static_cast<float>(m_width), static_cast<float>(m_height));
    } else {
        canvas_size = glm::vec2(m_requested_canvas.z, m_requested_canvas.w);
    }

    // Force font library reset on every size change
    if (force_recalc) {
        std::cout << "EMERGENCY: Forcing font library reset for " << canvas_size.x << "x" << canvas_size.y << std::endl;

        // Clear everything and start fresh
        m_fontlib.setCanvas(canvas_size);

    } else {
        m_fontlib.setCanvas(canvas_size);
    }

    if (m_draw_callback) {
        m_draw_callback();
    }

    // Render all queued text with depth testing disabled so UI is always visible
    glDisable(GL_DEPTH_TEST);
    m_fontlib.commitText();
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_SCISSOR_TEST);

    if (m_postdraw_callback) {
        m_postdraw_callback();
    }

    swap();

    // ----- Timing measurements -----
    const auto cpu_end_time = std::chrono::high_resolution_clock::now();
    const double render_time = std::chrono::duration<double, std::milli>(cpu_end_time - cpu_start_time).count();

    // Store CPU render time for external querying
    m_lastRenderTimeMs = render_time;

    const auto wait_start_time = cpu_end_time; // use as reference for waiting calculations

    int current_fps_limit;
    if (s_vsync_enabled) {
#ifdef _WIN32
        DEVMODE dev_mode;
        current_fps_limit = (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dev_mode))
                                ? static_cast<int>(dev_mode.dmDisplayFrequency)
                                : 60;
#else
        current_fps_limit = 60;
#endif
    } else {
        current_fps_limit = perfMon.getFPSLimit();
    }

    // (actualFPS already updated above)

    if (current_fps_limit > 0) {
        const double target_frame_time = 1000.0 / current_fps_limit;
        const auto target_end_time = wait_start_time +
                                     std::chrono::microseconds(static_cast<long long>(target_frame_time * 1000));

        if (render_time < target_frame_time) {
            while (std::chrono::high_resolution_clock::now() < target_end_time) {
                std::this_thread::yield();
            }
        }
    }

    // End-frame timing after waiting so frame time includes any sleep/vsync delay
    perfMon.endFrame();

    // Update smoothed FPS after endFrame()
    actualFPS = perfMon.getFPS();
}
}
