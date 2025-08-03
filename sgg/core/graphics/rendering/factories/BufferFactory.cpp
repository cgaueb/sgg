#include "core/graphics/rendering/factories/headers/BufferFactory.h"
#include <stdexcept>
#include <algorithm>
#include <optional>

// --- Constructors ---

BufferFactory::BufferFactory(const Config& config)
    : m_config(config)
    , m_nextId(config.initialId) {

    if (m_config.usePool) {
        m_pool = std::make_unique<BufferPool>(m_config.poolSize);
    }
}

BufferFactory::BufferFactory(bool usePool, size_t poolSize)
    : BufferFactory(Config{usePool, poolSize, false, 1}) {
    // Delegating constructor for backward compatibility
}

BufferFactory::BufferFactory() : BufferFactory(Config{}) {}

// --- Private Helpers ---

std::shared_ptr<Buffer> BufferFactory::findBuffer(BufferId id) const {
    const auto it = m_buffers.find(id);
    return (it != m_buffers.end()) ? it->second : nullptr;
}

BufferId BufferFactory::generateId() {
    uint32_t id = m_nextId.fetch_add(1, std::memory_order_relaxed);

    // Handle wrap-around (extremely unlikely but safe)
    if (id == 0) {
        throw std::runtime_error("Buffer ID counter overflow");
    }

    return BufferId{id};
}

void BufferFactory::validateId(BufferId id) const {
    if (!id) {
        throw std::invalid_argument("Invalid BufferId (0)");
    }
}

// --- High-level Buffer API ---

BufferId BufferFactory::createBuffer(BufferType type, BufferUsage usage,
                                    const void* data, GLsizeiptr size) {
    if (size < 0) {
        throw std::invalid_argument("Buffer size cannot be negative");
    }

    return withLock([&]() {
        std::shared_ptr<Buffer> buffer;

        if (m_config.usePool && m_pool) {
            buffer = m_pool->acquireBuffer(type, usage, size);
            if (buffer && data) {
                buffer->updateData(data, size, 0);
            }
        } else {
            buffer = std::make_shared<Buffer>(type, usage);
            if (size > 0) {
                buffer->setData(data, size);
            }
        }

        if (!buffer) {
            throw std::runtime_error("Failed to create buffer");
        }

        const BufferId id = generateId();
        m_buffers.emplace(id, std::move(buffer));
        return id;
    });
}

BufferId BufferFactory::createBuffer(BufferType type, BufferUsage usage, GLsizeiptr size) {
    return createBuffer(type, usage, nullptr, size);
}

BufferId BufferFactory::createVertexBuffer(const void* vertices, GLsizeiptr size,
                                          BufferUsage usage) {
    return createBuffer(BufferType::Vertex, usage, vertices, size);
}

BufferId BufferFactory::createIndexBuffer(const void* indices, GLsizeiptr size,
                                         BufferUsage usage) {
    return createBuffer(BufferType::Index, usage, indices, size);
}

void BufferFactory::updateBuffer(BufferId id, const void* data, GLsizeiptr size, GLintptr offset) {
    validateId(id);

    withLock([&]() {
        auto buffer = findBuffer(id);
        if (!buffer) {
            throw std::runtime_error("Buffer not found: " + std::to_string(id.value));
        }
        buffer->updateData(data, size, offset);
    });
}

void BufferFactory::bindBuffer(BufferId id) {
    validateId(id);

    withLock([&]() {
        auto buffer = findBuffer(id);
        if (!buffer) {
            throw std::runtime_error("Buffer not found: " + std::to_string(id.value));
        }
        buffer->bind();
    });
}

void BufferFactory::unbindBuffer(BufferType type) noexcept {
    // Unbinding doesn't require locking as it's stateless
    glBindBuffer(toGlBufferTarget(type), 0);
}

void BufferFactory::deleteBuffer(BufferId id) {
    validateId(id);

    withLock([&]() {
        auto it = m_buffers.find(id);
        if (it == m_buffers.end()) {
            throw std::runtime_error("Buffer not found: " + std::to_string(id.value));
        }

        auto buffer = std::move(it->second);
        m_buffers.erase(it);

        // Return to pool if using pooling
        if (m_config.usePool && m_pool) {
            m_pool->returnBuffer(std::move(buffer));
        }
        // Otherwise, buffer destructor handles cleanup
    });
}

void BufferFactory::deleteBuffers(const std::vector<BufferId>& ids) {
    if (ids.empty()) return;

    withLock([&]() {
        std::vector<std::shared_ptr<Buffer>> buffersToReturn;
        buffersToReturn.reserve(ids.size());

        for (BufferId id : ids) {
            if (!id) continue; // Skip invalid IDs

            auto it = m_buffers.find(id);
            if (it != m_buffers.end()) {
                buffersToReturn.emplace_back(std::move(it->second));
                m_buffers.erase(it);
            }
        }

        // Batch return to pool
        if (m_config.usePool && m_pool) {
            for (auto& buffer : buffersToReturn) {
                m_pool->returnBuffer(std::move(buffer));
            }
        }
    });
}

// --- Advanced Access ---

std::shared_ptr<Buffer> BufferFactory::getBuffer(BufferId id) const {
    if (!id) return nullptr;

    return withLock([&]() {
        return findBuffer(id);
    });
}

bool BufferFactory::hasBuffer(BufferId id) const noexcept {
    if (!id) return false;

    return withLock([&]() {
        return m_buffers.find(id) != m_buffers.end();
    });
}

std::optional<BufferFactory::BufferInfo> BufferFactory::getBufferInfo(BufferId id) const {
    if (!id) return std::nullopt;

    return withLock([&]() -> std::optional<BufferInfo> {
        auto buffer = findBuffer(id);
        if (!buffer) return std::nullopt;

        return BufferInfo{
            buffer->getType(),
            buffer->getSize(),
            buffer->isInitialized()
        };
    });
}

// --- Resource Management ---

void BufferFactory::clear() {
    withLock([&]() {
        if (m_config.usePool && m_pool) {
            // Return all buffers to pool before clearing
            for (auto& [id, buffer] : m_buffers) {
                m_pool->returnBuffer(std::move(buffer));
            }
        }

        m_buffers.clear();

        if (m_pool) {
            m_pool->cleanup();
        }
    });
}

void BufferFactory::optimize() {
    withLock([&]() {
        if (m_config.usePool && m_pool) {
            m_pool->cleanup();
        }
    });
}

BufferFactory::Stats BufferFactory::getStats() const {
    return withLock([&]() {
        Stats stats;
        stats.activeBuffers = m_buffers.size();
        stats.nextId = m_nextId.load(std::memory_order_relaxed);

        // Calculate approximate memory usage
        for (const auto& [id, buffer] : m_buffers) {
            if (buffer) {
                stats.totalMemoryUsed += buffer->getSize();
            }
        }
        return stats;
    });
}