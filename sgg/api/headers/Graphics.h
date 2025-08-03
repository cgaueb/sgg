#pragma once

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <string_view>
#include <glm/glm.hpp>
#include "core/utils/headers/Scancodes.h"
#include "core/graphics/textures/headers/Texture.h"
#include "core/graphics/textures/headers/TextureManager.h"
#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include "core/graphics/rendering/factories/headers/BufferFactory.h"
#include "core/graphics/rendering/factories/headers/VAOFactory.h"
#include "core/graphics/rendering/utils/headers/DrawCommands.h"
#include "core/graphics/shaders/headers/Shader.h"
#include "core/graphics/rendering/factories/headers/BatchRendererFactory.h"
#include "core/graphics/rendering/batching/headers/BatchRenderer.h"
#include "core/graphics/rendering/performance/headers/GLPerformanceMonitor.h"
#include "core/utils/headers/CompilerTraits.h"

/** @def LIKELY
 *  @brief Branch prediction hint for likely conditions (MSVC no-op).
 */
#ifdef _MSC_VER
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
    #define FORCE_INLINE __forceinline
    #define HOT_FUNCTION ATTR_GNU_HOT
    #define COLD_FUNCTION __declspec(noinline)
#else
    // GCC/Clang hints
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define FORCE_INLINE __attribute__((always_inline)) inline
    #define HOT_FUNCTION ATTR_GNU_HOT
    #define COLD_FUNCTION __attribute__((cold, noinline))
#endif

/** @file Graphics.h
 *  @brief Unified header containing both Simple Game Graphics and Advanced Graphics APIs.
 */

class Shader;

/** @namespace graphics
 *  @brief Contains all functions of the Simple Game Graphics library.
 */
namespace graphics {
    /** @enum ScaleMode
     *  @brief Canvas scaling modes for adapting to application window.
     */
    enum class ScaleMode : int {
        WINDOW = 0, /**< Canvas size matches window pixels. setCanvasSize is ignored. */
        STRETCH = 1, /**< Canvas stretches/compresses to fill the window. */
        FIT = 2 /**< Canvas scales to fit, preserving aspect ratio (letterboxing). */
    };

    /** @typedef scale_mode_t
     *  @brief Legacy typedef for backwards compatibility with ScaleMode.
     */
    using scale_mode_t = ScaleMode;
    constexpr ScaleMode CANVAS_SCALE_WINDOW = ScaleMode::WINDOW;
    constexpr ScaleMode CANVAS_SCALE_STRETCH = ScaleMode::STRETCH;
    constexpr ScaleMode CANVAS_SCALE_FIT = ScaleMode::FIT;

    /** @struct Brush
     *  @brief Drawing attributes for all supported primitives and draw calls, optimized for cache performance.
     */
    struct alignas(16) Brush {
        /** @brief Primary fill color {R, G, B}. Range [0.0, 1.0]. */
        float fill_color[3] = {1.0f, 1.0f, 1.0f};
        /** @brief Primary fill opacity. Range [0.0 (transparent), 1.0 (opaque)]. */
        float fill_opacity = 1.0f;

        /** @brief Outline stroke color {R, G, B}. */
        float outline_color[3] = {1.0f, 1.0f, 1.0f};
        /** @brief Outline opacity. Range [0.0, 1.0]. */
        float outline_opacity = 0.0f;
        /** @brief Outline stroke width in pixels (>= 0.0). */
        float outline_width = 1.0f;

        /** @brief Secondary fill color for gradients {R, G, B}. */
        float fill_secondary_color[3] = {1.0f, 1.0f, 1.0f};
        /** @brief Secondary fill opacity for gradients. */
        float fill_secondary_opacity = 0.0f;

        /** @brief U-component of gradient direction vector. */
        float gradient_dir_u = 0.0f;
        /** @brief V-component of gradient direction vector (default is vertical). */
        float gradient_dir_v = 1.0f;

        /** @brief Enable gradient fill (uses secondary color/opacity). */
        bool gradient = false;

        /** @brief Filename of a texture (PNG) to blend with the fill. Empty for no texture. */
        std::string texture = "";

        /** @brief Default constructor ensures proper initialization. */
        Brush() = default;

        // Copy and move operations for efficient parameter passing
        Brush(const Brush &) = default;

        Brush &operator=(const Brush &) = default;

        Brush(Brush &&) = default;

        Brush &operator=(Brush &&) = default;
    };

    /** @struct MouseState
     *  @brief Mouse/pointing device state information, optimized for cache performance.
     */
    struct alignas(8) MouseState {
        /** @brief Current cursor X position (window pixels). */
        int cur_pos_x = 0;
        /** @brief Current cursor Y position (window pixels). */
        int cur_pos_y = 0;
        /** @brief Previous cursor X position (window pixels). */
        int prev_pos_x = 0;
        /** @brief Previous cursor Y position (window pixels). */
        int prev_pos_y = 0;
        /** @brief Relative X motion since last frame (window pixels). */
        int rel_x = 0;
        /** @brief Relative Y motion since last frame (window pixels). */
        int rel_y = 0;

        /** @brief True if left button was just pressed this frame. */
        bool button_left_pressed = false;
        /** @brief True if middle button was just pressed this frame. */
        bool button_middle_pressed = false;
        /** @brief True if right button was just pressed this frame. */
        bool button_right_pressed = false;
        /** @brief True if left button was just released this frame. */
        bool button_left_released = false;
        /** @brief True if middle button was just released this frame. */
        bool button_middle_released = false;
        /** @brief True if right button was just released this frame. */
        bool button_right_released = false;
        /** @brief True if left button is currently held down. */
        bool button_left_down = false;
        /** @brief True if middle button is currently held down. */
        bool button_middle_down = false;
        /** @brief True if right button is currently held down. */
        bool button_right_down = false;
        /** @brief True if left button is down and mouse is moving. */
        bool dragging = false;

        /** @brief Default constructor ensures proper initialization. */
        MouseState() = default;
    };

    /** @defgroup _WINDOW Window initialization and handling
     *  @{
     */

    /** @brief Creates and shows a framed window. Must be the first SGG function called.
     *  @param width Window width in pixels (> 0).
     *  @param height Window height in pixels (> 0).
     *  @param title Window title text.
     */
    void createWindow(int width, int height, const std::string &title);

    /** @brief Sets the background color outside the canvas area.
     *  @param style Brush defining the background color (only fill_color is used).
     */
    void setWindowBackground(const Brush &style) noexcept;

    /** @brief Destroys the application window and cleans up resources.
     */
    void destroyWindow() noexcept;

    /** @brief Starts the main event processing and rendering loop.
     *  @note Blocks until stopMessageLoop() is called or the window is closed.
     *  @note Ensure all callbacks (draw, update, etc.) are set before calling.
     */
    void startMessageLoop();

    /** @brief Signals the engine to terminate the message loop.
     *  @note Returns control to the caller of startMessageLoop().
     */
    void stopMessageLoop() noexcept;

    /** @brief Defines the logical size of the drawing canvas in custom units.
     *  @param w Canvas width in custom units (> 0).
     *  @param h Canvas height in custom units (> 0).
     */
    void setCanvasSize(float w, float h);

    /** @brief Defines how the canvas scales to fit the window size.
     *  @param sm Scaling mode.
     */
    void setCanvasScaleMode(scale_mode_t sm) noexcept;

    /** @brief Sets the application window to full screen or windowed mode.
     *  @param fs True for full screen, false for windowed.
     */
    void setFullScreen(bool fs) noexcept;

    /** @} */

    /** @defgroup _COORDS Coordinate Conversion
     *  @{
     */

    /** @brief Converts a horizontal window coordinate to a canvas coordinate.
     *  @param x Window X coordinate (pixels).
     *  @param clamped If true, clamp result to canvas bounds.
     *  @return Corresponding canvas X coordinate.
     */
    HOT_FUNCTION float windowToCanvasX(float x, bool clamped = true) noexcept;

    /** @brief Converts a vertical window coordinate to a canvas coordinate.
     *  @param y Window Y coordinate (pixels).
     *  @param clamped If true, clamp result to canvas bounds.
     *  @return Corresponding canvas Y coordinate.
     */
    HOT_FUNCTION float windowToCanvasY(float y, bool clamped = true) noexcept;

    /** @brief Current canvas width in logical units. */
    float getCanvasWidth();

    /** @brief Current canvas height in logical units. */
    float getCanvasHeight();

    /** @} */

    /** @defgroup _CALLBACK Callback setup
     *  @{
     */

    /** @brief Specifies a function to be called for rendering the scene.
     *  @param draw The draw callback function.
     */
    void setDrawFunction(std::function<void()> draw);

    /** @brief Specifies a function called before the main draw function.
     *  @param pre_draw The pre-draw callback function.
     */
    void setPreDrawFunction(std::function<void()> pre_draw);

    /** @brief Specifies a function called after the main draw function.
     *  @param post_draw The post-draw callback function.
     */
    void setPostDrawFunction(std::function<void()> post_draw);

    /** @brief Specifies a function for updating application logic and state.
     *  @param update The update callback function (receives delta time in milliseconds).
     */
    void setUpdateFunction(std::function<void(float)> update);

    /** @brief Specifies a function called when the window is resized.
     *  @param resize The resize callback function (receives new width, height in pixels).
     */
    void setResizeFunction(std::function<void(int, int)> resize);

    /** @} */

    /** @defgroup _USERDATA User Data Storage
     *  @{
     */

    /** @brief Stores an arbitrary user-provided pointer within the engine.
     *  @param user_data Pointer to user data.
     */
    void setUserData(const void *user_data) noexcept;

    /** @brief Retrieves the user-provided pointer stored with setUserData().
     *  @return Pointer to user data, or nullptr if none set.
     */
    void *getUserData() noexcept;

    /** @} */

    /** @defgroup _INPUT Input Handling
     *  @{
     */

    /** @brief Gets the current state of the mouse.
     *  @param[out] ms MouseState struct to be filled with current state.
     */
    HOT_FUNCTION void getMouseState(MouseState &ms) noexcept;

    /** @brief Checks if a specific keyboard key is currently pressed.
     *  @param key Scancode of the key to check (see scancodes.h).
     *  @return True if the key is currently pressed, false otherwise.
     */
    HOT_FUNCTION bool getKeyState(scancode_t key) noexcept;

    /** @brief Gets the current window width.
     *  @return Window width in pixels.
     */
    int getWindowWidth();

    /** @brief Gets the current window height.
     *  @return Window height in pixels.
     */
    int getWindowHeight();

    /** @} */

    /** @defgroup _TIME Time reporting
     *  @{
     */

    /** @brief Returns the time elapsed since the last frame/update.
     *  @return Delta time in milliseconds.
     */
    HOT_FUNCTION float getDeltaTime() noexcept;

    /** @brief Returns the total time elapsed since createWindow() was called.
     *  @return Global time in milliseconds.
     */
    HOT_FUNCTION float getGlobalTime() noexcept;

    /** @} */

    /** @defgroup _PERF Performance Settings
     *  @{
     */

    /** @brief Gets the current rendering frame rate.
     *  @return Current FPS or 0 if the engine is not initialized.
     */
    HOT_FUNCTION float getFPS() noexcept;

    /** @brief Returns the CPU-side render/update time of the last frame (ms).
     *  @return Render time in milliseconds or 0 if the engine is not initialized.
     */
    HOT_FUNCTION float getRenderTime() noexcept;

    /** @brief Returns the frame time in milliseconds.
     *  @return Frame time in milliseconds or 0 if the engine is not initialized.
     */
    HOT_FUNCTION float getFrameTime() noexcept;

    /** @brief Gets the instantaneous rendering frame rate.
        *  @return Instantaneous FPS or 0 if the engine is not initialized.
        */
    HOT_FUNCTION float getInstantaneousFPS() noexcept;

    /** @brief Gets the current projection matrix used by the renderer.
     *  @return Reference to the projection matrix or identity matrix if engine is not initialized.
     */
    const glm::mat4 &getProjectionMatrix() noexcept;

    /** @brief Sets a target frame rate for the engine.
     *  @param fps Target frames per second (0 means no limit).
     */
    void setTargetFPS(int fps) noexcept;

    /** @brief Enables or disables VSync.
     *  @param VSYNC True to enable VSync, false to disable.
     */
    void setVSYNC(bool VSYNC = true) noexcept;

    /** @} */

    /** @defgroup _GRAPHICS Graphics output
     *  @{
     */

    /** @brief Sets the title text displayed on the window frame.
     *  @param title The new window title (must not be null).
     *  @return True on success, false otherwise.
     */
    bool setWindowName(const char *title) noexcept;

    /** @brief Draws a triangle defined by three vertices.
     *  @param x1, y1 Coordinates of the first vertex.
     *  @param x2, y2 Coordinates of the second vertex.
     *  @param x3, y3 Coordinates of the third vertex.
     *  @param brush Drawing attributes (color, outline, texture, etc.).
     */
    HOT_FUNCTION void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                                   const Brush &brush) noexcept;

    /** @brief Draws a triangle defined by three vertices with explicit Z coordinates.
     *  @param x1, y1, z1 Coordinates of the first vertex.
     *  @param x2, y2, z2 Coordinates of the second vertex.
     *  @param x3, y3, z3 Coordinates of the third vertex.
     *  @param brush Drawing attributes (color, outline, texture, etc.).
     */
    HOT_FUNCTION void drawTriangle(float x1, float y1, float z1,
                                   float x2, float y2, float z2,
                                   float x3, float y3, float z3,
                                   const Brush &brush) noexcept;

    /** @brief Draws a rectangle.
     *  @param center_x, center_y Coordinates of the rectangle's center.
     *  @param width Width of the rectangle (> 0).
     *  @param height Height of the rectangle (> 0).
     *  @param brush Drawing attributes.
     */
    HOT_FUNCTION void drawRect(float center_x, float center_y, float width, float height, const Brush &brush) noexcept;

    /** @brief Draws a rectangle with an explicit Z coordinate.
     *  @param center_x, center_y, center_z Coordinates of the rectangle's center.
     *  @param width Width of the rectangle (> 0).
     *  @param height Height of the rectangle (> 0).
     *  @param brush Drawing attributes.
     */
    HOT_FUNCTION void drawRect(float center_x, float center_y, float center_z,
                               float width, float height, const Brush &brush) noexcept;

    /** @brief Draws a line segment between two points.
     *  @param x1, y1 Coordinates of the start point.
     *  @param x2, y2 Coordinates of the end point.
     *  @param brush Drawing attributes (outline properties typically used).
     */
    HOT_FUNCTION void drawLine(float x1, float y1, float x2, float y2, const Brush &brush) noexcept;

    /** @brief Draws a line segment between two points with explicit Z coordinates.
     *  @param x1, y1, z1 Coordinates of the start point.
     *  @param x2, y2, z2 Coordinates of the end point.
     *  @param brush Drawing attributes (outline properties typically used).
     */
    HOT_FUNCTION void drawLine(float x1, float y1, float z1,
                               float x2, float y2, float z2,
                               const Brush &brush) noexcept;

    /** @brief Sets the font file for subsequent drawText calls.
     *  @param fontname Path to the font file (e.g., TTF, must not be empty).
     *  @return True on success, false otherwise.
     */
    bool setFont(const std::string &fontname);

    /** @brief Draws text on the canvas.
     *  @param pos_x, pos_y Coordinates of the text's baseline start position.
     *  @param size Font size in custom units (> 0).
     *  @param text The text string to draw (must not be empty).
     *  @param brush Drawing attributes (fill color/opacity primarily used).
     */
    HOT_FUNCTION void drawText(float pos_x, float pos_y, float size, const std::string &text,
                               const Brush &brush) noexcept;

    /** @brief Draws a filled circle (disk).
     *  @param x, y Coordinates of the disk's center.
     *  @param radius Radius of the disk (> 0).
     *  @param brush Drawing attributes.
     */
    HOT_FUNCTION void drawDisk(float x, float y, float radius, const Brush &brush) noexcept;

    /** @brief Draws a filled sector of an annulus (pie slice or ring segment).
     *  @param cx, cy Coordinates of the center.
     *  @param radius1 Inner radius (>= 0).
     *  @param radius2 Outer radius (>= radius1, > 0).
     *  @param start_angle Starting angle in degrees.
     *  @param end_angle Ending angle in degrees.
     *  @param brush Drawing attributes.
     */
    void drawSector(float cx, float cy, float radius1, float radius2, float start_angle, float end_angle,
                    const Brush &brush) noexcept;

    /** @brief Draws a filled sector of an annulus (pie slice or ring segment) with an explicit Z coordinate.
     *  @param cx, cy, cz Coordinates of the center.
     *  @param start_angle Starting angle in degrees.
     *  @param end_angle Ending angle in degrees.
     *  @param radius1 Inner radius (>= 0).
     *  @param radius2 Outer radius (>= radius1, > 0).
     *  @param brush Drawing attributes.
     */
    void drawSector(float cx, float cy, float cz,
                    float start_angle, float end_angle,
                    float radius1, float radius2,
                    const Brush &brush) noexcept;

    /** @} */

    /** @defgroup _TRANSFORM Transformations
     *  @{
     */

    /** @brief Sets the global rotation for subsequent drawing calls.
     *  @param angle Rotation angle in degrees.
     */
    void setOrientation(float angle) noexcept;

    /** @brief Sets the global scale factors for subsequent drawing calls.
     *  @param sx Horizontal scale factor.
     *  @param sy Vertical scale factor.
     */
    void setScale(float sx, float sy) noexcept;

    /** @brief Resets the global rotation and scale to identity.
     */
    void resetPose() noexcept;

    /** @brief Applies translation to the current transformation.
     *  @param dx X-axis translation.
     *  @param dy Y-axis translation.
     */
    void translate(float dx, float dy) noexcept;

    /** @brief Applies rotation to the current transformation.
     *  @param angleDeg Rotation angle in degrees.
     */
    void rotate(float angleDeg) noexcept;

    /** @brief Applies scaling to the current transformation.
     *  @param sx X-axis scale factor.
     *  @param sy Y-axis scale factor.
     */
    void scale(float sx, float sy) noexcept;

    /** @} */

    /** @defgroup _RESOURCES Resource Loading
     *  @{
     */

    /** @brief Preloads bitmap (texture) files from a directory.
     *  @param dir Path to the directory containing image files (must not be empty).
     *  @return Vector of successfully loaded bitmap filenames.
     */
    std::vector<std::string> preloadBitmaps(const std::string &dir);

    /** @} */

    /** @defgroup _AUDIO Audio Playback
     *  @{
     */

    /** @brief Plays a short sound effect.
     *  @param soundfile Path to the sound file (must not be empty).
     *  @param volume Playback volume [0.0, 1.0].
     *  @param looping True to loop the sound, false to play once.
     */
    void playSound(const std::string &soundfile, float volume = 1.0f, bool looping = false);

    /** @brief Stops currently playing background music.
     *  @param fade_time Fade-out duration in milliseconds (0 for immediate stop).
     */
    void stopMusic(int fade_time = 0) noexcept;

    /** @brief Plays background music.
     *  @param soundfile Path to the music file (must not be empty).
     *  @param volume Playback volume [0.0, 1.0].
     *  @param looping True to loop the music, false to play once.
     *  @param fade_time Fade-in duration in milliseconds (0 for immediate start).
     */
    void playMusic(const std::string &soundfile, float volume = 1.0f, bool looping = true, int fade_time = 0);

    /** @} */

    /** @defgroup _MISC Miscellaneous
     *  @{
     */

    /** @brief Gets a pointer to the internal TextureManager instance.
     *  @return Pointer to TextureManager, or nullptr if engine not initialized.
     */
    TextureManager *getTextureManager() noexcept;

    /** @brief Returns the default shader object used by the renderer.
     *  @return Pointer to the default shader object, or nullptr if engine not initialized.
     */
    Shader *getDefaultShader() noexcept;

    /** @} */
} // namespace graphics

/** @namespace advanced_gfx
 *  @brief Contains functions for low-level rendering in the Advanced Graphics library.
 */
namespace advanced_gfx {
    /** @brief Initializes advanced graphics subsystem and OpenGL capabilities.
     *  @return True on success, false on failure.
     */
    bool init();

    /** @brief Frees all resources created by AdvancedGraphics.
     */
    void shutdown();

    /** @brief Creates a vertex buffer with the specified data.
     *  @param vertices Pointer to vertex data.
     *  @param size Size of vertex data in bytes.
     *  @param usage Buffer usage hint (StaticDraw, DynamicDraw, etc.).
     *  @return BufferId for the created buffer.
     */
    BufferId createVertexBuffer(const void *vertices,
                                GLsizeiptr size,
                                BufferUsage usage = BufferUsage::StaticDraw);

    /** @brief Creates an index buffer with the specified data.
     *  @param indices Pointer to index data.
     *  @param size Size of index data in bytes.
     *  @param usage Buffer usage hint (StaticDraw, DynamicDraw, etc.).
     *  @return BufferId for the created buffer.
     */
    BufferId createIndexBuffer(const void *indices,
                               GLsizeiptr size,
                               BufferUsage usage = BufferUsage::StaticDraw);

    /** @brief Creates a new Vertex Array Object (VAO).
     *  @return VAOId for the created VAO.
     */
    VAOId createVAO();

    /** @brief Configures vertex attributes for a VAO.
     *  @param vaoId ID of the VAO to configure.
     *  @param vertexBuffer Buffer containing vertex data.
     *  @param attributes Vector of vertex attribute descriptions.
     */
    void setVAOAttributes(VAOId vaoId,
                          BufferId vertexBuffer,
                          const std::vector<VertexAttribute> &attributes);

    /** @brief Associates an index buffer with a VAO.
     *  @param vaoId ID of the VAO.
     *  @param indexBuffer Buffer containing index data.
     */
    void setIndexBuffer(VAOId vaoId, BufferId indexBuffer);

    /** @brief Binds a VAO for rendering.
     *  @param vaoId ID of the VAO to bind.
     */
    void bindVAO(VAOId vaoId);

    /** @brief Unbinds the currently bound VAO.
     */
    void unbindVAO();

    /** @brief Creates a shader program from source code strings.
     *  @param vertexSource Vertex shader source code.
     *  @param fragmentSource Fragment shader source code.
     *  @return Shared pointer to the created shader program.
     */
    std::shared_ptr<Shader> createShaderFromSource(const std::string &vertexSource,
                                                   const std::string &fragmentSource);

    /** @brief Creates a shader program from file paths.
     *  @param vertexPath Path to the vertex shader file.
     *  @param fragmentPath Path to the fragment shader file.
     *  @return Shared pointer to the created shader program.
     */
    std::shared_ptr<Shader> createShaderFromFiles(const std::string &vertexPath,
                                                  const std::string &fragmentPath);

    /** @brief Gets a uniform handle for setting shader uniforms.
     *  @param shader Shared pointer to the shader.
     *  @param name Name of the uniform variable.
     *  @return UniformHandle for the specified uniform.
     */
    UniformHandle getUniform(const std::shared_ptr<Shader> &shader, std::string_view name);

    /** @brief Loads a texture from file.
     *  @param name Path to the texture file.
     *  @param useLodepng True to use lodepng for PNG loading, false for other methods.
     *  @param customBuildFn Optional custom function for texture building.
     *  @return Pointer to the loaded texture, or nullptr on failure.
     */
    graphics::Texture *loadTexture(const std::string &name,
                                   bool useLodepng = true,
                                   const std::function<void(graphics::Texture &)> &customBuildFn = nullptr);

    /** @brief Binds a texture to a specific texture unit.
     *  @param texture Pointer to the texture to bind.
     *  @param slot Texture unit to bind to (0-31).
     */
    void bindTexture(graphics::Texture *texture, unsigned int slot);

    /** @brief Unbinds the texture from a specific texture unit.
     *  @param slot Texture unit to unbind.
     */
    void unbindTextureSlot(unsigned int slot);

    /** @brief Unbinds all textures from all texture units.
     */
    void unbindAllTextures();

    /** @brief Draws arrays using the currently bound VAO.
     *  @param mode Primitive type to draw.
     *  @param first First vertex to draw.
     *  @param count Number of vertices to draw.
     */
    inline void drawArrays(DrawCommands::PrimitiveType mode, GLint first, GLsizei count) {
        DrawCommands::drawArrays(mode, first, count);
    }

    /** @brief Draws elements using the currently bound VAO and index buffer.
     *  @param mode Primitive type to draw.
     *  @param count Number of indices to draw.
     *  @param type Data type of the indices.
     *  @param indices Pointer to index data (nullptr to use bound index buffer).
     */
    inline void drawElements(DrawCommands::PrimitiveType mode, GLsizei count,
                             DrawCommands::IndexType type = DrawCommands::IndexType::UnsignedInt,
                             const void *indices = nullptr) {
        DrawCommands::drawElements(mode, count, type, indices);
    }

    /** @brief Draws a fullscreen quad for post-processing effects.
     */
    inline void drawFullscreenQuad() { DrawCommands::drawFullscreenQuad(); }

    /** @brief Enables or disables OpenGL debug groups.
     *  @param enabled True to enable debug groups, false to disable.
     */
    void enableDebugGroups(bool enabled);

    /** @brief Creates a batch renderer with default settings.
     *  @return Unique pointer to the batch renderer.
     */
    std::unique_ptr<BatchRenderer> makeDefaultBatch();

    /** @brief Creates a batch renderer with custom capacity.
     *  @param maxVertexCount Maximum number of vertices.
     *  @param maxIndexCount Maximum number of indices.
     *  @return Unique pointer to the batch renderer.
     */
    std::unique_ptr<BatchRenderer> createBatch(GLsizeiptr maxVertexCount,
                                               GLsizeiptr maxIndexCount);

    /** @brief Flushes a batch renderer, drawing all queued primitives.
     *  @param batch Reference to the batch renderer to flush.
     */
    void flushBatch(BatchRenderer &batch);

    /** @brief Gets statistics about buffer usage.
     *  @return Buffer factory statistics.
     */
    BufferFactory::Stats getBufferStats();

    /** @brief Optimizes buffer usage and performance.
     */
    void optimizeBuffers();

    /** @brief Sets texture parameters for filtering and wrapping.
     *  @param texture Pointer to the texture.
     *  @param minFilter Minification filter (GL_LINEAR, GL_NEAREST, etc.).
     *  @param magFilter Magnification filter (GL_LINEAR, GL_NEAREST).
     *  @param wrapS Horizontal wrapping mode (GL_REPEAT, GL_CLAMP_TO_EDGE, etc.).
     *  @param wrapT Vertical wrapping mode (GL_REPEAT, GL_CLAMP_TO_EDGE, etc.).
     */
    void setTextureParameters(graphics::Texture *texture,
                              GLint minFilter,
                              GLint magFilter,
                              GLint wrapS,
                              GLint wrapT);
} // namespace advanced_gfx

#endif //GRAPHICS_H
