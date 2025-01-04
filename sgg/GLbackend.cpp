#include <cstring>
#include "headers/GLbackend.h"
#include <iostream>
#include <string>
#include <chrono>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <filesystem>
#include <cctype>
#include "headers/Graphics.h"

#ifdef  _EXPERIMENTAL_FILESYSTEM_
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif


#ifdef __APPLE__
#define sggBindVertexArray glBindVertexArrayAPPLE)
#define sggGenVertexArrays glGenVertexArraysAPPLE)
#else
#define sggBindVertexArray glBindVertexArray
#define sggGenVertexArrays glGenVertexArrays
#endif

const char *__PrimitivesVertexShader = R"(
             #version 330 core

             in vec4 coord;
             out vec2 texcoord;
             uniform mat4 MV;
             uniform mat4 P;

             void main(void) {
                gl_Position = P * MV * vec4(coord.xy, 0.0, 1.0); // Ensure you're using 0.0 for float literals
                texcoord = coord.zw;
             }
        )";

const char *__SolidFragmentShader = R"(
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

graphics::TextureManager *graphics::TextureManager::instance = nullptr;

namespace graphics {
    const uint32_t delay = 5; // update every delay ms.
    float delta_time = 0.0f;

    GLBackend::GLBackend(int w, int h, std::string title)
        : m_width(w),
          m_height(h),
          m_title(std::move(title)) {
        textureManager = &TextureManager::getInstance();
    }


    void GLBackend::cleanup() {
        SDL_GL_DeleteContext(m_context);
        SDL_DestroyWindow(m_window);
        if (m_audio)
            delete m_audio;
        SDL_Quit();
    }

    void GLBackend::swap() {
        SDL_GL_SwapWindow(m_window);
    }

    GLBackend::~GLBackend() {
    }

    void GLBackend::makeCurrent() {
        SDL_GL_MakeCurrent(m_window, m_context);
    }

    void GLBackend::show(bool s) {
        if (s)
            SDL_ShowWindow(m_window);
        else
            SDL_HideWindow(m_window);
    }

    void GLBackend::setBackgroundColor(float r, float g, float b) {
        m_back_color = glm::vec3(r, g, b);
    }

    float GLBackend::getDeltaTime() {
        return m_delta_time;
    }

    float GLBackend::getGlobalTime() {
        return m_global_time;
    }

    void GLBackend::getMouseButtonPressed(bool *button_array) {
        button_array[0] = m_button_pressed[0];
        button_array[1] = m_button_pressed[1];
        button_array[2] = m_button_pressed[2];
    }

    void GLBackend::getMouseButtonReleased(bool *button_array) {
        button_array[0] = m_button_released[0];
        button_array[1] = m_button_released[1];
        button_array[2] = m_button_released[2];
    }

    void GLBackend::getMouseButtonState(bool *button_array) {
        button_array[0] = m_button_state[0];
        button_array[1] = m_button_state[1];
        button_array[2] = m_button_state[2];
    }

    void GLBackend::getRelativeMousePosition(int *x, int *y) {
        SDL_GetRelativeMouseState(x, y);
    }

    void GLBackend::getMousePosition(int *x, int *y) {
        *x = m_mouse_pos.x;
        *y = m_mouse_pos.y;
    }

    void GLBackend::getPrevMousePosition(int *x, int *y) {
        *x = m_prev_mouse_pos.x;
        *y = m_prev_mouse_pos.y;
    }

    bool GLBackend::isMouseDragging() {
        return m_mouse_dragging;
    }

    void GLBackend::setCanvasMode(int m) {
        m_canvas_mode = m;
        if (m == CANVAS_SCALE_WINDOW)
            m_requested_canvas = glm::vec4(0.0f);
    }

    void GLBackend::setCanvasSize(float w, float h) {
        m_requested_canvas.z = w;
        m_requested_canvas.w = h;
    }


    void GLBackend::setFullscreen(bool fs) {
        if (fs) {
            SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else {
            SDL_SetWindowFullscreen(m_window, 0);
        }
        SDL_GetWindowSize(m_window, &m_width, &m_height);
        resize(m_width, m_height);
    }

    void GLBackend::playSound(std::string soundfile, float volume, bool looping) {
        m_audio->playSound(soundfile, volume, looping);
    }

    void GLBackend::playMusic(std::string soundfile, float volume, bool looping, int fade_time) {
        m_audio->playMusic(soundfile, volume, looping, fade_time);
    }

    void GLBackend::stopMusic(int fade_time) {
        m_audio->stopMusic(fade_time);
    }

    void GLBackend::terminate() {
        m_quit = true;
    }

    bool GLBackend::processEvent(SDL_Event event) {
        if (m_quit) return false;

        if (event.type == SDL_MOUSEMOTION) {
            m_prev_mouse_pos = m_mouse_pos;
            m_mouse_pos.x = event.motion.x;
            m_mouse_pos.y = event.motion.y;
            m_mouse_dragging = (m_button_state[0] || m_button_state[1] || m_button_state[2]);
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            bool new_state = (event.type == SDL_MOUSEBUTTONDOWN);
            switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    m_button_state[0] = new_state;
                    break;
                case SDL_BUTTON_MIDDLE:
                    m_button_state[1] = new_state;
                    break;
                case SDL_BUTTON_RIGHT:
                    m_button_state[2] = new_state;
                    break;
            }
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                if (SDL_GetRelativeMouseMode()) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                } else {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                }
            }
        } else if (event.type == SDL_WINDOWEVENT) {
            if (event.window.windowID == m_windowID &&
                (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                 event.window.event == SDL_WINDOWEVENT_MAXIMIZED)) {
                if (event.window.data1 != 0 && event.window.data2 != 0) {
                    resize(event.window.data1, event.window.data2);
                }
            }
        }

        return true;
    }


    void GLBackend::computeProjection() {
        float w, h;
        float n = -1.0f, f = 1.0f;
        float true_aspect = m_width / (float) m_height;
        float req_aspect = m_requested_canvas.z / m_requested_canvas.w;

        switch (m_canvas_mode) {
            case CANVAS_SCALE_STRETCH:
                m_canvas = m_requested_canvas;
                m_projection = glm::scale(glm::vec3(1, -1, 1)) * glm::ortho(0.0f, m_canvas.z, 0.0f, m_canvas.w, n, f);
                break;
            case CANVAS_SCALE_FIT:

                if (true_aspect > req_aspect) {
                    m_canvas.z = (true_aspect / req_aspect) * m_requested_canvas.z;
                    m_canvas.w = m_requested_canvas.w;
                    m_canvas.x = -m_requested_canvas.z * (true_aspect / req_aspect - 1.0f) / 2;
                    m_canvas.y = 0.0f;
                    m_canvas.z += m_canvas.x;
                } else {
                    m_canvas.z = m_requested_canvas.z;
                    m_canvas.w = (req_aspect / true_aspect) * m_requested_canvas.w;
                    m_canvas.y = 0.0f;
                    m_canvas.x = 0.0f;
                    m_canvas.y = -m_requested_canvas.w * (req_aspect / true_aspect - 1.0f) / 2;
                    m_canvas.w += m_canvas.y;
                }
                m_projection = glm::scale(glm::vec3(1, -1, 1)) * glm::ortho(
                                   m_canvas.x, m_canvas.z, m_canvas.y, m_canvas.w, n, f);

                break;
            default:
                m_canvas = glm::vec4(0.0f, 0.0f, (float) m_width, (float) m_height);
                m_projection = glm::scale(glm::vec3(1, -1, 1)) * glm::ortho(0.0f, m_canvas.z, 0.0f, m_canvas.w, n, f);
        }
    }

    void GLBackend::initPrimitives() {
        m_flat_shader = Shader(
            __PrimitivesVertexShader,
            __SolidFragmentShader
        );
        m_flat_shader.use(false);

        GLfloat box[4][4] = {
            {-0.5f, 0.5f, 0, 1},
            {0.5f, 0.5f, 1, 1},
            {-0.5f, -0.5f, 0, 0},
            {0.5f, -0.5f, 1, 0}
        };

        GLfloat box_outline[4][4] = {
            {-0.5f, 0.5f, 0, 0},
            {0.5f, 0.5f, 1, 0},
            {0.5f, -0.5f, 1, 1},
            {-0.5f, -0.5f, 0, 1}
        };

        GLfloat line[2][4] =
        {
            {0, 0, 0, 1},
            {1, 1, 1, 1}
        };

        float sector_vertices[2 * CURVE_SUBDIVS][4];
        float sector_outline_vertices[2 * CURVE_SUBDIVS][4];
        float r1 = 0.0f, r2 = 1.0f;
        for (int i = 0; i < CURVE_SUBDIVS; i++) {
            float s = i / (float) CURVE_SUBDIVS;
            sector_vertices[i * 2 + 0][0] = r1 * sin(3.1415936f * s);
            sector_vertices[i * 2 + 0][1] = r1 * cos(3.1415936f * s);
            sector_vertices[i * 2 + 0][2] = s;
            sector_vertices[i * 2 + 0][3] = 0.0f;
            sector_vertices[i * 2 + 1][0] = r2 * sin(3.1415936f * s);
            sector_vertices[i * 2 + 1][1] = r2 * cos(3.1415936f * s);
            sector_vertices[i * 2 + 1][2] = s;
            sector_vertices[i * 2 + 1][3] = 1.0f;
        }
        for (int i = 0; i < CURVE_SUBDIVS; i++) {
            float s = i / (float) CURVE_SUBDIVS;
            sector_vertices[i][0] = r1 * sin(3.1415936f * s);
            sector_vertices[i][1] = r1 * cos(3.1415936f * s);
            sector_vertices[i][2] = s;
            sector_vertices[i][3] = 0.0f;
            sector_vertices[2 * CURVE_SUBDIVS - i - 1][0] = r2 * sin(3.1415936f * s);
            sector_vertices[2 * CURVE_SUBDIVS - i - 1][1] = r2 * cos(3.1415936f * s);
            sector_vertices[2 * CURVE_SUBDIVS - i - 1][2] = s;
            sector_vertices[2 * CURVE_SUBDIVS - i - 1][3] = 1.0f;
        }

        sggGenVertexArrays(1, &m_sector_vao);
        sggBindVertexArray(m_sector_vao);
        glGenBuffers(1, &m_sector_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_sector_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof sector_vertices, sector_vertices, GL_DYNAMIC_DRAW);

        sggGenVertexArrays(1, &m_sector_outline_vao);
        sggBindVertexArray(m_sector_outline_vao);
        glGenBuffers(1, &m_sector_outline_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_sector_outline_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof sector_outline_vertices, sector_outline_vertices, GL_DYNAMIC_DRAW);

        sggGenVertexArrays(1, &m_line_vao);
        sggBindVertexArray(m_line_vao);
        glGenBuffers(1, &m_line_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof line, line, GL_DYNAMIC_DRAW);

        sggGenVertexArrays(1, &m_rect_vao);
        sggBindVertexArray(m_rect_vao);
        glGenBuffers(1, &m_rect_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_rect_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_STATIC_DRAW);

        sggGenVertexArrays(1, &m_rect_outline_vao);
        sggBindVertexArray(m_rect_outline_vao);
        glGenBuffers(1, &m_rect_outline_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_rect_outline_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof box_outline, box_outline, GL_STATIC_DRAW);

        unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");
        glEnableVertexAttribArray(attrib_flat_position);
        glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, 0);
    }

    void GLBackend::computeTransformation() {
        // note: rotation is CW, due to mirroring after projection.
        m_transformation = glm::rotate(-3.1415936f * m_orientation / 180.0f, glm::vec3(0.f, 0.f, 1.f)) *
                           glm::scale(m_scale);
    }

    void GLBackend::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Brush &brush) {
        static glm::vec4 default_color(0.0f, 0.0f, 0.0f, 0.0f); // Reuse a default color
        static glm::mat4 identity_matrix(1.0f); // Avoid recalculating identity matrix
        static bool texture_enabled = false; // Track texture state to avoid redundant enables

        // Only enable texture if it's not already enabled
        if (!texture_enabled) {
            glEnable(GL_TEXTURE_2D);
            texture_enabled = true;
        }
        glFrontFace(GL_CCW);
        glPolygonMode(GL_FRONT, GL_FILL);

        // Vertex data for the triangle
        GLfloat triangle[3][4] = {
            {x1, y1, 0.0f, 1.0f},
            {x2, y2, 0.0f, 1.0f},
            {x3, y3, 0.0f, 1.0f}
        };

        // Fill logic
        if (brush.fill_opacity > 0.0f || brush.fill_secondary_opacity > 0.0f) {
            Texture *texture = nullptr;
            bool has_texture = false;

            if (!brush.texture.empty()) {
                texture = textureManager->getTexture(brush.texture);
                if (texture) {
                    textureManager->bindTexture(texture, 30);
                    has_texture = true;
                }
            }
            m_flat_shader["has_texture"] = has_texture ? 1 : 0;

            // Set shader uniforms
            m_flat_shader["gradient"] = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);
            m_flat_shader["color1"] = glm::vec4(brush.fill_color[0], brush.fill_color[1], brush.fill_color[2],
                                                brush.fill_opacity);
            m_flat_shader["color2"] = brush.gradient
                                          ? glm::vec4(brush.fill_secondary_color[0], brush.fill_secondary_color[1],
                                                      brush.fill_secondary_color[2], brush.fill_secondary_opacity)
                                          : glm::vec4(brush.fill_color[0], brush.fill_color[1], brush.fill_color[2],
                                                      brush.fill_opacity);

            m_flat_shader["MV"] = identity_matrix;
            m_flat_shader["tex"] = 30; // Texture slot

            // Bind and draw the triangle
            unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");
            sggBindVertexArray(m_triangle_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_triangle_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STREAM_DRAW);
            glEnableVertexAttribArray(attrib_flat_position);
            glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

            // Unbind texture if used
            if (texture) {
                textureManager->unbindTexture(30);
            }
        }

        // Outline logic
        if (brush.outline_opacity > 0.0f) {
            glm::vec4 outline_color = glm::vec4(brush.outline_color[0], brush.outline_color[1],
                                                brush.outline_color[2], brush.outline_opacity);
            m_flat_shader["color1"] = outline_color;
            m_flat_shader["color2"] = outline_color;
            m_flat_shader["MV"] = identity_matrix;
            m_flat_shader["has_texture"] = 0; // No texture for outline

            glLineWidth(brush.outline_width);
            sggBindVertexArray(m_triangle_outline_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_triangle_outline_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STREAM_DRAW);
            unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");
            glEnableVertexAttribArray(attrib_flat_position);
            glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
            glLineWidth(1);
        }
    }

    void GLBackend::drawRect(float cx, float cy, float w, float h, const Brush &brush) {
        static bool texture_enabled = false;
        if (!texture_enabled) {
            glEnable(GL_TEXTURE_2D);
            texture_enabled = true;
        }
        glFrontFace(GL_CCW);
        glPolygonMode(GL_FRONT, GL_FILL);

        glm::mat4 mat = glm::translate(glm::vec3(cx, cy, 0.0f)) *
                        m_transformation * glm::scale(glm::vec3(w, h, 1.0f));

        Texture *texture = nullptr;

        // Fill logic
        if (brush.fill_opacity > 0.0f || brush.fill_secondary_opacity > 0.0f) {
            bool has_texture = false;
            if (!brush.texture.empty()) {
                texture = textureManager->createTexture(brush.texture, true, nullptr);
                if (texture) {
                    // Ensure the texture was successfully created
                    textureManager->bindTexture(texture, 30);
                    has_texture = true;
                } else {
                    std::cerr << "Warning: Failed to create texture: " << brush.texture << std::endl;
                }
            }
            m_flat_shader["has_texture"] = has_texture ? 1 : 0;
            m_flat_shader["gradient"] = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);
            m_flat_shader["color1"] = glm::vec4(brush.fill_color[0], brush.fill_color[1], brush.fill_color[2],
                                                brush.fill_opacity);
            m_flat_shader["color2"] = brush.gradient
                                          ? glm::vec4(brush.fill_secondary_color[0], brush.fill_secondary_color[1],
                                                      brush.fill_secondary_color[2], brush.fill_secondary_opacity)
                                          : glm::vec4(brush.fill_color[0], brush.fill_color[1], brush.fill_color[2],
                                                      brush.fill_opacity);

            m_flat_shader["MV"] = mat;
            m_flat_shader["tex"] = 30;

            unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");
            sggBindVertexArray(m_rect_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_rect_vbo);
            glEnableVertexAttribArray(attrib_flat_position);
            glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (texture) {
                textureManager->unbindTexture(30);
            }
        }

        // Outline logic
        if (brush.outline_opacity > 0.0f) {
            glm::vec4 outline_color = glm::vec4(brush.outline_color[0], brush.outline_color[1],
                                                brush.outline_color[2], brush.outline_opacity);
            m_flat_shader["color1"] = outline_color;
            m_flat_shader["color2"] = outline_color;
            m_flat_shader["MV"] = mat;
            m_flat_shader["has_texture"] = 0; // No texture for outline

            glLineWidth(brush.outline_width);
            sggBindVertexArray(m_rect_outline_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_rect_outline_vbo);
            unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");
            glEnableVertexAttribArray(attrib_flat_position);
            glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            glLineWidth(1);
        }
    }


    void GLBackend::drawLine(float x_1, float y_1, float x_2, float y_2, const Brush &brush) {
        // Set the color for the line using the brush's outline color and opacity
        m_flat_shader["color1"] = glm::vec4(brush.outline_color[0], brush.outline_color[1], brush.outline_color[2],
                                            brush.outline_opacity);

        // Apply an identity matrix (no transformations applied, just a simple 1:1 mapping)
        m_flat_shader["MV"] = glm::mat4(1.0f);

        // Set the gradient direction to use in the shader (not affecting much here since it's just a line)
        m_flat_shader["gradient"] = glm::vec2(1.0f, 0.0f);

        // Define the two points of the line with their respective coordinates
        // Each vertex has 4 components: x, y, z (depth), and w (for homogeneous coordinates)
        GLfloat line[2][4] = {
            {x_1, y_1, 0.0f, 1.0f}, // First point (x_1, y_1)
            {x_2, y_2, 0.1f, 1.0f} // Second point (x_2, y_2), with a slight difference in z for depth
        };

        // Bind the Vertex Array Object (VAO) which stores the configuration for the line's vertex attributes
        sggBindVertexArray(m_line_vao);

        // Bind the Vertex Buffer Object (VBO) to store the line vertex data in GPU memory
        glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);

        // Upload the line vertex data to the GPU, marking it as dynamic (GL_DYNAMIC_DRAW) for potentially frequent updates
        glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_DYNAMIC_DRAW);

        // Get the shader attribute location for the vertex position ("coord") from the shader program
        unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");

        // Enable the vertex attribute array for position to allow the shader to use the coordinates data
        glEnableVertexAttribArray(attrib_flat_position);

        // Specify how the position data is organized in the buffer:
        // 4 components per vertex (x, y, z, w), each of type float, with no padding between them
        glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

        // Draw the line using the vertex data, as a line connecting the two points
        glDrawArrays(GL_LINES, 0, 2);
    }


    std::vector<std::string> GLBackend::preloadBitmaps(std::string dir) {
        std::vector<std::string> names;
        Brush brush;
        fs::directory_iterator dir_iter;
        for (auto &entry: fs::directory_iterator(dir)) {
            std::string filename = entry.path().string();
            // std::cout << filename ;
            // TODO: for now, using MSVC17-compliant code, where
            // filesystem classes are not fully implemented.
            std::string extension = filename.substr(filename.length() - 4, 4);
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (extension.compare(".png")) {
                continue;
            }

            if (textureManager->getTexture(filename)) {
                names.push_back(filename);
            }
        }
        return names;
    }

    void GLBackend::drawSector(float cx, float cy, float start_angle, float end_angle, float radius1, float radius2,
                               const Brush &brush) {
        static bool texture_enabled = false; // Track texture state

        // Enable texture only if it's not already enabled
        if (!texture_enabled) {
            glEnable(GL_TEXTURE_2D);
            texture_enabled = true;
        }

        glFrontFace(GL_CCW);
        glPolygonMode(GL_FRONT, GL_FILL);

        // Set up transformation matrix and gradient
        m_flat_shader["MV"] = glm::translate(glm::vec3(cx, cy, 0.0f)) * m_transformation;
        m_flat_shader["gradient"] = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);

        float sector_vertices[2 * CURVE_SUBDIVS + 2][4];
        float sector_outline_vertices[2 * CURVE_SUBDIVS + 2][4];
        float r1 = radius1, r2 = radius2;
        float arc_inc = 3.1415936f * (end_angle - start_angle) / (180.0f * CURVE_SUBDIVS);

        // Populate sector vertices
        for (int i = 0; i <= CURVE_SUBDIVS; i++) {
            float s = i / (float) CURVE_SUBDIVS;
            sector_vertices[i * 2 + 0][0] = r1 * cos(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_vertices[i * 2 + 0][1] = r1 * -sin(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_vertices[i * 2 + 0][2] = s;
            sector_vertices[i * 2 + 0][3] = 0.0f;
            sector_vertices[i * 2 + 1][0] = r2 * cos(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_vertices[i * 2 + 1][1] = r2 * -sin(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_vertices[i * 2 + 1][2] = s;
            sector_vertices[i * 2 + 1][3] = 1.0f;
        }

        // Populate outline vertices
        for (int i = 0; i <= CURVE_SUBDIVS; i++) {
            float s = i / (float) CURVE_SUBDIVS;
            sector_outline_vertices[i][0] = r1 * cos(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_outline_vertices[i][1] = r1 * -sin(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_outline_vertices[i][2] = s;
            sector_outline_vertices[i][3] = 0.0f;
        }
        for (int i = CURVE_SUBDIVS; i >= 0; i--) {
            float s = i / (float) CURVE_SUBDIVS;
            sector_outline_vertices[2 * CURVE_SUBDIVS - i + 1][0] =
                    r2 * cos(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_outline_vertices[2 * CURVE_SUBDIVS - i + 1][1] =
                    r2 * -sin(3.1415936f * start_angle / 180.f + i * arc_inc);
            sector_outline_vertices[2 * CURVE_SUBDIVS - i + 1][2] = s;
            sector_outline_vertices[2 * CURVE_SUBDIVS - i + 1][3] = 1.0f;
        }

        Texture *texture = nullptr;

        // Fill
        if (brush.fill_opacity != 0.0f || brush.fill_secondary_opacity != 0.0f) {
            bool has_texture = false;

            // Check and bind texture if available
            if (!brush.texture.empty()) {
                texture = textureManager->getTexture(brush.texture);
                if (texture) {
                    textureManager->bindTexture(texture, 30);
                    has_texture = true;
                }
            }

            m_flat_shader["has_texture"] = has_texture ? 1 : 0;
            m_flat_shader["color1"] = glm::vec4(brush.fill_color[0], brush.fill_color[1], brush.fill_color[2],
                                                brush.fill_opacity);
            m_flat_shader["color2"] = brush.gradient
                                          ? glm::vec4(brush.fill_secondary_color[0], brush.fill_secondary_color[1],
                                                      brush.fill_secondary_color[2], brush.fill_secondary_opacity)
                                          : glm::vec4(brush.fill_color[0], brush.fill_color[1], brush.fill_color[2],
                                                      brush.fill_opacity);
            m_flat_shader["tex"] = 30;

            sggBindVertexArray(m_sector_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_sector_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof sector_vertices, sector_vertices, GL_DYNAMIC_DRAW);

            unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");
            glEnableVertexAttribArray(attrib_flat_position);
            glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, 0);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * CURVE_SUBDIVS + 2);

            if (texture) {
                textureManager->unbindTexture(30);
            }
        }

        // Outline
        if (brush.outline_opacity > 0.0f) {
            glm::vec4 outline_color = glm::vec4(brush.outline_color[0], brush.outline_color[1],
                                                brush.outline_color[2], brush.outline_opacity);
            m_flat_shader["color1"] = outline_color;
            m_flat_shader["color2"] = outline_color;
            m_flat_shader["has_texture"] = 0; // No texture for outline
            glLineWidth(brush.outline_width);

            sggBindVertexArray(m_sector_outline_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_sector_outline_vbo);

            glBufferData(GL_ARRAY_BUFFER, sizeof sector_outline_vertices, sector_outline_vertices, GL_DYNAMIC_DRAW
            );

            unsigned int attrib_flat_position = m_flat_shader.getAttributeLocation("coord");
            glEnableVertexAttribArray(attrib_flat_position);
            glVertexAttribPointer(attrib_flat_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

            if (fabs(end_angle - start_angle - 360.0f) > 0.000f)
                glDrawArrays(GL_LINE_LOOP, 0, 2 * CURVE_SUBDIVS + 2);
            else
                glDrawArrays(GL_LINE_LOOP, CURVE_SUBDIVS + 1, CURVE_SUBDIVS);

            glLineWidth(1);
        }
    }

    void GLBackend::drawText(float pos_x, float pos_y, float size, const std::string &text, const Brush &brush) {
        TextRecord entry;
        entry.text = text;
        entry.pos = glm::vec2(pos_x, pos_y);
        entry.size = glm::vec2(size, size);
        entry.color1 = glm::vec4(brush.fill_color[0], brush.fill_color[1], brush.fill_color[2], brush.fill_opacity);
        entry.color2 = glm::vec4(brush.fill_secondary_color[0], brush.fill_secondary_color[1],
                                 brush.fill_secondary_color[2], brush.fill_secondary_opacity);
        entry.gradient = glm::vec2(brush.gradient_dir_u, brush.gradient_dir_v);
        entry.use_gradient = brush.gradient;
        entry.mv = m_transformation;
        entry.proj = m_projection;

        m_fontlib.submitText(entry);
    }

    void GLBackend::setUserData(const void *user_data) {
        m_user_data = user_data;
    }

    void *GLBackend::getUserData() {
        return const_cast<void *>(m_user_data);
    }

    void GLBackend::setScale(float sx, float sy, float sz) {
        m_scale = glm::vec3(sx, sy, sz);
        computeTransformation();
    }

    void GLBackend::setOrientation(float degrees) {
        m_orientation = degrees;
        computeTransformation();
    }

    void GLBackend::resetPose() {
        m_transformation = glm::mat4(1.0f);
        m_scale = glm::vec3(1.0f);
        m_orientation = 0.0f;
    }

    bool GLBackend::setFont(std::string fontname) {
        return m_fontlib.setCurrentFont(fontname);
    }

    bool GLBackend::getKeyState(scancode_t key) {
        int numkeys;
        const uint8_t *keymap = SDL_GetKeyboardState(&numkeys);
        return keymap[key];
    }


    bool GLBackend::processMessages() {
        SDL_Event event;
        bool loop = true;
        while (SDL_PollEvent(&event) && loop) {
            if (event.type == SDL_WINDOWEVENT_CLOSE || event.type == SDL_QUIT) {
                return false;
            } else {
                loop = processEvent(event);
            }
        }
        update();
        if (m_idle_callback != nullptr) {
            m_idle_callback(getDeltaTime());
        }
        capFPS();
        advanceTime();
        draw();
        calculateFPS();

        return loop;
    }

    void GLBackend::setVSYNC(int input) {
        SDL_GL_SetSwapInterval(input);
    }

    float GLBackend::getFPS() const {
        return m_fps;
    }

    void GLBackend::setTargetFPS(int fps) {
        if (fps == -1) {
            m_target_fps = std::numeric_limits<int>::max(); // Uncap FPS
        } else {
            m_target_fps = fps; // Set target FPS
        }
    }

    void GLBackend::capFPS() {
        float target_frame_time = (m_target_fps == std::numeric_limits<int>::max()) ? 0 : (1000.0f / m_target_fps);

        Uint32 current_time = SDL_GetTicks();
        Uint32 frame_duration = current_time - m_frame_start_time;

        // Delay if we need to cap the FPS
        if (target_frame_time > 0 && frame_duration < target_frame_time) {
            SDL_Delay(static_cast<Uint32>(target_frame_time - frame_duration));
        }

        // Calculate delta time for the current frame
        m_delta_time = (SDL_GetTicks() - m_frame_start_time) / 1000.0f; // Convert to seconds
        m_frame_start_time = SDL_GetTicks(); // Update frame start time
    }

    void GLBackend::calculateFPS() {
        m_frame_count++;
        m_time_accumulator += m_delta_time;

        // Update FPS every second
        if (m_time_accumulator >= m_fps_update_interval) {
            m_fps = m_frame_count * (1000.0f / m_time_accumulator); // FPS calculation
            m_frame_count = 0;
            m_time_accumulator = 0.0f;
        }
    }

    void GLBackend::advanceTime() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed_seconds = now - m_prev_time_tick;
        m_delta_time = elapsed_seconds.count() * 1000.0f;
        m_global_time += m_delta_time;
        m_prev_time_tick = now;
    }

    void GLBackend::update(float delta_time) {
        static bool prev_mouse_state[3] = {0, 0, 0};

        m_button_pressed[0] = (m_button_state[0] && !prev_mouse_state[0]);
        m_button_pressed[1] = (m_button_state[1] && !prev_mouse_state[1]);
        m_button_pressed[2] = (m_button_state[2] && !prev_mouse_state[2]);

        m_button_released[0] = (!m_button_state[0] && prev_mouse_state[0]);
        m_button_released[1] = (!m_button_state[1] && prev_mouse_state[1]);
        m_button_released[2] = (!m_button_state[2] && prev_mouse_state[2]);

        prev_mouse_state[0] = m_button_state[0];
        prev_mouse_state[1] = m_button_state[1];
        prev_mouse_state[2] = m_button_state[2];
    }

    void GLBackend::resize(int w, int h) {
        m_width = w;
        m_height = h;

        if (m_resize_callback != nullptr)
            m_resize_callback(w, h);

        if (m_requested_canvas.z == 0 || m_requested_canvas.w == 0)
            // default canvas is window-sized, in pixel units.
            m_canvas = glm::vec4(0, 0, m_width, m_height);
        else
            m_canvas = m_requested_canvas;

        float c_w = m_canvas.z;
        float c_h = m_canvas.w;
        float cr = c_w / c_h;
        float wr = m_width / (float) m_height;

        // calculate window to canvas unit conversion ratios
        if (m_canvas_mode == CANVAS_SCALE_FIT) {
            if (cr > wr) {
                m_window_to_canvas_factors.x = c_w / m_width;
                m_window_to_canvas_factors.y = 0.0f;
                m_window_to_canvas_factors.z = c_w / m_width;
                m_window_to_canvas_factors.w = c_h / 2.0f - 0.5 * m_height * c_w / m_width;
            } else {
                m_window_to_canvas_factors.z = c_h / m_height;
                m_window_to_canvas_factors.w = 0.0f;
                m_window_to_canvas_factors.x = c_h / m_height;
                m_window_to_canvas_factors.y = c_w / 2.0f - 0.5 * m_width * c_h / m_height;
            }
        } else {
            m_window_to_canvas_factors.x = c_w / m_width;
            m_window_to_canvas_factors.y = 0.0f;
            m_window_to_canvas_factors.z = c_h / m_height;
            m_window_to_canvas_factors.w = 0.0f;
        }

        computeProjection();
        glViewport(0, 0, m_width, m_height);
        //SDL_Delay(5);
        draw();
    }

    float GLBackend::WindowToCanvasX(float x, bool clamped) {
        float coord = m_window_to_canvas_factors.x * x + m_window_to_canvas_factors.y;
        return clamped ? glm::clamp(coord, 0.0f, m_canvas.z) : coord;
    }

    float GLBackend::WindowToCanvasY(float y, bool clamped) {
        float coord = m_window_to_canvas_factors.z * y + m_window_to_canvas_factors.w;
        return clamped ? glm::clamp(coord, 0.0f, m_canvas.w) : coord;
    }

    void GLBackend::draw() {
        m_flat_shader.use(true);
        static bool first_time = true;
        if (first_time) {
            advanceTime();
            m_global_time = 0.0f;
            m_delta_time = 0.0f;
            first_time = false;
            resize(m_width, m_height);
            return;
        }

        resetPose();
        glDepthMask(0.0f);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.f, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_canvas_mode == CANVAS_SCALE_FIT) {
            float true_aspect = m_width / (float) m_height;
            float req_aspect = m_requested_canvas.z / m_requested_canvas.w;
            glEnable(GL_SCISSOR_TEST);
            glm::vec4 rect;
            rect.x = (true_aspect > req_aspect ? (m_width - m_height * req_aspect) / 2.0f : 0.0f);
            rect.y = (req_aspect > true_aspect ? (m_height - m_width / req_aspect) / 2.0f : 0.0f);
            rect.z = (true_aspect > req_aspect ? m_height * req_aspect : m_width);
            rect.w = (req_aspect > true_aspect ? m_width / req_aspect : m_height);
            glScissor(rect.x, rect.y, rect.z, rect.w);
        }

        Brush bck;
        bck.fill_color[0] = m_back_color.r, bck.fill_color[1] = m_back_color.g, bck.fill_color[2] = m_back_color.b;
        bck.outline_opacity = 0.0f;
        drawRect(m_requested_canvas.z / 2, m_requested_canvas.w / 2, m_requested_canvas.z, m_requested_canvas.w, bck);

        if (m_predraw_callback != nullptr) {
            m_predraw_callback();
            m_flat_shader.use();
            glEnable(GL_SCISSOR_TEST);
            glDepthMask(0.0f);
            glDisable(GL_DEPTH_TEST);
        }

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_flat_shader["P"] = m_projection;

        if (m_draw_callback != nullptr) {
            m_draw_callback();
        }

        m_fontlib.setCanvas(glm::vec2(m_requested_canvas.z, m_requested_canvas.w));
        m_fontlib.commitText();

        glDisable(GL_SCISSOR_TEST);

        if (m_postdraw_callback != nullptr) {
            m_postdraw_callback();
        }
        swap();
    }

    void GLBackend::setDrawCallback(std::function<void()> drf) {
        m_draw_callback = drf;
    }

    void GLBackend::setPreDrawCallback(std::function<void()> drf) {
        m_predraw_callback = drf;
    }

    void GLBackend::setPostDrawCallback(std::function<void()> drf) {
        m_postdraw_callback = drf;
    }

    void GLBackend::setIdleCallback(std::function<void(float)> idf) {
        m_idle_callback = idf;
    }

    void GLBackend::setResizeCallback(std::function<void(int, int)> rsf) {
        m_resize_callback = rsf;
    }

    void GLAPIENTRY GLBackend::debugMessageCallback(GLenum Source,
                                                    GLenum Type,
                                                    GLuint Id,
                                                    GLenum Severity,
                                                    GLsizei Length,
                                                    const GLchar *Message,
                                                    const void *UserParam) {
        static std::map<GLenum, const GLchar *> Sources =
        {
            {GL_DEBUG_SOURCE_API, "API"},
            {GL_DEBUG_SOURCE_WINDOW_SYSTEM, "WINDOW_SYSTEM"},
            {GL_DEBUG_SOURCE_SHADER_COMPILER, "SHADER_COMPILER"},
            {GL_DEBUG_SOURCE_THIRD_PARTY, "THIRD_PARTY"},
            {GL_DEBUG_SOURCE_APPLICATION, "APPLICATION"},
            {GL_DEBUG_SOURCE_OTHER, "OTHER"}
        };

        static std::map<GLenum, const GLchar *> Severities =
        {
            {GL_DEBUG_SEVERITY_HIGH, "HIGH"},
            {GL_DEBUG_SEVERITY_MEDIUM, "MEDIUM"},
            {GL_DEBUG_SEVERITY_LOW, "LOW"},
            {GL_DEBUG_SEVERITY_NOTIFICATION, "NOTIFICATION"}
        };

        static std::map<GLenum, const GLchar *> Types =
        {
            {GL_DEBUG_TYPE_ERROR, "ERROR"},
            {GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "DEPRECATED_BEHAVIOR"},
            {GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "UNDEFINED_BEHAVIOR"},
            {GL_DEBUG_TYPE_PORTABILITY, "PORTABILITY"},
            {GL_DEBUG_TYPE_PERFORMANCE, "PERFORMANCE"},
            {GL_DEBUG_TYPE_MARKER, "MARKER"},
            {GL_DEBUG_TYPE_PUSH_GROUP, "PUSH_GROUP"},
            {GL_DEBUG_TYPE_POP_GROUP, "POP_GROUP"},
            {GL_DEBUG_TYPE_OTHER, "OTHER"}
        };

        printf("[OpenGL %s] - SEVERITY: %s, SOURCE: %s, ID: %d: %s\n", Types[Type], Severities[Severity],
               Sources[Source], Id, Message);
    }

    bool GLBackend::setWindowName(const char *title) {
        if (!m_window || !title) {
            return false; // Return false if the window or title is invalid
        }
        m_title = title;
        SDL_SetWindowTitle(m_window, title); // Set the window title
        return true;
    }


    bool GLBackend::init() {
        m_initialized = false;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_AUDIO) < 0) {
            std::cout << "Failed to init SDL\n";
            return false;
        }

        //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_SetRelativeMouseMode(SDL_TRUE);


        m_window = SDL_CreateWindow(m_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    m_width, m_height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        m_windowID = SDL_GetWindowID(m_window);
        m_audio = new AudioManager();

        if (!m_window) {
            std::cout << "Unable to create window\n";
            CheckSDLError(__LINE__);
            return false;
        }

        m_context = SDL_GL_CreateContext(m_window);

        glewExperimental = GL_TRUE;
        glewInit();
        glGetError();
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glClearDepth(1.0f);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, nullptr);

        bool init = m_fontlib.init();
        if (!init) {
            std::cout << "Unable to initialize font library\n";
            CheckSDLError(__LINE__);
            return false;
        }

        initPrimitives();
        computeProjection();
        glViewport(0, 0, m_width, m_height);

        m_initialized = true;
        return true;
    }


    void CheckSDLError(int line = -1) {
        std::string error = SDL_GetError();

        if (error != "") {
            std::cout << "SLD Error : " << error << std::endl;

            if (line != -1)
                std::cout << "\nLine : " << line << std::endl;

            SDL_ClearError();
        }
    }

    void PrintSDL_GL_Attributes() {
        int value = 0;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &value);
        std::cout << "SDL_GL_CONTEXT_MAJOR_VERSION : " << value << std::endl;

        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &value);
        std::cout << "SDL_GL_CONTEXT_MINOR_VERSION: " << value << std::endl;
    }
}
