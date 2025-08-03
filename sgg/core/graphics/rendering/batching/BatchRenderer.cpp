#include "core/graphics/rendering/batching/headers/BatchRenderer.h"
#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include <algorithm>
#include <cstddef>

// Constructor
BatchRenderer::BatchRenderer(size_t maxVertexCount, size_t maxIndexCount)
    : m_maxVertexCount(maxVertexCount),
      m_maxIndexCount(maxIndexCount)
{
    // Pre-allocate CPU buffers to avoid reallocations during rendering
    m_vertexBuffer.reserve(maxVertexCount);
    m_indexBuffer.reserve(maxIndexCount);

    initializeGpuResources();
}

void BatchRenderer::begin() {
    if (m_drawing) [[unlikely]] {
        throw std::runtime_error("BatchRenderer::begin() called while already drawing");
    }
    m_drawing = true;
    resetBatch();
}

void BatchRenderer::end() {
    if (!m_drawing) [[unlikely]] {
        throw std::runtime_error("BatchRenderer::end() called without begin()");
    }

    // Flush any remaining data
    if (!m_vertexBuffer.empty() && !m_indexBuffer.empty()) {
        flush();
    }
    m_drawing = false;
}

void BatchRenderer::flush() {
    validateDrawingState();

    if (m_vertexBuffer.empty() || m_indexBuffer.empty()) {
        return; // Nothing to draw
    }

    m_vao->bind();

    // Upload vertex data - single upload per flush
    m_vertexGpuBuffer->bind();
    const size_t vertexDataSize = m_vertexBuffer.size() * sizeof(VertexData);
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, vertexDataSize, m_vertexBuffer.data()));

    // Upload index data - single upload per flush
    m_indexGpuBuffer->bind();
    const size_t indexDataSize = m_indexBuffer.size() * sizeof(uint32_t);
    GL_CHECK(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexDataSize, m_indexBuffer.data()));

    // Issue draw call
    GL_CHECK(glDrawElements(GL_TRIANGLES,
                           static_cast<GLsizei>(m_indexBuffer.size()),
                           GL_UNSIGNED_INT,
                           nullptr));

    resetBatch();
}

void BatchRenderer::addVertexData(const VertexData* data, size_t vertexCount) {
    validateDrawingState();

    if (!data || vertexCount == 0) [[unlikely]] {
        throw std::invalid_argument("Invalid vertex data or zero count");
    }

    // Check if we need to flush first
    if (m_vertexBuffer.size() + vertexCount > m_maxVertexCount) {
        flush();
        if (vertexCount > m_maxVertexCount) [[unlikely]] {
            throw std::runtime_error("Vertex data exceeds maximum batch capacity");
        }
    }

    // Batch copy vertices
    m_vertexBuffer.insert(m_vertexBuffer.end(), data, data + vertexCount);
}

void BatchRenderer::addIndexData(const uint32_t* data, size_t indexCount) {
    validateDrawingState();

    if (!data || indexCount == 0) [[unlikely]] {
        throw std::invalid_argument("Invalid index data or zero count");
    }

    // Check if we need to flush first
    if (m_indexBuffer.size() + indexCount > m_maxIndexCount) {
        flush();
        if (indexCount > m_maxIndexCount) [[unlikely]] {
            throw std::runtime_error("Index data exceeds maximum batch capacity");
        }
    }

    // Add indices with current vertex base offset
    const uint32_t baseVertex = static_cast<uint32_t>(m_vertexBuffer.size());
    m_indexBuffer.reserve(m_indexBuffer.size() + indexCount);

    for (size_t i = 0; i < indexCount; ++i) {
        m_indexBuffer.push_back(data[i] + baseVertex);
    }
}

void BatchRenderer::addQuad(float x, float y, float width, float height,
                           const glm::vec4& color, const glm::vec2* texCoords) {
    validateDrawingState();

    // Check if we have space for a quad (4 vertices, 6 indices)
    if (!hasSpaceFor(4, 6)) {
        flush();
    }

    // Default texture coordinates (optimized with constexpr)
    static glm::vec2 defaultTexCoords[4] = {
        {0.0f, 1.0f}, // Top-Left
        {1.0f, 1.0f}, // Top-Right
        {1.0f, 0.0f}, // Bottom-Right
        {0.0f, 0.0f}  // Bottom-Left
    };

    const glm::vec2* tc = texCoords ? texCoords : defaultTexCoords;
    const uint32_t baseIndex = static_cast<uint32_t>(m_vertexBuffer.size());

    // Add vertices directly to buffer
    m_vertexBuffer.emplace_back(VertexData{{x, y + height, 0.0f}, color, tc[0]});         // TL
    m_vertexBuffer.emplace_back(VertexData{{x + width, y + height, 0.0f}, color, tc[1]}); // TR
    m_vertexBuffer.emplace_back(VertexData{{x + width, y, 0.0f}, color, tc[2]});          // BR
    m_vertexBuffer.emplace_back(VertexData{{x, y, 0.0f}, color, tc[3]});                  // BL

    // Add indices for two triangles (optimized batch insertion)
    static constexpr uint32_t quadIndices[6] = {0, 1, 2, 2, 3, 0};
    m_indexBuffer.reserve(m_indexBuffer.size() + 6);

    for (uint32_t idx : quadIndices) {
        m_indexBuffer.push_back(baseIndex + idx);
    }
}

// Convenience: add single triangle (3 vertices, 3 indices)
void BatchRenderer::addTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                                const glm::vec4& color, const glm::vec2* texCoords) {
    validateDrawingState();

    // Check capacity (3 vertices, 3 indices)
    if (!hasSpaceFor(3, 3)) {
        flush();
    }

    // Fallback UVs covering full texture top half (arbitrary but consistent)
    static glm::vec2 defaultTex[3] = {
        {0.0f, 1.0f}, // V1
        {1.0f, 1.0f}, // V2
        {0.5f, 0.0f}  // V3
    };

    const glm::vec2* tc = texCoords ? texCoords : defaultTex;

    const uint32_t baseIndex = static_cast<uint32_t>(m_vertexBuffer.size());

    // Push vertices
    m_vertexBuffer.emplace_back(VertexData{{x1, y1, 0.0f}, color, tc[0]});
    m_vertexBuffer.emplace_back(VertexData{{x2, y2, 0.0f}, color, tc[1]});
    m_vertexBuffer.emplace_back(VertexData{{x3, y3, 0.0f}, color, tc[2]});

    // Push indices forming the triangle
    m_indexBuffer.reserve(m_indexBuffer.size() + 3);
    m_indexBuffer.push_back(baseIndex + 0);
    m_indexBuffer.push_back(baseIndex + 1);
    m_indexBuffer.push_back(baseIndex + 2);
}

// --- Private Helper Methods ---

void BatchRenderer::resetBatch() noexcept {
    m_vertexBuffer.clear();
    m_indexBuffer.clear();
    // Vectors maintain their reserved capacity
}

void BatchRenderer::initializeGpuResources() {
    // Create and initialize VAO
    m_vao = std::make_shared<VertexArray>();
    m_vao->init();

    // Create vertex buffer with full capacity upfront
    m_vertexGpuBuffer = std::make_shared<Buffer>(BufferType::Vertex, BufferUsage::DynamicDraw);
    const GLsizeiptr vertexBufferSize = static_cast<GLsizeiptr>(m_maxVertexCount * sizeof(VertexData));
    m_vertexGpuBuffer->init(nullptr, vertexBufferSize);

    const std::vector<VertexAttribute> attributes = {
        VertexAttribute::position(0, sizeof(VertexData), offsetof(VertexData, position)),
        VertexAttribute::color(1, sizeof(VertexData), offsetof(VertexData, color)),
        VertexAttribute::texCoord(2, sizeof(VertexData), offsetof(VertexData, texCoord))
    };

    m_vao->addVertexBuffer(m_vertexGpuBuffer, attributes);

    // Create index buffer with full capacity upfront
    m_indexGpuBuffer = std::make_shared<Buffer>(BufferType::Index, BufferUsage::DynamicDraw);
    const GLsizeiptr indexBufferSize = static_cast<GLsizeiptr>(m_maxIndexCount * sizeof(uint32_t));
    m_indexGpuBuffer->init(nullptr, indexBufferSize);

    m_vao->setIndexBuffer(m_indexGpuBuffer);
    m_vao->unbind();
}

void BatchRenderer::validateDrawingState() const {
    if (!m_drawing) [[unlikely]] {
        throw std::runtime_error("Operation called outside begin()/end() block");
    }
}

bool BatchRenderer::hasSpaceFor(size_t vertexCount, size_t indexCount) const noexcept {
    return (m_vertexBuffer.size() + vertexCount <= m_maxVertexCount) &&
           (m_indexBuffer.size() + indexCount <= m_maxIndexCount);
}