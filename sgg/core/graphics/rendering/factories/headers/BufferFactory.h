#pragma once

#include "core/graphics/rendering/core/headers/Buffer.h"
#include "core/graphics/rendering/factories/headers/BufferPool.h"
#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <optional>

/**
 * @brief High-performance factory for creating and managing GPU buffer objects.
 *
 * Provides efficient buffer lifecycle management with optional pooling,
 * thread-safe operations, and automatic resource cleanup.
 */
class BufferFactory {
public:
    /**
     * @brief Configuration for buffer factory behavior.
     */
    struct Config {
        bool usePool = true;           ///< Enable buffer pooling for performance
        size_t poolSize = 256;         ///< Maximum buffers in pool
        bool threadSafe = false;       ///< Enable thread-safe operations
        uint32_t initialId = 1;        ///< Starting ID for buffer allocation

        static Config performance() {
            return {true, 512, false, 1};  // High-performance defaults
        }

        static Config threadsafe() {
            return {true, 256, true, 1};   // Thread-safe defaults
        }

        static Config minimal() {
            return {false, 0, false, 1};   // No pooling, minimal overhead
        }
    };

    /**
     * @brief Constructs a BufferFactory with default configuration.
     */
    BufferFactory();

    /**
     * @brief Constructs a BufferFactory with specified configuration.
     */
    explicit BufferFactory(const Config& config);

    /**
     * @brief Legacy constructor for backward compatibility.
     */
    [[deprecated("Use Config-based constructor instead")]]
    explicit BufferFactory(bool usePool, size_t poolSize = 100);

    ~BufferFactory() = default;

    // Non-copyable, non-movable (manages unique state)
    BufferFactory(const BufferFactory&) = delete;
    BufferFactory& operator=(const BufferFactory&) = delete;
    BufferFactory(BufferFactory&&) = delete;
    BufferFactory& operator=(BufferFactory&&) = delete;

    // --- High-level Buffer API ---

    /**
     * @brief Creates a buffer with data upload.
     */
    [[nodiscard]] BufferId createBuffer(BufferType type, BufferUsage usage,
                                       const void* data, GLsizeiptr size);

    /**
     * @brief Creates an empty buffer with reserved space.
     */
    [[nodiscard]] BufferId createBuffer(BufferType type, BufferUsage usage, GLsizeiptr size);

    /**
     * @brief Creates a buffer optimized for vertex data.
     */
    [[nodiscard]] BufferId createVertexBuffer(const void* vertices, GLsizeiptr size,
                                             BufferUsage usage = BufferUsage::StaticDraw);

    /**
     * @brief Creates a buffer optimized for index data.
     */
    [[nodiscard]] BufferId createIndexBuffer(const void* indices, GLsizeiptr size,
                                            BufferUsage usage = BufferUsage::StaticDraw);

    /**
     * @brief Updates buffer data efficiently.
     */
    void updateBuffer(BufferId id, const void* data, GLsizeiptr size, GLintptr offset = 0);

    /**
     * @brief Binds buffer to its OpenGL target.
     */
    void bindBuffer(BufferId id);

    /**
     * @brief Unbinds buffer target.
     */
    void unbindBuffer(BufferType type) noexcept;

    /**
     * @brief Removes buffer and frees resources.
     */
    void deleteBuffer(BufferId id);

    /**
     * @brief Batch delete multiple buffers efficiently.
     */
    void deleteBuffers(const std::vector<BufferId>& ids);

    // --- Advanced Access ---

    /**
     * @brief Gets buffer for advanced operations (use with caution).
     */
    [[nodiscard]] std::shared_ptr<Buffer> getBuffer(BufferId id) const;

    /**
     * @brief Checks if buffer exists and is valid.
     */
    [[nodiscard]] bool hasBuffer(BufferId id) const noexcept;

    /**
     * @brief Gets buffer info without full buffer access.
     */
    struct BufferInfo {
        BufferType type;
        GLsizeiptr size;
        bool isInitialized;
    };
    [[nodiscard]] std::optional<BufferInfo> getBufferInfo(BufferId id) const;

    // --- Resource Management ---

    /**
     * @brief Clears all buffers and resets factory state.
     */
    void clear();

    /**
     * @brief Optimizes pool and cleans up unused resources.
     */
    void optimize();

    /**
     * @brief Gets current resource usage statistics.
     */
    struct Stats {
        size_t activeBuffers = 0;
        size_t pooledBuffers = 0;
        size_t totalMemoryUsed = 0;  // Approximate
        uint32_t nextId = 1;
    };
    [[nodiscard]] Stats getStats() const;

private:
    // Core data
    std::unordered_map<BufferId, std::shared_ptr<Buffer>> m_buffers;
    std::unique_ptr<BufferPool> m_pool;

    // Configuration
    Config m_config;
    std::atomic<uint32_t> m_nextId;

    // Thread safety
    mutable std::mutex m_mutex;

    // Helper methods
    [[nodiscard]] std::shared_ptr<Buffer> findBuffer(BufferId id) const;
    [[nodiscard]] BufferId generateId();
    void validateId(BufferId id) const;

    template<typename Func>
    auto withLock(Func&& func) const -> decltype(func()) {
        if (m_config.threadSafe) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return func();
        }
        return func();
    }
};