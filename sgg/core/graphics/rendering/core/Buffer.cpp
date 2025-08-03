#include "core/graphics/rendering/core/headers/Buffer.h"
#include <utility>
#include <stdexcept>

// --- Constructor / Destructor ---

Buffer::Buffer(BufferType type, BufferUsage usage) noexcept
    : m_target(toGlBufferTarget(type))
    , m_usage(toGlBufferUsage(usage))
    , m_type(type) {
    // Cache GL enums once to avoid repeated conversions
}

Buffer::~Buffer() noexcept {
    clear();
}

// --- Move Semantics ---

Buffer::Buffer(Buffer&& other) noexcept
    : m_id(other.m_id)
    , m_target(other.m_target)
    , m_usage(other.m_usage)
    , m_size(other.m_size)
    , m_type(other.m_type)
    , m_initialized(other.m_initialized) {

    // Reset source object
    other.m_id = 0;
    other.m_size = 0;
    other.m_initialized = false;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        // Clean up current resources
        clear();

        // Transfer ownership
        m_id = other.m_id;
        m_target = other.m_target;
        m_usage = other.m_usage;
        m_size = other.m_size;
        m_type = other.m_type;
        m_initialized = other.m_initialized;

        // Reset source
        other.m_id = 0;
        other.m_size = 0;
        other.m_initialized = false;
    }
    return *this;
}

// --- Private Methods ---

void Buffer::ensureInitialized() {
    if (!m_initialized) {
        GL_CHECK(glGenBuffers(1, &m_id));
        if (m_id == 0) {
            throw std::runtime_error("Failed to generate OpenGL buffer object");
        }
        m_initialized = true;
    }
}

// --- High-level Operations ---

void Buffer::setData(const void* data, GLsizeiptr size) {
    if (size < 0) {
        throw std::invalid_argument("Buffer size cannot be negative");
    }

    ensureInitialized();

    m_size = size;

    // Bind, upload, unbind pattern
    GL_CHECK(glBindBuffer(m_target, m_id));
    GL_CHECK(glBufferData(m_target, size, data, m_usage));
    GL_CHECK(glBindBuffer(m_target, 0));
}

void Buffer::updateData(const void* data, GLsizeiptr size, GLintptr offset) {
    if (size < 0 || offset < 0) {
        throw std::invalid_argument("Size and offset must be non-negative");
    }

    if (!m_initialized) {
        if (offset == 0) {
            // Simple case: treat as setData
            setData(data, size);
            return;
        } else {
            throw std::runtime_error("Cannot update uninitialized buffer with non-zero offset");
        }
    }

    GL_CHECK(glBindBuffer(m_target, m_id));

    const GLsizeiptr requiredSize = offset + size;
    if (requiredSize > m_size) {
        // Need to grow buffer - use orphaning strategy for performance
        const GLsizeiptr newSize = requiredSize;

        // Orphan old buffer and allocate new one
        GL_CHECK(glBufferData(m_target, newSize, nullptr, m_usage));
        m_size = newSize;
    }

    // Update the data
    GL_CHECK(glBufferSubData(m_target, offset, size, data));
    GL_CHECK(glBindBuffer(m_target, 0));
}

void Buffer::reserve(GLsizeiptr size) {
    if (size <= m_size) {
        return; // Already have enough space
    }

    setData(nullptr, size);
}

void Buffer::bind() const noexcept {
    if (m_initialized) {
        glBindBuffer(m_target, m_id);
        // Note: Removed GL_CHECK for performance in hot paths
        // Consider adding debug-only checks if needed
    }
}

void Buffer::unbind() const noexcept {
    glBindBuffer(m_target, 0);
}

void Buffer::clear() noexcept {
    if (m_initialized && m_id != 0) {
        glDeleteBuffers(1, &m_id);
        m_id = 0;
        m_initialized = false;
    }
    m_size = 0;
}