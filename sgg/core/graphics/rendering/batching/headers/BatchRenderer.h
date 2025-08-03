#pragma once

#include "core/graphics/rendering/core/headers/Buffer.h"
#include "core/graphics/rendering/core/headers/VertexArray.h"
#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>
#include <glm/glm.hpp>

/**
 * @brief Structure representing a single vertex with position, color, and texture coordinates.
 */
struct VertexData {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
};

/**
 * @brief Provides efficient rendering of primitives by batching draw calls.
 *
 * Collects vertex and index data over several submissions and sends them to the GPU
 * in larger chunks to reduce draw call overhead. Designed for rendering large numbers
 * of simple geometries (sprites, lines, etc.).
 */
class BatchRenderer {
public:
    /**
     * @brief Constructs a BatchRenderer.
     *
     * @param maxVertexCount The maximum number of vertices to batch.
     * @param maxIndexCount The maximum number of indices to batch.
     */
    BatchRenderer(size_t maxVertexCount, size_t maxIndexCount);

    ~BatchRenderer() = default;

    // Non-copyable, non-movable for GPU resource safety
    BatchRenderer(const BatchRenderer&) = delete;
    BatchRenderer& operator=(const BatchRenderer&) = delete;
    BatchRenderer(BatchRenderer&&) = delete;
    BatchRenderer& operator=(BatchRenderer&&) = delete;

    // --- Batching Lifecycle ---

    /**
     * @brief Begins a new batching sequence.
     * Must be called before adding any data.
     */
    void begin();

    /**
     * @brief Ends the current batching sequence and flushes any remaining data.
     */
    void end();

    /**
     * @brief Flushes accumulated data to GPU and issues a draw call.
     * Can be called manually mid-batch if needed.
     */
    void flush();

    // --- Adding Data ---

    /**
     * @brief Adds vertex data to the current batch.
     * Automatically flushes if buffer is full.
     */
    void addVertexData(const VertexData* data, size_t vertexCount);

    /**
     * @brief Adds index data to the current batch.
     * Indices are automatically offset for the current batch.
     * Automatically flushes if buffer is full.
     */
    void addIndexData(const uint32_t* data, size_t indexCount);

    /**
     * @brief Convenience method to add a textured quad to the batch.
     */
    void addQuad(float x, float y, float width, float height,
                 const glm::vec4& color, const glm::vec2* texCoords = nullptr);

    /**
     * @brief Convenience method to add a single colored triangle to the batch.
     *
     * @param x1,y1 First vertex position in screen space.
     * @param x2,y2 Second vertex position.
     * @param x3,y3 Third vertex position.
     * @param color Vertex color (same for all vertices). Use brush fill color/opacity.
     * @param texCoords Optional array of 3 texture coordinates.
     */
    void addTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                     const glm::vec4& color, const glm::vec2* texCoords = nullptr);

    // --- Getters for debugging/stats ---
    [[nodiscard]] size_t getCurrentVertexCount() const noexcept { return m_vertexBuffer.size(); }
    [[nodiscard]] size_t getCurrentIndexCount() const noexcept { return m_indexBuffer.size(); }
    [[nodiscard]] size_t getMaxVertexCount() const noexcept { return m_maxVertexCount; }
    [[nodiscard]] size_t getMaxIndexCount() const noexcept { return m_maxIndexCount; }
    [[nodiscard]] bool isDrawing() const noexcept { return m_drawing; }

private:
    // GPU resources
    std::shared_ptr<VertexArray> m_vao;
    std::shared_ptr<Buffer> m_vertexGpuBuffer;
    std::shared_ptr<Buffer> m_indexGpuBuffer;

    // Configuration
    const size_t m_maxVertexCount;
    const size_t m_maxIndexCount;

    // CPU-side buffers (pre-allocated for performance)
    std::vector<VertexData> m_vertexBuffer;
    std::vector<uint32_t> m_indexBuffer;

    // State
    bool m_drawing = false;

    // Helper methods
    void resetBatch() noexcept;
    void initializeGpuResources();
    void validateDrawingState() const;
    bool hasSpaceFor(size_t vertexCount, size_t indexCount) const noexcept;
};