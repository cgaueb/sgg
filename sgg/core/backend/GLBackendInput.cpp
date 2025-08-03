#include "core/backend/GLBackend.h"
#include <SDL2/SDL.h>
#include "core/utils/headers/Scancodes.h"
#include <algorithm>

namespace graphics {
    bool GLBackend::processEvent(SDL_Event event) {
        if (m_quit) return false;

        switch (event.type) {
            case SDL_MOUSEMOTION:
                m_prev_mouse_pos = m_mouse_pos;
                m_mouse_pos.x = event.motion.x;
                m_mouse_pos.y = event.motion.y;
                m_mouse_dragging = (m_button_state[0] | m_button_state[1] | m_button_state[2]);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                const bool new_state = (event.type == SDL_MOUSEBUTTONDOWN);
                const uint8_t button = event.button.button;

                // Bounds check with single comparison
                if (button >= SDL_BUTTON_LEFT && button <= SDL_BUTTON_RIGHT) {
                    const int button_index = button - SDL_BUTTON_LEFT;
                    m_button_state[button_index] = new_state;
                }
                break;
            }

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    const SDL_bool current_mode = SDL_GetRelativeMouseMode();
                    SDL_SetRelativeMouseMode(current_mode ? SDL_FALSE : SDL_TRUE);
                }
                break;

            case SDL_WINDOWEVENT: {
                const SDL_WindowEvent& win_event = event.window;
                if (win_event.windowID == m_windowID) {
                    switch (win_event.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        case SDL_WINDOWEVENT_MAXIMIZED:
                            if (win_event.data1 > 0 && win_event.data2 > 0) {
                                resize(win_event.data1, win_event.data2);
                            }
                            break;
                    }
                }
                break;
            }
        }

        return true;
    }

    bool GLBackend::processMessages() {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            // Check for quit events first for early exit
            if (event.type == SDL_QUIT ||
                (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) {
                return false;
            }

            if (!processEvent(event)) {
                return false;
            }
        }

        update();

        // Start frame timing for performance monitoring
        perfMon.startFrame();

        if (m_idle_callback) {
            m_idle_callback(getDeltaTime());
        }

        draw();

        return true;
    }

    void GLBackend::update(float delta_time) {
        thread_local bool prev_mouse_state[3] = {false, false, false};

        // Process all three buttons in a single loop
        for (int i = 0; i < 3; ++i) {
            const bool current_state = m_button_state[i];
            const bool previous_state = prev_mouse_state[i];

            m_button_pressed[i] = current_state && !previous_state;
            m_button_released[i] = !current_state && previous_state;
            prev_mouse_state[i] = current_state;
        }
    }

    float GLBackend::getGlobalTime() const noexcept {
        const auto stats = perfMon.getFrameStats();
        return static_cast<float>(stats.totalTime);
    }

    // Return the frame time measured *after* the previous frame finished rendering and
    // any frame-rate limiting delay was applied. This guarantees that animations use
    // the exact duration between visible frames, giving identical speeds on all
    // platforms (Windows, Linux, with/without FPS cap).
    float GLBackend::getDeltaTime() const noexcept {
        const int fps_cap = perfMon.getFPSLimit();
        if (fps_cap > 0) {
            return static_cast<float>(1000.0 / fps_cap);
        }
        return static_cast<float>(perfMon.getFrameTimeMs());
    }

    void GLBackend::getMouseButtonPressed(bool *button_array) {
        // Use memcpy for efficient bulk copy or std::copy
        std::copy(std::begin(m_button_pressed), std::end(m_button_pressed), button_array);
    }

    void GLBackend::getMouseButtonReleased(bool *button_array) {
        std::copy(std::begin(m_button_released), std::end(m_button_released), button_array);
    }

    void GLBackend::getMouseButtonState(bool *button_array) {
        std::copy(std::begin(m_button_state), std::end(m_button_state), button_array);
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

    bool GLBackend::isMouseDragging() const noexcept {
        return m_mouse_dragging;
    }

    bool GLBackend::getKeyState(Scancode key) noexcept {
        // Early return for invalid keys
        if (!ScancodeMaps::isValid(key)) {
            return false;
        }

        int numKeys;
        const uint8_t *keymap = SDL_GetKeyboardState(&numKeys);
        const int keyIndex = static_cast<int>(key);

        // Combined bounds check
        if (!keymap || keyIndex >= numKeys || keyIndex < 0) {
            return false;
        }

        return keymap[keyIndex] != 0;
    }

    void GLBackend::setDrawCallback(std::function<void()> drf) {
        m_draw_callback = std::move(drf);
    }

    void GLBackend::setPreDrawCallback(std::function<void()> drf) {
        m_predraw_callback = std::move(drf);
    }

    void GLBackend::setPostDrawCallback(std::function<void()> drf) {
        m_postdraw_callback = std::move(drf);
    }

    void GLBackend::setIdleCallback(std::function<void(float)> idf) {
        m_idle_callback = std::move(idf);
    }

    void GLBackend::setResizeCallback(std::function<void(int, int)> rsf) {
        m_resize_callback = std::move(rsf);
    }
}