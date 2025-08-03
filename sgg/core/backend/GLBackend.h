#pragma once
/**
 * @file GLBackend.h
 * @brief Main OpenGL backend for SGG rendering
 */

#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string>
#include <thread>
#include "core/fonts/headers/Fonts.h"
#include <functional>
#include "core/utils/headers/Scancodes.h"
#include "core/audio/headers/AudioManager.h"
#include "core/graphics/textures/headers/TextureManager.h"
#include "core/graphics/rendering/performance/headers/GLPerformanceMonitor.h"
#include <map>
#include <cstdint>
#include <array>
#include "api/headers/Graphics.h"

#define SGG_CHECK_GL() do {GLenum err;while((err = glGetError()) != GL_NO_ERROR){ printf("Error %s %d\n", (const char*)glewGetErrorString(err), err);exit(0);}printf("Pass\n");} while(0);

constexpr auto CURVE_SUBDIVS = 64;
inline auto GLEW_INIT = false;

//#undef main
namespace graphics {
    void PrintSDL_GL_Attributes();

    void CheckSDLError(int line);

    class GLBackend {
    protected:
        int m_width;
        int m_height;
        std::string m_title;
        SDL_Window *m_window;
        uint32_t m_windowID;
        SDL_GLContext m_context;
        bool m_initialized = false;
        FontLib m_fontlib;
        SDL_TimerID m_idle_timer;
        glm::vec3 m_back_color = {0.0f, 0.0f, 0.0f};
        bool m_quit = false;

        glm::ivec2 m_mouse_pos = glm::ivec2();
        glm::ivec2 m_prev_mouse_pos = glm::ivec2();
        bool m_button_state[3] = {0, 0, 0};
        bool m_button_pressed[3] = {0, 0, 0};
        bool m_button_released[3] = {0, 0, 0};
        bool m_mouse_dragging = false;

        glm::vec4 m_requested_canvas = glm::vec4(0.0f);
        glm::vec4 m_canvas;
        scale_mode_t m_canvas_mode = CANVAS_SCALE_WINDOW;

        glm::mat4 m_projection;
        // Separate projection matrix for UI / text rendering (top-left origin)
        glm::mat4 m_ui_projection;
        glm::mat4 m_transformation = glm::mat4(1.0f);
        float m_orientation = 0.0f;
        glm::vec3 m_scale = glm::vec3(1.0f);

        Shader m_flat_shader;

        unsigned int m_point_vbo;
        unsigned int m_point_vao;
        unsigned int m_triangle_vbo;
        unsigned int m_triangle_vao;
        unsigned int m_triangle_outline_vbo;
        unsigned int m_triangle_outline_vao;
        unsigned int m_rect_vbo;
        unsigned int m_rect_vao;
        unsigned int m_rect_outline_vbo;
        unsigned int m_rect_outline_vao;
        unsigned int m_line_vbo;
        unsigned int m_line_vao;
        unsigned int m_sector_vbo;
        unsigned int m_sector_vao;
        unsigned int m_sector_outline_vbo;
        unsigned int m_sector_outline_vao;

        glm::vec4 m_window_to_canvas_factors;

        AudioManager *m_audio = nullptr;

        TextureManager *textureManager;

        GLPerformanceMonitor perfMon;

        double actualFPS = 0.0;

        // CPU-side render/update time of the last frame (ms)
        double m_lastRenderTimeMs = 0.0;

        std::map<uint32_t, GLuint> m_user_buffers;
        uint32_t m_next_buffer_id = 1;

        const void *m_user_data = nullptr;

        bool processEvent(SDL_Event event);

        void computeProjection();
        void computeUIProjection();

        void initPrimitives();

        void computeTransformation();

        std::function<void()> m_predraw_callback = nullptr;
        std::function<void()> m_draw_callback = nullptr;
        std::function<void()> m_postdraw_callback = nullptr;
        std::function<void(float ms)> m_idle_callback = nullptr;
        std::function<void(int w, int h)> m_resize_callback = nullptr;

        static GLBackend *s_current_instance;
        bool s_vsync_enabled;

    public:
        [[nodiscard]] TextureManager *getTextureManager() const { return textureManager; }

        [[nodiscard]] Shader *getDefaultShader() noexcept { return &m_flat_shader; }

        // ---- GLPerformanceMonitor Integration ----
        void setTargetFPS(int fps) noexcept { perfMon.setLimitFPS(fps); }
        [[nodiscard]] double getFPS() const noexcept { return actualFPS > 0 ? actualFPS : perfMon.getFPS(); }
        [[nodiscard]] double getFrameTime() const noexcept { return perfMon.getFrameTimeMs(); }
        [[nodiscard]] double getInstantaneousFPS() const noexcept { return perfMon.getInstantaneousFPS(); }
        void setVSYNC(bool enabled) noexcept { s_vsync_enabled = enabled; }
        [[nodiscard]] bool isVSYNCEnabled() { return s_vsync_enabled; }

        // Pure CPU render/update cost of last frame (does not include waiting)
        [[nodiscard]] double getRenderTime() const noexcept { return m_lastRenderTimeMs; }

        [[nodiscard]] GLPerformanceMonitor::FrameStats getFrameStats() const noexcept {
            return perfMon.getFrameStats();
        }

        void resetPerformanceStats() noexcept { perfMon.reset(); }
        [[nodiscard]] GLPerformanceMonitor &getPerformanceMonitor() noexcept { return perfMon; }
        [[nodiscard]] const GLPerformanceMonitor &getPerformanceMonitor() const noexcept { return perfMon; }

        // ---- Static access for convenience ----
        static GLBackend *getCurrentInstance() noexcept { return s_current_instance; }

        [[nodiscard]] const glm::mat4 &getProjectionMatrix() const noexcept { return m_projection; }

        bool setWindowName(const char *title);

        // Draw triangle with optional Z coordinate (default wrapper uses z=0)
        void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const struct Brush &brush);

        void drawTriangle(float x1, float y1, float z1,
                          float x2, float y2, float z2,
                          float x3, float y3, float z3,
                          const struct Brush &brush);

        void drawRect(float cx, float cy, float w, float h, const struct Brush &brush);

        void drawRect(float cx, float cy, float cz, float w, float h, const struct Brush &brush);

        void drawLine(float x_1, float y_1, float x_2, float y_2, const struct Brush &brush);

        void drawLine(float x1, float y1, float z1,
                      float x2, float y2, float z2,
                      const struct Brush &brush);

        void drawSector(float cx, float cy, float cz,
                       float start_angle, float end_angle,
                       float radius1, float radius2,
                       const struct Brush &brush);
        // 2D wrapper (cz=0)
        void drawSector(float cx, float cy,
                       float start_angle, float end_angle,
                       float radius1, float radius2,
                       const struct Brush &brush);

        void drawText(float pos_x, float pos_y, float size, const std::string &text, const Brush &brush);

        void setUserData(const void *user_data);

        void *getUserData();

        void setScale(float sx, float sy, float sz);

        void setOrientation(float degrees);

        void resetPose();

        // --- Transform helpers
        void translate(float dx, float dy);

        void rotate(float angleDeg);

        void scale(float sx, float sy);

        bool setFont(std::string fontname);

        std::vector<std::string> preloadBitmaps(std::string dir);

        bool getKeyState(scancode_t key) noexcept;

        void setPreDrawCallback(std::function<void()> drf);

        void setDrawCallback(std::function<void()> drf);

        void setPostDrawCallback(std::function<void()> drf);

        void setIdleCallback(std::function<void(float ms)> idf);

        void setResizeCallback(std::function<void(int w, int h)> rsf);

        // --- Window size accessors ---
        int getWindowWidth() const { return m_width; }
        int getWindowHeight() const { return m_height; }

        // --- Canvas size accessors ---
        float getCanvasWidth() const { return m_canvas.z; }
        float getCanvasHeight() const { return m_canvas.w; }

        static void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                                    GLsizei length, const GLchar *message, const void *userParam);

        virtual bool init();

        explicit GLBackend(int w = 800, int h = 600, std::string title = "");

        virtual void cleanup();

        void swap();

        bool processMessages();

        virtual void update(float delta_time = 0.0f);

        virtual void resize(int w, int h);

        float WindowToCanvasX(float x, bool clamped = true);

        float WindowToCanvasY(float y, bool clamped = true);

        virtual void draw();

        virtual ~GLBackend();

        void makeCurrent();

        void show(bool s);

        void setBackgroundColor(float r, float g, float b);

        [[nodiscard]] float getDeltaTime() const noexcept;

        [[nodiscard]] float getGlobalTime() const noexcept;

        void getMouseButtonPressed(bool *button_array);

        void getMouseButtonReleased(bool *button_array);

        void getMouseButtonState(bool *button_array);

        void getRelativeMousePosition(int *x, int *y);

        void getMousePosition(int *x, int *y);

        void getPrevMousePosition(int *x, int *y);

        [[nodiscard]] bool isMouseDragging() const noexcept;

        void setCanvasMode(scale_mode_t m);

        void setCanvasSize(float w, float h);

        void setFullscreen(bool fs);

        void playSound(std::string soundfile, float volume, bool looping);

        void playMusic(std::string soundfile, float volume, bool looping, int fade_time = 500);

        void stopMusic(int fade_time = 500);

        void terminate();

        // Get access to the default shader instance
        [[nodiscard]] const Shader &getDefaultShader() const { return m_flat_shader; }

    private:
        void drawTriangleInternal(const std::array<std::array<float,4>,3>& triangle,
                                  const struct Brush &brush);
    };
}