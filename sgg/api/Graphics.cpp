#include "api/headers/Graphics.h"
#include "core/backend/GLBackend.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Global engine pointer - initialized to nullptr
// GLBackend is now the central authority governing both APIs
static graphics::GLBackend *engine = nullptr;

// Advanced Graphics internal components (managed by GLBackend)
namespace advanced_gfx {
    static std::unique_ptr<BufferFactory>     g_bufferFactory;
    static std::unique_ptr<VAOFactory>        g_vaoFactory;
    static bool                               g_initialised = false;
    static GLPerformanceMonitor               g_perfMonitor;
    static bool                               g_useDebugGroups = true;

    // Helper to read entire file into string
    static std::string readFile(const std::string& path) {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file) throw std::runtime_error("AdvancedGraphics: failed to open file " + path);
        std::ostringstream ss; ss << file.rdbuf();
        return ss.str();
    }
}

// =============================================================================
// GRAPHICS NAMESPACE IMPLEMENTATION (Simple Game Graphics API)
// =============================================================================

namespace graphics {

    // Optimized engine check - marked as hot path and force inlined
    [[msvc::forceinline]]
    inline bool checkEngineInitialized() noexcept {
        return LIKELY(engine != nullptr);
    }

    // Cold path error reporting - separate function to keep hot paths lean
    [[gnu::cold]] [[gnu::noinline]]
    static void reportUninitializedError(const char* functionName) noexcept {
        std::cerr << "ERROR [graphics::" << (functionName ? functionName : "<unknown>")
                  << "]: Graphics engine not initialized. Call createWindow() first.\n";
    }

    // --- Time Functions (Hot Path) ---
    float getDeltaTime() noexcept {
        return checkEngineInitialized() ? engine->getDeltaTime() : 0.0f;
    }

    float getGlobalTime() noexcept {
        return checkEngineInitialized() ? engine->getGlobalTime() : 0.0f;
    }

    // --- Window Functions ---
    bool setWindowName(const char *title) noexcept {
        return (checkEngineInitialized() && LIKELY(title)) ? engine->setWindowName(title) : false;
    }

    void createWindow(int width, int height, const std::string& title) {
        if (UNLIKELY(engine)) {
            std::cerr << "WARNING [graphics::createWindow]: Window already created. Ignoring request.\n";
            return;
        }

        if (UNLIKELY(width <= 0 || height <= 0)) {
            std::cerr << "ERROR [graphics::createWindow]: Window dimensions must be positive. Got "
                      << width << "x" << height << ".\n";
            return;
        }

        engine = new(std::nothrow) GLBackend(width, height, title);
        if (UNLIKELY(!engine || !engine->init())) {
            std::cerr << "ERROR [graphics::createWindow]: Failed to initialize graphics backend.\n";
            delete engine;
            engine = nullptr;
        } else {
            engine->show(true);
            advanced_gfx::init();
        }
    }

    void setWindowBackground(const Brush &style) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setBackgroundColor(style.fill_color[0], style.fill_color[1], style.fill_color[2]);
        }
    }

    void destroyWindow() noexcept {
        if (LIKELY(engine)) {
            // Clean up advanced graphics first
            advanced_gfx::shutdown();
            engine->cleanup();
            delete engine;
            engine = nullptr;
        }
    }

    void startMessageLoop() {
        if (UNLIKELY(!checkEngineInitialized())) {
            reportUninitializedError("startMessageLoop");
            return;
        }

        while (LIKELY(engine->processMessages())) {
            // Continue processing - empty body is intentional
        }
    }

    void stopMessageLoop() noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->terminate();
        }
    }

    // --- Drawing Functions (Critical Hot Path) ---
    void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Brush &brush) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->drawTriangle(x1, y1, x2, y2, x3, y3, brush);
        }
    }

    void drawTriangle(float x1, float y1, float z1,
                      float x2, float y2, float z2,
                      float x3, float y3, float z3,
                      const Brush &brush) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->drawTriangle(x1, y1, z1,
                                 x2, y2, z2,
                                 x3, y3, z3, brush);
        }
    }

    void drawRect(float center_x, float center_y, float width, float height, const Brush &brush) noexcept {
        if (UNLIKELY(!checkEngineInitialized())) return;

        // Debug validation only in debug builds
        #ifdef _DEBUG
        if (UNLIKELY(width <= 0 || height <= 0)) {
            std::cerr << "WARNING [graphics::drawRect]: Width and height should be positive. Got "
                      << width << "x" << height << ". Skipping draw.\n";
            return;
        }
        #endif

        engine->drawRect(center_x, center_y, width, height, brush);
    }

    void drawRect(float center_x, float center_y, float center_z,
                  float width, float height, const Brush &brush) noexcept {
        if (UNLIKELY(!checkEngineInitialized())) return;

        #ifdef _DEBUG
        if (UNLIKELY(width <= 0 || height <= 0)) {
            std::cerr << "WARNING [graphics::drawRect]: Width and height should be positive. Skipping draw.\n";
            return;
        }
        #endif

        engine->drawRect(center_x, center_y, center_z, width, height, brush);
    }

    void drawLine(float x1, float y1, float x2, float y2, const Brush &brush) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->drawLine(x1, y1, x2, y2, brush);
        }
    }

    void drawLine(float x1, float y1, float z1,
                  float x2, float y2, float z2,
                  const Brush &brush) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->drawLine(x1, y1, z1, x2, y2, z2, brush);
        }
    }

    bool setFont(const std::string& fontname) {
        if (UNLIKELY(!checkEngineInitialized() || fontname.empty())) {
            if (fontname.empty()) {
                std::cerr << "ERROR [graphics::setFont]: Font name cannot be empty.\n";
            }
            return false;
        }
        return engine->setFont(fontname);
    }

    void drawText(float pos_x, float pos_y, float size, const std::string &text, const Brush &brush) noexcept {
        if (UNLIKELY(!checkEngineInitialized() || text.empty())) return;

        #ifdef _DEBUG
        if (UNLIKELY(size <= 0)) {
            std::cerr << "WARNING [graphics::drawText]: Font size should be positive. Got "
                      << size << ". Skipping draw.\n";
            return;
        }
        #endif

        engine->drawText(pos_x, pos_y, size, text, brush);
    }

    void drawDisk(float x, float y, float radius, const Brush &brush) noexcept {
        if (UNLIKELY(!checkEngineInitialized())) return;

        #ifdef _DEBUG
        if (UNLIKELY(radius <= 0)) {
            std::cerr << "WARNING [graphics::drawDisk]: Radius should be positive. Got "
                      << radius << ". Skipping draw.\n";
            return;
        }
        #endif

        engine->drawSector(x, y, 0, 360, 0.0f, radius, brush);
    }

    void drawSector(float cx, float cy, float radius1, float radius2, float start_angle, float end_angle,
                    const Brush &brush) noexcept {
        if (UNLIKELY(!checkEngineInitialized())) return;

        // Quick early exit for degenerate case
        if (UNLIKELY(radius1 == 0.0f && radius2 == 0.0f)) return;

        #ifdef _DEBUG
        if (UNLIKELY(radius1 < 0 || radius2 < 0 || radius2 < radius1)) {
            std::cerr << "WARNING [graphics::drawSector]: Invalid radii (r1=" << radius1 << ", r2=" << radius2
                      << "). Skipping draw.\n";
            return;
        }
        #endif

        engine->drawSector(cx, cy, start_angle, end_angle, radius1, radius2, brush);
    }

    void drawSector(float cx, float cy, float cz,
                    float start_angle, float end_angle,
                    float radius1, float radius2,
                    const Brush &brush) noexcept {
        if (UNLIKELY(!checkEngineInitialized())) return;
        engine->drawSector(cx, cy, cz, start_angle, end_angle, radius1, radius2, brush);
    }

    // --- Transformation Functions ---
    void setOrientation(float angle) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setOrientation(angle);
        }
    }

    void setScale(float sx, float sy) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setScale(sx, sy, 1.0f);
        }
    }

    void resetPose() noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->resetPose();
        }
    }

    void translate(float dx, float dy) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->translate(dx, dy);
        }
    }

    void rotate(float angleDeg) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->rotate(angleDeg);
        }
    }

    void scale(float sx, float sy) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->scale(sx, sy);
        }
    }

    // --- Resource Loading ---
    std::vector<std::string> preloadBitmaps(const std::string& dir) {
        if (UNLIKELY(!checkEngineInitialized() || dir.empty())) {
            if (dir.empty()) {
                std::cerr << "WARNING [graphics::preloadBitmaps]: Directory name is empty.\n";
            }
            return {};
        }
        return engine->preloadBitmaps(dir);
    }

    // --- Audio Functions ---
    void playSound(const std::string& soundfile, float volume, bool looping) {
        if (UNLIKELY(!checkEngineInitialized() || soundfile.empty())) {
            if (soundfile.empty()) {
                std::cerr << "ERROR [graphics::playSound]: Sound file name cannot be empty.\n";
            }
            return;
        }
        engine->playSound(soundfile, volume, looping);
    }

    void stopMusic(int fade_time) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->stopMusic(fade_time);
        }
    }

    void playMusic(const std::string& soundfile, float volume, bool looping, int fade_time) {
        if (UNLIKELY(!checkEngineInitialized() || soundfile.empty())) {
            if (soundfile.empty()) {
                std::cerr << "ERROR [graphics::playMusic]: Music file name cannot be empty.\n";
            }
            return;
        }
        engine->playMusic(soundfile, volume, looping, fade_time);
    }

    // --- Performance Settings ---
    float getFPS() noexcept {
        if (UNLIKELY(!checkEngineInitialized())) {
            reportUninitializedError("getFPS");
            return 0.0f;
        }
        return static_cast<float>(engine->getFPS());
    }

    float getFrameTime() noexcept {
        if (UNLIKELY(!checkEngineInitialized())) {
            reportUninitializedError("getFrameTime");
            return 0.0f;
        }
        return static_cast<float>(engine->getFrameTime());
    }

    float getInstantaneousFPS() noexcept {
        if (UNLIKELY(!checkEngineInitialized())) {
            reportUninitializedError("getInstantaneousFPS");
            return 0.0f;
        }
        return static_cast<float>(engine->getInstantaneousFPS());
    }

    float getRenderTime() noexcept {
        if (UNLIKELY(!checkEngineInitialized())) {
            reportUninitializedError("getRenderTime");
            return 0.0f;
        }
        return static_cast<float>(engine->getRenderTime());
    }

    const glm::mat4& getProjectionMatrix() noexcept {
        // Thread-safe static initialization (C++11 guaranteed)
        static const glm::mat4 identityMatrix(1.0f);
        return LIKELY(checkEngineInitialized()) ? engine->getProjectionMatrix() : identityMatrix;
    }

    void setTargetFPS(int fps) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setTargetFPS(fps);
        }
    }

    void setVSYNC(bool VSYNC) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setVSYNC(VSYNC);
        }
    }

    // --- Canvas & Coordinate Functions ---
    void setCanvasSize(float w, float h) {
        if (UNLIKELY(!checkEngineInitialized())) return;

        #ifdef _DEBUG
        if (UNLIKELY(w <= 0 || h <= 0)) {
            std::cerr << "ERROR [graphics::setCanvasSize]: Canvas dimensions must be positive. Got "
                      << w << "x" << h << ".\n";
            return;
        }
        #endif

        engine->setCanvasSize(w, h);
    }

    void setCanvasScaleMode(scale_mode_t sm) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setCanvasMode(sm);
        }
    }

    void setFullScreen(bool fs) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setFullscreen(fs);
        }
    }

    float windowToCanvasX(float x, bool clamped) noexcept {
        return LIKELY(checkEngineInitialized()) ? engine->WindowToCanvasX(x, clamped) : 0.0f;
    }

    float windowToCanvasY(float y, bool clamped) noexcept {
        return LIKELY(checkEngineInitialized()) ? engine->WindowToCanvasY(y, clamped) : 0.0f;
    }

    // --- Canvas size helpers -------------------------------------------------
    float getCanvasWidth() {
        return LIKELY(checkEngineInitialized()) ? engine->getCanvasWidth() : 0.0f;
    }

    float getCanvasHeight() {
        return LIKELY(checkEngineInitialized()) ? engine->getCanvasHeight() : 0.0f;
    }

    // --- Callback Functions ---
    void setDrawFunction(std::function<void()> fdraw) {
        if (LIKELY(checkEngineInitialized()) && LIKELY(fdraw)) {
            engine->setDrawCallback(std::move(fdraw));
        }
    }

    void setPreDrawFunction(std::function<void()> pre_draw) {
        if (LIKELY(checkEngineInitialized()) && LIKELY(pre_draw)) {
            engine->setPreDrawCallback(std::move(pre_draw));
        }
    }

    void setPostDrawFunction(std::function<void()> post_draw) {
        if (LIKELY(checkEngineInitialized()) && LIKELY(post_draw)) {
            engine->setPostDrawCallback(std::move(post_draw));
        }
    }

    void setUpdateFunction(std::function<void(float)> fupdate) {
        if (LIKELY(checkEngineInitialized()) && LIKELY(fupdate)) {
            engine->setIdleCallback(std::move(fupdate));
        }
    }

    void setResizeFunction(std::function<void(int, int)> fresize) {
        if (LIKELY(checkEngineInitialized()) && LIKELY(fresize)) {
            engine->setResizeCallback(std::move(fresize));
        }
    }

    // --- User Data ---
    void setUserData(const void *user_data) noexcept {
        if (LIKELY(checkEngineInitialized())) {
            engine->setUserData(user_data);
        }
    }

    void *getUserData() noexcept {
        return LIKELY(checkEngineInitialized()) ? engine->getUserData() : nullptr;
    }

    // --- Input Functions (Hot Path) ---
    void getMouseState(MouseState &ms) noexcept {
        if (UNLIKELY(!checkEngineInitialized())) {
            ms = MouseState{}; // Zero-initialize on failure
            return;
        }

        // Batch state retrieval for better cache performance
        bool button_states[3], button_pressed[3], button_released[3];

        engine->getMouseButtonState(button_states);
        ms.button_left_down = button_states[0];
        ms.button_middle_down = button_states[1];
        ms.button_right_down = button_states[2];

        engine->getMouseButtonPressed(button_pressed);
        ms.button_left_pressed = button_pressed[0];
        ms.button_middle_pressed = button_pressed[1];
        ms.button_right_pressed = button_pressed[2];

        engine->getMouseButtonReleased(button_released);
        ms.button_left_released = button_released[0];
        ms.button_middle_released = button_released[1];
        ms.button_right_released = button_released[2];

        ms.dragging = engine->isMouseDragging();

        // Get position data efficiently
        int x, y;
        engine->getMousePosition(&x, &y);
        ms.cur_pos_x = x;
        ms.cur_pos_y = y;

        engine->getPrevMousePosition(&x, &y);
        ms.prev_pos_x = x;
        ms.prev_pos_y = y;

        engine->getRelativeMousePosition(&x, &y);
        ms.rel_x = x;
        ms.rel_y = y;
    }

    bool getKeyState(scancode_t key) noexcept {
        return LIKELY(checkEngineInitialized()) ? engine->getKeyState(key) : false;
    }

    // --- Window size helpers -------------------------------------------------
    int getWindowWidth() {
        return engine->getWindowWidth();
    }

    int getWindowHeight() {
        return engine->getWindowHeight();
    }

    // --- Miscellaneous ---
    TextureManager *getTextureManager() noexcept {
        return LIKELY(checkEngineInitialized()) ? engine->getTextureManager() : nullptr;
    }

    Shader* getDefaultShader() noexcept {
        return LIKELY(checkEngineInitialized()) ? engine->getDefaultShader() : nullptr;
    }

} // namespace graphics

// =============================================================================
// ADVANCED_GFX NAMESPACE IMPLEMENTATION (Advanced Graphics API)
// =============================================================================

namespace advanced_gfx {

    // --- Initialization / Shutdown ---
    bool init() {
        if (g_initialised) return true;

        // Ensure main graphics engine is initialized first
        if (UNLIKELY(!graphics::checkEngineInitialized())) {
            std::cerr << "ERROR [advanced_gfx::init]: Main graphics engine not initialized. "
                      << "Call graphics::createWindow() first.\n";
            return false;
        }

        try {
            BufferFactory::Config cfg = BufferFactory::Config::minimal();
            g_bufferFactory  = std::make_unique<BufferFactory>(cfg);
            g_vaoFactory     = std::make_unique<VAOFactory>(*g_bufferFactory);
            g_initialised    = true;
        }
        catch (const std::exception& e) {
            // Log and propagate
            std::cerr << "ERROR [advanced_gfx::init]: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    void shutdown() {
        if (!g_initialised) return;

        // Clear GPU resources managed by factories
        if (g_bufferFactory)  g_bufferFactory->clear();
        // All VAOs will be deleted when factory is destroyed.

        // Clear all textures - delegate to main graphics engine
        if (graphics::checkEngineInitialized()) {
            graphics::TextureManager::getInstance().clearAllTextures();
        }

        g_vaoFactory.reset();
        g_bufferFactory.reset();
        g_initialised = false;
    }

    // --- Buffers & VAOs ---
    BufferId createVertexBuffer(const void* vertices, GLsizeiptr size, BufferUsage usage) {
        if (UNLIKELY(!g_initialised)) {
            std::cerr << "ERROR [advanced_gfx::createVertexBuffer]: Advanced graphics not initialized.\n";
            return BufferId{};
        }
        return g_bufferFactory->createVertexBuffer(vertices, size, usage);
    }

    BufferId createIndexBuffer(const void* indices, GLsizeiptr size, BufferUsage usage) {
        if (UNLIKELY(!g_initialised)) {
            std::cerr << "ERROR [advanced_gfx::createIndexBuffer]: Advanced graphics not initialized.\n";
            return BufferId{};
        }
        return g_bufferFactory->createIndexBuffer(indices, size, usage);
    }

    VAOId createVAO() {
        if (UNLIKELY(!g_initialised)) {
            std::cerr << "ERROR [advanced_gfx::createVAO]: Advanced graphics not initialized.\n";
            return VAOId{};
        }
        return g_vaoFactory->createVAO();
    }

    void setVAOAttributes(VAOId vaoId, BufferId vertexBuffer, const std::vector<VertexAttribute>& attrs) {
        if (LIKELY(g_initialised)) {
            g_vaoFactory->configureVertexAttributes(vaoId, vertexBuffer, attrs);
        }
    }

    void setIndexBuffer(VAOId vaoId, BufferId indexBuffer) {
        if (LIKELY(g_initialised)) {
            g_vaoFactory->setIndexBuffer(vaoId, indexBuffer);
        }
    }

    void bindVAO(VAOId vaoId) {
        if (LIKELY(g_initialised)) {
            g_vaoFactory->bindVAO(vaoId);
        }
    }

    void unbindVAO() {
        if (LIKELY(g_initialised)) {
            VAOFactory::unbindVAO();
        }
    }

    // --- Shader helpers ---
    std::shared_ptr<Shader> createShaderFromSource(const std::string& vertexSource,
                                                   const std::string& fragmentSource) {
        if (UNLIKELY(!g_initialised)) {
            std::cerr << "ERROR [advanced_gfx::createShaderFromSource]: Advanced graphics not initialized.\n";
            return nullptr;
        }

        try {
            return std::make_shared<Shader>(vertexSource, fragmentSource);
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR [advanced_gfx::createShaderFromSource]: " << e.what() << std::endl;
            return nullptr;
        }
    }

    std::shared_ptr<Shader> createShaderFromFiles(const std::string& vertexPath,
                                                  const std::string& fragmentPath) {
        if (UNLIKELY(!g_initialised)) {
            std::cerr << "ERROR [advanced_gfx::createShaderFromFiles]: Advanced graphics not initialized.\n";
            return nullptr;
        }

        try {
            auto vertSrc = readFile(vertexPath);
            auto fragSrc = readFile(fragmentPath);
            return std::make_shared<Shader>(vertSrc, fragSrc);
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR [advanced_gfx::createShaderFromFiles]: " << e.what() << std::endl;
            return nullptr;
        }
    }

    UniformHandle getUniform(const std::shared_ptr<Shader>& shader, std::string_view name) {
        return shader ? shader->createUniformHandle(name) : UniformHandle{};
    }

    // --- Texture helpers ---
    graphics::Texture* loadTexture(const std::string& name,
                                   bool useLodepng,
                                   const std::function<void(graphics::Texture&)>& customBuildFn) {
        if (UNLIKELY(!g_initialised || !graphics::checkEngineInitialized())) {
            std::cerr << "ERROR [advanced_gfx::loadTexture]: Graphics system not properly initialized.\n";
            return nullptr;
        }

        try {
            auto* tex = graphics::TextureManager::getInstance().createTexture(name, useLodepng, customBuildFn);
            if (tex && !tex->buildGLTexture()) {
                std::cerr << "ERROR [advanced_gfx::loadTexture]: Failed to build texture " << name << std::endl;
                return nullptr;
            }
            return tex;
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR [advanced_gfx::loadTexture]: " << e.what() << std::endl;
            return nullptr;
        }
    }

    void bindTexture(graphics::Texture* texture, unsigned int slot) {
        if (LIKELY(graphics::checkEngineInitialized())) {
            graphics::TextureManager::getInstance().bindTexture(texture, slot);
        }
    }

    void unbindTextureSlot(unsigned int slot) {
        if (LIKELY(graphics::checkEngineInitialized())) {
            graphics::TextureManager::getInstance().unbindTexture(slot);
        }
    }

    void unbindAllTextures() {
        if (LIKELY(graphics::checkEngineInitialized())) {
            graphics::TextureManager::getInstance().unbindAllTextures();
        }
    }

    void enableDebugGroups(bool enabled) {
        g_useDebugGroups = enabled;
    }

    // --- Batch Rendering ---
    std::unique_ptr<BatchRenderer> makeDefaultBatch() {
        if (UNLIKELY(!g_initialised)) {
            std::cerr << "ERROR [advanced_gfx::makeDefaultBatch]: Advanced graphics not initialized.\n";
            return nullptr;
        }
        return BatchRendererFactory::createDefault();
    }

    std::unique_ptr<BatchRenderer> createBatch(GLsizeiptr maxVertexCount, GLsizeiptr maxIndexCount) {
        if (UNLIKELY(!g_initialised)) {
            std::cerr << "ERROR [advanced_gfx::createBatch]: Advanced graphics not initialized.\n";
            return nullptr;
        }
        return BatchRendererFactory::create(maxVertexCount, maxIndexCount);
    }

    void flushBatch(BatchRenderer& batch) {
        batch.flush();
    }

    // --- Buffer Factory utilities ---
    BufferFactory::Stats getBufferStats() {
        return g_bufferFactory ? g_bufferFactory->getStats() : BufferFactory::Stats{};
    }

    void optimizeBuffers() {
        if (g_bufferFactory) g_bufferFactory->optimize();
    }

    // --- Texture parameter helpers ---
    void setTextureParameters(graphics::Texture* texture,
                              GLint minFilter,
                              GLint magFilter,
                              GLint wrapS,
                              GLint wrapT) {
        if (!texture) return;
        GLuint id = texture->getID();
        if (!id) return;
        GL_CHECK_NOTHROW(glBindTexture(GL_TEXTURE_2D, id));
        GL_CHECK_NOTHROW(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
        GL_CHECK_NOTHROW(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));
        GL_CHECK_NOTHROW(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS));
        GL_CHECK_NOTHROW(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT));
        GL_CHECK_NOTHROW(glBindTexture(GL_TEXTURE_2D, 0));
    }

} // namespace advanced_gfx
