#pragma once

#include "core/graphics/rendering/utils/headers/GLUtils.h" // Includes <GL/glew.h> and necessary enums

/**
 * @brief RAII wrapper for OpenGL buffer objects (VBO, EBO, UBO).
 *
 * Manages the lifecycle of a GPU buffer with optimal performance and safety.
 * Features automatic initialization, efficient updates, and move-only semantics.
 */
class Buffer {
private:
    GLuint m_id = 0;                ///< OpenGL buffer object ID.
    GLenum m_target;               ///< Cached GL target (e.g., GL_ARRAY_BUFFER).
    GLenum m_usage;                ///< Cached GL usage pattern (e.g., GL_STATIC_DRAW).
    GLsizeiptr m_size = 0;         ///< Current size of the buffer in bytes.
    BufferType m_type;             ///< Buffer type for external queries.
    bool m_initialized = false;     ///< Initialization state.

    /**
     * @brief Ensures the buffer is initialized, creating it if necessary.
     */
    void ensureInitialized();

public:
    /**
     * @brief Constructs a Buffer wrapper.
     * @param type The type of buffer to create.
     * @param usage The expected usage pattern.
     */
    Buffer(BufferType type, BufferUsage usage) noexcept;

    /**
     * @brief Destructor. Safely deletes the OpenGL buffer object.
     */
    ~Buffer() noexcept;

    // --- Move-only semantics ---
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // --- High-level operations ---

    /**
     * @brief Uploads data to the buffer, initializing if necessary.
     *
     * Automatically handles buffer creation and optimal data transfer.
     * @param data Pointer to the data (can be nullptr for allocation-only).
     * @param size Size of the data in bytes.
     * @throws std::runtime_error on OpenGL errors.
     */
    void setData(const void* data, GLsizeiptr size);

    /**
     * @brief Updates a portion of the buffer with new data.
     *
     * Automatically handles buffer growth and uses optimal GL calls.
     * @param data Pointer to the new data.
     * @param size Size of the new data in bytes.
     * @param offset Byte offset within the buffer.
     * @throws std::runtime_error on OpenGL errors.
     */
    void updateData(const void* data, GLsizeiptr size, GLintptr offset = 0);

    /**
     * @brief Reserves buffer space without uploading data.
     * @param size Size to reserve in bytes.
     */
    void reserve(GLsizeiptr size);

    /**
     * @brief Binds the buffer to its target.
     */
    void bind() const noexcept;

    /**
     * @brief Unbinds any buffer from this buffer's target.
     */
    void unbind() const noexcept;

    /**
     * @brief Clears the buffer and releases GPU memory.
     */
    void clear() noexcept;

    // --- Getters ---
    [[nodiscard]] GLuint getId() const noexcept { return m_id; }
    [[nodiscard]] BufferType getType() const noexcept { return m_type; }
    [[nodiscard]] GLsizeiptr getSize() const noexcept { return m_size; }
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }
    [[nodiscard]] bool isEmpty() const noexcept { return m_size == 0; }

    // --- Compatibility methods (deprecated but functional) ---
    [[deprecated("Use setData() instead")]]
    void init(const void* data, GLsizeiptr size) { setData(data, size); }

    [[deprecated("Use updateData() instead")]]
    void update(const void* data, GLsizeiptr size, GLintptr offset = 0) { updateData(data, size, offset); }

    [[deprecated("Use getType() instead")]]
    [[nodiscard]] BufferUsage getUsage() const { return static_cast<BufferUsage>(m_usage); }
};