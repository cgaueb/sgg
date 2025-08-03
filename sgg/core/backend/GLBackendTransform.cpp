#include "core/backend/GLBackend.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace graphics {
    void GLBackend::computeProjection() {
        constexpr float n = -1.0f, f = 1.0f;
        const float true_aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        const float req_aspect = m_requested_canvas.z / m_requested_canvas.w;

        // Pre-compute flip matrix once
        static const glm::mat4 flip_matrix = glm::scale(glm::vec3(1.0f, -1.0f, 1.0f));

        switch (m_canvas_mode) {
            case CANVAS_SCALE_STRETCH:
                m_canvas = m_requested_canvas;
                m_projection = flip_matrix * glm::ortho(0.0f, m_canvas.z, 0.0f, m_canvas.w, n, f);
                break;

            case CANVAS_SCALE_FIT: {
                const bool width_constrained = true_aspect > req_aspect;

                if (width_constrained) {
                    const float aspect_ratio = true_aspect / req_aspect;
                    const float offset = -m_requested_canvas.z * (aspect_ratio - 1.0f) * 0.5f;

                    m_canvas.z = aspect_ratio * m_requested_canvas.z;
                    m_canvas.w = m_requested_canvas.w;
                    m_canvas.x = offset;
                    m_canvas.y = 0.0f;
                    m_canvas.z += m_canvas.x;
                } else {
                    const float aspect_ratio = req_aspect / true_aspect;
                    const float offset = -m_requested_canvas.w * (aspect_ratio - 1.0f) * 0.5f;

                    m_canvas.z = m_requested_canvas.z;
                    m_canvas.w = aspect_ratio * m_requested_canvas.w;
                    m_canvas.x = 0.0f;
                    m_canvas.y = offset;
                    m_canvas.w += m_canvas.y;
                }

                m_projection = flip_matrix * glm::ortho(m_canvas.x, m_canvas.z, m_canvas.y, m_canvas.w, n, f);
                break;
            }

            default: // CANVAS_SCALE_WINDOW
                m_canvas = glm::vec4(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
                m_projection = flip_matrix * glm::ortho(0.0f, m_canvas.z, 0.0f, m_canvas.w, n, f);
                break;
        }
    }

    void GLBackend::computeTransformation() {
        const float deg_to_rad = -glm::pi<float>() / 180.0f;

        m_transformation = glm::rotate(deg_to_rad * m_orientation, glm::vec3(0.0f, 0.0f, 1.0f)) *
                          glm::scale(m_scale);
    }

    void GLBackend::setCanvasMode(scale_mode_t m) {
        m_canvas_mode = m;
        if (m == CANVAS_SCALE_WINDOW) {
            m_requested_canvas = glm::vec4(0.0f);
        }
    }

    void GLBackend::setCanvasSize(float w, float h) {
        m_requested_canvas.z = w;
        m_requested_canvas.w = h;
    }

    void GLBackend::resize(int w, int h) {
        m_width = w;
        m_height = h;

        // Early callback execution
        if (m_resize_callback) {
            m_resize_callback(w, h);
        }

        // Determine canvas dimensions
        if (m_requested_canvas.z == 0.0f || m_requested_canvas.w == 0.0f) {
            m_canvas = glm::vec4(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
        } else {
            m_canvas = m_requested_canvas;
        }

        // Pre-compute common values
        const float c_w = m_canvas.z;
        const float c_h = m_canvas.w;
        const float canvas_aspect = c_w / c_h;
        const float window_aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        const bool width_constrained = canvas_aspect > window_aspect;

        // Compute window-to-canvas transformation factors
        if (m_canvas_mode == CANVAS_SCALE_FIT) {
            if (width_constrained) {
                const float scale_factor = c_w / static_cast<float>(m_width);
                m_window_to_canvas_factors = glm::vec4(
                    scale_factor,
                    0.0f,
                    scale_factor,
                    c_h * 0.5f - static_cast<float>(m_height) * scale_factor * 0.5f
                );
            } else {
                const float scale_factor = c_h / static_cast<float>(m_height);
                m_window_to_canvas_factors = glm::vec4(
                    scale_factor,
                    c_w * 0.5f - static_cast<float>(m_width) * scale_factor * 0.5f,
                    scale_factor,
                    0.0f
                );
            }
        } else {
            m_window_to_canvas_factors = glm::vec4(
                c_w / static_cast<float>(m_width),
                0.0f,
                c_h / static_cast<float>(m_height),
                0.0f
            );
        }

        computeProjection();
        computeUIProjection();
        glViewport(0, 0, m_width, m_height);
    }

    // ---------------------------------------------------------------------
    // Compute UI (top-left origin) projection matrix – keeps text right-side-up
    // ---------------------------------------------------------------------
    void GLBackend::computeUIProjection() {
        constexpr float n = -1.0f, f = 1.0f;

        static const glm::mat4 flipY = glm::scale(glm::vec3(1.0f, -1.0f, 1.0f));

        switch (m_canvas_mode) {
            case CANVAS_SCALE_STRETCH:
                m_ui_projection = glm::ortho(0.0f, m_canvas.z, m_canvas.w, 0.0f, n, f);
            break;

            case CANVAS_SCALE_FIT:
                m_ui_projection = glm::ortho(m_canvas.x, m_canvas.z, m_canvas.w, m_canvas.y, n, f);
            break;

            default: // WINDOW
                m_ui_projection = glm::ortho(0.0f,
                                             static_cast<float>(m_width),
                                             static_cast<float>(m_height),
                                             0.0f,
                                             n, f);
            break;
        }
    }

    float GLBackend::WindowToCanvasX(float x, bool clamped) {
        const float coord = m_window_to_canvas_factors.x * x + m_window_to_canvas_factors.y;
        return clamped ? glm::clamp(coord, 0.0f, m_canvas.z) : coord;
    }

    float GLBackend::WindowToCanvasY(float y, bool clamped) {
        const float coord = m_window_to_canvas_factors.z * y + m_window_to_canvas_factors.w;
        return clamped ? glm::clamp(coord, 0.0f, m_canvas.w) : coord;
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

    void GLBackend::translate(float dx, float dy) {
        m_transformation = glm::translate(m_transformation, glm::vec3(dx, dy, 0.0f));
    }

    void GLBackend::rotate(float angleDeg) {
        m_transformation = glm::rotate(m_transformation, glm::radians(angleDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    void GLBackend::scale(float sx, float sy) {
        m_transformation = glm::scale(m_transformation, glm::vec3(sx, sy, 1.0f));
    }

    bool GLBackend::setFont(std::string fontname) {
        return m_fontlib.setCurrentFont(fontname);
    }
}