#pragma once
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string>
#include <thread>
#include "Fonts.h"
#include <functional>
#include "Scancodes.h"
#include "AudioManager.h"
#include "TextureManager.h"


#define SGG_CHECK_GL() do {GLenum err;while((err = glGetError()) != GL_NO_ERROR){ printf("Error %s %d\n", (const char*)glewGetErrorString(err), err);exit(0);}printf("Pass\n");} while(0);

constexpr auto CURVE_SUBDIVS = 64;

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
        int m_canvas_mode = 0;

        glm::mat4 m_projection;
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

        std::chrono::time_point<std::chrono::steady_clock> m_prev_time_tick;
        float m_global_time = 0.0f;
        float m_delta_time = 0.0f;

        const void *m_user_data = nullptr;

        bool processEvent(SDL_Event event);

        void advanceTime();

        void computeProjection();

        void initPrimitives();

        void computeTransformation();

        std::function<void()> m_predraw_callback = nullptr;
        std::function<void()> m_postdraw_callback = nullptr;
        std::function<void()> m_draw_callback = nullptr;
        std::function<void(float ms)> m_idle_callback = nullptr;
        std::function<void(int w, int h)> m_resize_callback = nullptr;

        float m_fps = 0.0f;
        float m_target_fps = 60.0f;
        int m_frame_count = 0;
        float m_fps_update_interval = 1000.0f; // Update FPS every second (1000ms)
        float m_time_accumulator = 0.0f;
        Uint32 m_frame_start_time = 0;

    public:
        TextureManager *getTextureManager() const { return textureManager; }

        void calculateFPS();

        float getFPS() const;

        void setTargetFPS(int fps);

        void capFPS();

        static void setVSYNC(int input);

        bool setWindowName(const char *title);

        void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const struct Brush &brush);

        void drawRect(float cx, float cy, float w, float h, const struct Brush &brush);

        void drawLine(float x_1, float y_1, float x_2, float y_2, const struct Brush &brush);

        void drawSector(float cx, float cy, float start_angle, float end_angle, float radius1, float radius2,
                        const struct Brush &brush);

        void drawText(float pos_x, float pos_y, float size, const std::string &text, const Brush &brush);

        void setUserData(const void *user_data);

        void *getUserData();

        void setScale(float sx, float sy, float sz);

        void setOrientation(float degrees);

        void resetPose();

        bool setFont(std::string fontname);

        std::vector<std::string> preloadBitmaps(std::string dir);

        bool getKeyState(scancode_t key);

        void setDrawCallback(std::function<void()> drf);

        void setPreDrawCallback(std::function<void()> drf);

        void setPostDrawCallback(std::function<void()> drf);

        void setIdleCallback(std::function<void(float ms)> idf);

        void setResizeCallback(std::function<void(int w, int h)> rsf);

        static void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                                    GLsizei length, const GLchar *message, const void *userParam);

        virtual bool init();

        GLBackend(int w = 800, int h = 600, std::string title = "");

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

        float getDeltaTime();

        float getGlobalTime();

        void getMouseButtonPressed(bool *button_array);

        void getMouseButtonReleased(bool *button_array);

        void getMouseButtonState(bool *button_array);

        void getRelativeMousePosition(int *x, int *y);

        void getMousePosition(int *x, int *y);

        void getPrevMousePosition(int *x, int *y);

        bool isMouseDragging();

        void setCanvasMode(int m);

        void setCanvasSize(float w, float h);

        void setFullscreen(bool fs);

        void playSound(std::string soundfile, float volume, bool looping = false);

        void playMusic(std::string soundfile, float volume, bool looping = true, int fade_time = 0);

        void stopMusic(int fade_time = 0);

        void terminate();
    };
}
