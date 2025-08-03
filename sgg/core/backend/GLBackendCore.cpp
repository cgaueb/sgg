#include "core/backend/GLBackend.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <iostream>
#include "core/audio/headers/AudioManager.h"
#include "core/fonts/headers/Fonts.h"
#include <string_view>
#include <array>

namespace graphics {
    GLBackend *GLBackend::s_current_instance = nullptr;

    GLBackend::GLBackend(int w, int h, std::string title)
        : m_width(w),
          m_height(h),
          m_title(std::move(title)),
          textureManager(&TextureManager::getInstance()) {
        s_current_instance = this;
    }

    GLBackend::~GLBackend() = default;

    bool GLBackend::init() {
        m_initialized = false;

        // Initialize SDL with required subsystems
        constexpr Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_AUDIO;
        if (SDL_Init(sdl_flags) < 0) {
            std::cout << "Failed to init SDL: " << SDL_GetError() << '\n';
            return false;
        }

        // Set OpenGL attributes
        const std::array<std::pair<SDL_GLattr, int>, 6> gl_attributes = {{
            {SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE},
            {SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG},
            {SDL_GL_DOUBLEBUFFER, 1},
            {SDL_GL_ALPHA_SIZE, 8},
            {SDL_GL_DEPTH_SIZE, 24},
            {SDL_GL_STENCIL_SIZE, 8}
        }};

        for (const auto& [attr, value] : gl_attributes) {
            SDL_GL_SetAttribute(attr, value);
        }

        SDL_SetRelativeMouseMode(SDL_TRUE);

        // Create window
        constexpr Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
        m_window = SDL_CreateWindow(m_title.c_str(),
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   m_width, m_height, window_flags);

        if (!m_window) {
            std::cout << "Unable to create window: " << SDL_GetError() << '\n';
            return false;
        }

        m_windowID = SDL_GetWindowID(m_window);

        // Initialize audio manager
        m_audio = new(std::nothrow) AudioManager();
        if (!m_audio) {
            std::cout << "Failed to allocate AudioManager\n";
            return false;
        }

        // Try creating OpenGL context with fallback versions
        const std::array<std::pair<int, int>, 3> gl_versions = {{
            {4, 3}, {4, 0}, {3, 0}
        }};

        for (const auto& [major, minor] : gl_versions) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);

            m_context = SDL_GL_CreateContext(m_window);
            if (m_context) {
                break;
            }

            std::cout << "OpenGL " << major << '.' << minor
                     << " context creation failed. Trying fallback...\n";
        }

        if (!m_context) {
            std::cout << "Failed to create any OpenGL context: " << SDL_GetError() << '\n';
            return false;
        }

        // Initialize GLEW
        glewExperimental = GL_TRUE;
        if (const GLenum glew_error = glewInit(); glew_error != GLEW_OK) {
            std::cout << "GLEW initialization failed: " << glewGetErrorString(glew_error) << '\n';
            return false;
        }

        // Clear any OpenGL errors from GLEW initialization
        glGetError();

        // Set OpenGL state
        constexpr std::array<GLenum, 3> enable_caps = {
            GL_DEPTH_TEST, GL_DEBUG_OUTPUT, GL_DEBUG_OUTPUT_SYNCHRONOUS
        };

        for (GLenum cap : enable_caps) {
            glEnable(cap);
        }

        glDepthFunc(GL_LEQUAL);
        glClearDepth(1.0f);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDebugMessageCallback(debugMessageCallback, nullptr);

        GLEW_INIT = true;

        // Initialize font library
        if (!m_fontlib.init()) {
            std::cout << "Unable to initialize font library\n";
            return false;
        }

        makeCurrent();
        initPrimitives();
        computeProjection();
        glViewport(0, 0, m_width, m_height);

        m_initialized = true;
        return true;
    }

    void GLBackend::cleanup() {
        if (m_context) {
            SDL_GL_DeleteContext(m_context);
            m_context = nullptr;
        }

        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        delete m_audio;
        m_audio = nullptr;

        SDL_Quit();
    }

    void GLBackend::swap() {
        SDL_GL_SwapWindow(m_window);
    }

    void GLBackend::makeCurrent() {
        SDL_GL_MakeCurrent(m_window, m_context);
    }

    void GLBackend::show(bool s) {
        if (s) {
            SDL_ShowWindow(m_window);
        } else {
            SDL_HideWindow(m_window);
        }
    }

    bool GLBackend::setWindowName(const char *title) {
        if (!m_window || !title) {
            return false;
        }
        m_title = title;
        SDL_SetWindowTitle(m_window, title);
        return true;
    }

    void GLBackend::setBackgroundColor(float r, float g, float b) {
        m_back_color = glm::vec3(r, g, b);
    }

    void GLBackend::setFullscreen(bool fs) {
        const Uint32 fullscreen_flag = fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        SDL_SetWindowFullscreen(m_window, fullscreen_flag);
        SDL_GetWindowSize(m_window, &m_width, &m_height);
        resize(m_width, m_height);
    }

    void GLBackend::playSound(std::string soundfile, float volume, bool looping) {
        if (m_audio) {
            m_audio->playSound(std::move(soundfile), volume, looping);
        }
    }

    void GLBackend::playMusic(std::string soundfile, float volume, bool looping, int fade_time) {
        if (m_audio) {
            m_audio->playMusic(std::move(soundfile), volume, looping, fade_time);
        }
    }

    void GLBackend::stopMusic(int fade_time) {
        if (m_audio) {
            m_audio->stopMusic(fade_time);
        }
    }

    void GLBackend::terminate() {
        m_quit = true;
    }

    void CheckSDLError(int line) {
        const char* error = SDL_GetError();
        if (error && *error) {  // Check for non-empty string more efficiently
            std::cout << "SDL Error: " << error;
            if (line != -1) {
                std::cout << " (Line: " << line << ')';
            }
            std::cout << '\n';
            SDL_ClearError();
        }
    }

    void PrintSDL_GL_Attributes() {
        int major_version = 0, minor_version = 0;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major_version);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor_version);

        std::cout << "OpenGL Context Version: " << major_version << '.' << minor_version << '\n';
    }

    void GLAPIENTRY GLBackend::debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                                   GLsizei length, const GLchar *message, const void *userParam) {
        // Use constexpr arrays for better performance
        constexpr std::array<std::pair<GLenum, std::string_view>, 6> sources = {{
            {GL_DEBUG_SOURCE_API, "API"},
            {GL_DEBUG_SOURCE_WINDOW_SYSTEM, "WINDOW_SYSTEM"},
            {GL_DEBUG_SOURCE_SHADER_COMPILER, "SHADER_COMPILER"},
            {GL_DEBUG_SOURCE_THIRD_PARTY, "THIRD_PARTY"},
            {GL_DEBUG_SOURCE_APPLICATION, "APPLICATION"},
            {GL_DEBUG_SOURCE_OTHER, "OTHER"}
        }};

        constexpr std::array<std::pair<GLenum, std::string_view>, 4> severities = {{
            {GL_DEBUG_SEVERITY_HIGH, "HIGH"},
            {GL_DEBUG_SEVERITY_MEDIUM, "MEDIUM"},
            {GL_DEBUG_SEVERITY_LOW, "LOW"},
            {GL_DEBUG_SEVERITY_NOTIFICATION, "NOTIFICATION"}
        }};

        constexpr std::array<std::pair<GLenum, std::string_view>, 9> types = {{
            {GL_DEBUG_TYPE_ERROR, "ERROR"},
            {GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "DEPRECATED_BEHAVIOR"},
            {GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "UNDEFINED_BEHAVIOR"},
            {GL_DEBUG_TYPE_PORTABILITY, "PORTABILITY"},
            {GL_DEBUG_TYPE_PERFORMANCE, "PERFORMANCE"},
            {GL_DEBUG_TYPE_MARKER, "MARKER"},
            {GL_DEBUG_TYPE_PUSH_GROUP, "PUSH_GROUP"},
            {GL_DEBUG_TYPE_POP_GROUP, "POP_GROUP"},
            {GL_DEBUG_TYPE_OTHER, "OTHER"}
        }};

        // Helper lambda to find string representation
        auto find_string = [](const auto& container, GLenum value) -> std::string_view {
            for (const auto& [key, str] : container) {
                if (key == value) return str;
            }
            return "UNKNOWN";
        };

        // Skip low-priority notifications in release builds
        #ifndef DEBUG
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;
        }
        #endif

        printf("[OpenGL %.*s] - SEVERITY: %.*s, SOURCE: %.*s, ID: %u: %s\n",
               static_cast<int>(find_string(types, type).length()), find_string(types, type).data(),
               static_cast<int>(find_string(severities, severity).length()), find_string(severities, severity).data(),
               static_cast<int>(find_string(sources, source).length()), find_string(sources, source).data(),
               id, message);
    }
}