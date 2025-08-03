#pragma once

#include "core/graphics/rendering/core/headers/Buffer.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <list>
#include <cstddef>
#include <functional>

/**
 * @brief High-performance pool for reusable Buffer objects with smart allocation strategies.
 *
 * Features:
 * - Hash-based lookup for O(1) buffer matching
 * - LRU eviction policy for optimal memory usage
 * - Size-based buffer reuse with growth tolerance
 * - Automatic cleanup and optimization
 * - Detailed statistics and monitoring
 */
class BufferPool {
public:
    /**
     * @brief Configuration for pool behavior and performance tuning.
     */
    struct Config {
        size_t maxPoolSize = 256;           ///< Maximum buffers in pool
        float sizeGrowthTolerance = 1.5f;   ///< Allow buffers up to 1.5x requested size
        bool enableLRU = true;              ///< Use LRU eviction policy
        bool enableStats = false;           ///< Track detailed statistics
        size_t cleanupThreshold = 50;       ///< Auto-cleanup when pool exceeds this size

        static Config performance() {
            return {512, 2.0f, true, false, 100};
        }

        static Config memory() {
            return {128, 1.2f, true, true, 32};
        }

        static Config minimal() {
            return {64, 1.1f, false, false, 16};
        }
    };

    /**
     * @brief Statistics for monitoring pool performance.
     */
    struct Stats {
        size_t totalBuffers = 0;
        size_t activeBuffers = 0;
        size_t availableBuffers = 0;
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
        size_t createdBuffers = 0;
        size_t unpooledBuffers = 0;
        size_t totalMemoryBytes = 0;

        [[nodiscard]] float hitRatio() const {
            const size_t total = cacheHits + cacheMisses;
            return total > 0 ? static_cast<float>(cacheHits) / total : 0.0f;
        }
    };

private:
    /**
     * @brief Internal buffer entry with LRU support.
     */
    struct BufferEntry {
        std::shared_ptr<Buffer> buffer;
        GLsizeiptr allocatedSize = 0;
        BufferType type = BufferType::Vertex;
        BufferUsage usage = BufferUsage::StaticDraw;
        bool inUse = false;

        // LRU support
        mutable std::list<size_t>::iterator lruIterator;

        BufferEntry() = default;
        BufferEntry(std::shared_ptr<Buffer> buf, GLsizeiptr size, BufferType t, BufferUsage u)
            : buffer(std::move(buf)), allocatedSize(size), type(t), usage(u) {}
    };

    /**
     * @brief Hash key for fast buffer lookup.
     */
    struct BufferKey {
        BufferType type;
        BufferUsage usage;
        GLsizeiptr sizeCategory; // Quantized size for better reuse

        bool operator==(const BufferKey& other) const noexcept {
            return type == other.type && usage == other.usage && sizeCategory == other.sizeCategory;
        }
    };

    struct BufferKeyHash {
        size_t operator()(const BufferKey& key) const noexcept {
            return std::hash<int>{}(static_cast<int>(key.type)) ^
                   (std::hash<int>{}(static_cast<int>(key.usage)) << 1) ^
                   (std::hash<GLsizeiptr>{}(key.sizeCategory) << 2);
        }
    };

    // Core data structures
    std::vector<BufferEntry> m_pool;
    std::unordered_map<BufferKey, std::vector<size_t>, BufferKeyHash> m_hashMap;
    std::list<size_t> m_lruList;  // Most recently used at front

    Config m_config;
    mutable Stats m_stats;

    // Helper methods
    [[nodiscard]] BufferKey createKey(BufferType type, BufferUsage usage, GLsizeiptr size) const;
    [[nodiscard]] GLsizeiptr quantizeSize(GLsizeiptr size) const;
    [[nodiscard]] size_t findBestMatch(const BufferKey& key, GLsizeiptr requestedSize);

    void updateLRU(size_t index);
    void evictLRU();
    size_t createNewBuffer(BufferType type, BufferUsage usage, GLsizeiptr size);
    void addToHashMap(size_t index);
    void removeFromHashMap(size_t index);
    void updateStats(bool hit, bool created = false, bool unpooled = false) const;

public:
    /**
     * @brief Constructs a BufferPool with default configuration.
     */
    BufferPool();

    /**
     * @brief Constructs a BufferPool with specified configuration.
     */
    explicit BufferPool(const Config& config);

    /**
     * @brief Legacy constructor for backward compatibility.
     */
    [[deprecated("Use Config-based constructor instead")]]
    explicit BufferPool(size_t maxPoolSize);

    ~BufferPool() = default;

    // Non-copyable, movable
    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;
    BufferPool(BufferPool&&) noexcept = default;
    BufferPool& operator=(BufferPool&&) noexcept = default;

    // --- Core API ---

    /**
     * @brief Acquires a buffer from pool or creates new one.
     *
     * Uses intelligent matching to find the best available buffer,
     * considering size tolerance and usage patterns.
     */
    [[nodiscard]] std::shared_ptr<Buffer> acquireBuffer(BufferType type, BufferUsage usage, GLsizeiptr size);

    /**
     * @brief Returns a buffer to the pool for reuse.
     *
     * Automatically handles LRU updating and availability tracking.
     */
    void returnBuffer(std::shared_ptr<Buffer> buffer);

    /**
     * @brief Removes unused buffers and optimizes pool structure.
     *
     * Uses configurable cleanup strategies for optimal performance.
     */
    void cleanup();

    /**
     * @brief Aggressive cleanup that removes all unused buffers.
     */
    void clear();

    /**
     * @brief Optimizes pool structure and rebuilds hash maps.
     */
    void optimize();

    // --- Statistics and Monitoring ---

    [[nodiscard]] Stats getStats() const { return m_stats; }
    [[nodiscard]] size_t getCurrentPoolSize() const { return m_pool.size(); }
    [[nodiscard]] size_t getMaxPoolSize() const { return m_config.maxPoolSize; }
    [[nodiscard]] size_t getActiveBufferCount() const;
    [[nodiscard]] size_t getAvailableBufferCount() const;
    [[nodiscard]] size_t getTotalMemoryUsage() const;

    /**
     * @brief Resets all statistics counters.
     */
    void resetStats();

    /**
     * @brief Updates pool configuration (some changes require optimization).
     */
    void updateConfig(const Config& newConfig);

    // --- Advanced Features ---

    /**
     * @brief Pre-allocates buffers for expected usage patterns.
     */
    void preallocate(BufferType type, BufferUsage usage, GLsizeiptr size, size_t count);

    /**
     * @brief Reserves pool capacity to avoid reallocations.
     */
    void reserve(size_t capacity);

    /**
     * @brief Shrinks pool capacity to current size.
     */
    void shrinkToFit();
};