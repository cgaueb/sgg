#include "core/graphics/rendering/core/headers/VertexArray.h"
#include <stdexcept>
#include <utility>

// --- Private Helpers ---

void VertexArray::ensureInitialized() {
    if (!m_initialized) [[unlikely]] {
        init();
    }
}

void VertexArray::cleanup() noexcept {
    if (m_initialized && m_id != 0) {
        // Check if we have a valid OpenGL context before attempting cleanup
        // In a proper OpenGL context, glGetError should not fail
        const GLenum contextCheck = glGetError();
        if (contextCheck == GL_NO_ERROR) {
            glDeleteVertexArrays(1, &m_id);
        }
        m_id = 0;
        m_initialized = false;
    }
}

// --- Constructor / Destructor ---

VertexArray::~VertexArray() {
    cleanup();
}

// --- Move Semantics ---

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_id(std::exchange(other.m_id, 0)),
      m_initialized(std::exchange(other.m_initialized, false)),
      m_vertexBuffers(std::move(other.m_vertexBuffers)),
      m_indexBuffer(std::move(other.m_indexBuffer)) {}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) [[likely]] {
        cleanup();
        
        m_id = std::exchange(other.m_id, 0);
        m_initialized = std::exchange(other.m_initialized, false);
        m_vertexBuffers = std::move(other.m_vertexBuffers);
        m_indexBuffer = std::move(other.m_indexBuffer);
    }
    return *this;
}

// --- OpenGL Operations ---

void VertexArray::init() {
    if (m_initialized) [[likely]] {
        return;
    }

    GL_CHECK(glGenVertexArrays(1, &m_id));
    if (m_id == 0) [[unlikely]] {
        throw std::runtime_error("Failed to generate vertex array object.");
    }
    m_initialized = true;
}

void VertexArray::bind() const {
    if (!m_initialized) [[unlikely]] {
        throw std::runtime_error("Attempting to bind uninitialized VertexArray");
    }
    GL_CHECK(glBindVertexArray(m_id));
}

void VertexArray::unbind() noexcept {
    GL_CHECK(glBindVertexArray(0));
}

void VertexArray::addVertexBuffer(const std::shared_ptr<Buffer>& vertexBuffer,
                                 const std::vector<VertexAttribute>& attributes) {
    if (!vertexBuffer) [[unlikely]] {
        throw std::invalid_argument("Vertex buffer cannot be null");
    }
    if (attributes.empty()) [[unlikely]] {
        throw std::invalid_argument("Vertex attributes must be provided");
    }

    ensureInitialized();

    GL_CHECK(glBindVertexArray(m_id));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->getId()));

    // Configure vertex attributes - using range-based for with const reference
    for (const auto& attr : attributes) {
        GL_CHECK(glEnableVertexAttribArray(attr.location));
        GL_CHECK(glVertexAttribPointer(
            attr.location,
            attr.componentCount,
            attr.componentType,
            attr.normalized,
            attr.stride,
            attr.getOffsetPtr()
        ));
    }

    // Store the buffer to keep it alive
    m_vertexBuffers.emplace_back(vertexBuffer);

    // Unbind VBO after configuring attributes
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexArray::setIndexBuffer(const std::shared_ptr<Buffer>& indexBuffer) {
    if (!indexBuffer) [[unlikely]] {
        throw std::invalid_argument("Index buffer cannot be null");
    }

    ensureInitialized();

    GL_CHECK(glBindVertexArray(m_id));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getId()));

    m_indexBuffer = indexBuffer;
}

void VertexArray::reserveVertexBuffers(size_t count) {
    m_vertexBuffers.reserve(count);
}