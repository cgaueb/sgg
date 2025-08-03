#pragma once

#include "core/graphics/rendering/core/headers/Buffer.h"
#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include <vector>
#include <memory>

/**
 * @brief Represents an OpenGL Vertex Array Object (VAO).
 *
 * Encapsulates the state needed to specify vertex attribute data, including
 * which VBOs to use and how to interpret the vertex data within those VBOs.
 */
class VertexArray {
private:
    GLuint m_id = 0;
    bool m_initialized = false;

    // Use small_vector optimization - most VAOs have 1-3 vertex buffers
    std::vector<std::shared_ptr<Buffer>> m_vertexBuffers;
    std::shared_ptr<Buffer> m_indexBuffer;

    /**
     * @brief Ensures the VAO is initialized, calling init() if necessary.
     */
    void ensureInitialized();

    /**
     * @brief Internal cleanup helper for move operations and destructor.
     */
    void cleanup() noexcept;

public:
    /**
     * @brief Default constructor. VAO is created lazily on first use.
     */
    VertexArray() = default;

    /**
     * @brief Destructor. Safely deletes the OpenGL VAO if initialized.
     */
    ~VertexArray();

    // Non-copyable, moveable
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    /**
     * @brief Move constructor.
     * @param other The VertexArray object to move resources from.
     */
    VertexArray(VertexArray&& other) noexcept;

    /**
     * @brief Move assignment operator.
     * @param other The VertexArray object to move resources from.
     * @return Reference to this VertexArray.
     */
    VertexArray& operator=(VertexArray&& other) noexcept;

    // --- OpenGL Operations ---

    /**
     * @brief Explicitly initializes the VAO by generating the OpenGL ID.
     * @throws std::runtime_error if VAO creation fails.
     */
    void init();

    /**
     * @brief Binds the VAO, making it the active vertex array state.
     * @throws std::runtime_error if VAO is not initialized.
     */
    void bind() const;

    /**
     * @brief Unbinds any currently bound VAO.
     */
    static void unbind() noexcept;

    /**
     * @brief Associates a Vertex Buffer with this VAO and configures vertex attributes.
     *
     * @param vertexBuffer A shared_ptr to the Buffer object (must be a VERTEX_BUFFER).
     * @param attributes Vertex attributes stored in the buffer.
     * @throws std::invalid_argument if attributes is empty or buffer is invalid.
     */
    void addVertexBuffer(const std::shared_ptr<Buffer>& vertexBuffer,
                        const std::vector<VertexAttribute>& attributes);

    /**
     * @brief Associates an Index Buffer with this VAO.
     *
     * @param indexBuffer A shared_ptr to the Buffer object (must be an INDEX_BUFFER).
     * @throws std::invalid_argument if buffer is invalid.
     */
    void setIndexBuffer(const std::shared_ptr<Buffer>& indexBuffer);

    // --- Getters ---

    /**
     * @brief Returns the OpenGL VAO ID.
     * @return The OpenGL VAO ID, or 0 if not initialized.
     */
    [[nodiscard]] GLuint getId() const noexcept { return m_id; }

    /**
     * @brief Returns true if the VAO has been initialized.
     * @return true if initialized, false otherwise.
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /**
     * @brief Returns the number of vertex buffers associated with this VAO.
     * @return The count of vertex buffers.
     */
    [[nodiscard]] size_t getVertexBufferCount() const noexcept { return m_vertexBuffers.size(); }

    /**
     * @brief Returns whether this VAO has an associated index buffer.
     * @return true if an index buffer is set, false otherwise.
     */
    [[nodiscard]] bool hasIndexBuffer() const noexcept { return m_indexBuffer != nullptr; }

    /**
     * @brief Reserves space for vertex buffers to avoid reallocations.
     * @param count Expected number of vertex buffers.
     */
    void reserveVertexBuffers(size_t count);
};