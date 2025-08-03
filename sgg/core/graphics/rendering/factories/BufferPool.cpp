#include "core/graphics/rendering/factories/headers/BufferPool.h"
#include <algorithm>
#include <cmath>
#include <cassert>

// Default constructor delegating to config-based constructor with default values
BufferPool::BufferPool() : BufferPool(Config{}) {}

// --- Constructors ---

BufferPool::BufferPool(const Config& config) : m_config(config) {
    m_pool.reserve(config.maxPoolSize);
    m_stats = {}; // Initialize stats
}

BufferPool::BufferPool(size_t maxPoolSize)
    : BufferPool(Config{maxPoolSize, 1.5f, true, false, maxPoolSize / 4}) {
}

// --- Private Helper Methods ---

BufferPool::BufferKey BufferPool::createKey(BufferType type, BufferUsage usage, GLsizeiptr size) const {
    return {type, usage, quantizeSize(size)};
}

GLsizeiptr BufferPool::quantizeSize(GLsizeiptr size) const {
    // Quantize sizes to power-of-2 boundaries for better reuse
    if (size <= 0) return 0;

    // Round up to next power of 2, but with some granularity
    const GLsizeiptr minSize = 1024; // 1KB minimum
    if (size < minSize) return minSize;

    // For larger sizes, use progressive quantization
    if (size < 64 * 1024) { // < 64KB: 4KB boundaries
        return ((size + 4095) / 4096) * 4096;
    } else if (size < 1024 * 1024) { // < 1MB: 64KB boundaries
        return ((size + 65535) / 65536) * 65536;
    } else { // >= 1MB: 256KB boundaries
        return ((size + 262143) / 262144) * 262144;
    }
}

size_t BufferPool::findBestMatch(const BufferKey& key, GLsizeiptr requestedSize) {
    auto hashIt = m_hashMap.find(key);
    if (hashIt == m_hashMap.end()) {
        return SIZE_MAX; // Not found
    }

    size_t bestIndex = SIZE_MAX;
    GLsizeiptr bestSize = 0;
    const GLsizeiptr maxAcceptableSize = static_cast<GLsizeiptr>(
        requestedSize * m_config.sizeGrowthTolerance);

    // Find the smallest buffer that fits within our tolerance
    for (size_t index : hashIt->second) {
        const BufferEntry& entry = m_pool[index];

        if (entry.inUse || !entry.buffer || !entry.buffer->isInitialized()) {
            continue;
        }

        const GLsizeiptr bufferSize = entry.buffer->getSize();
        if (bufferSize >= requestedSize && bufferSize <= maxAcceptableSize) {
            if (bestIndex == SIZE_MAX || bufferSize < bestSize) {
                bestIndex = index;
                bestSize = bufferSize;
            }
        }
    }

    return bestIndex;
}

void BufferPool::updateLRU(size_t index) {
    if (!m_config.enableLRU || index >= m_pool.size()) return;

    BufferEntry& entry = m_pool[index];

    // Remove from current position if it exists in the list
    if (entry.lruIterator != m_lruList.end()) {
        m_lruList.erase(entry.lruIterator);
    }

    // Add to front (most recently used)
    m_lruList.push_front(index);
    entry.lruIterator = m_lruList.begin();
}

void BufferPool::evictLRU() {
    if (!m_config.enableLRU || m_lruList.empty()) return;

    // Find LRU unused buffer from the back of the list
    for (auto it = m_lruList.rbegin(); it != m_lruList.rend(); ++it) {
        size_t index = *it;
        if (index < m_pool.size() && !m_pool[index].inUse) {
            removeFromHashMap(index);

            // Invalidate the iterator for the buffer being removed
            m_pool[index].lruIterator = m_lruList.end();

            // Move last element to this position to avoid gaps
            if (index != m_pool.size() - 1) {
                m_pool[index] = std::move(m_pool.back());
                // Update hash map for moved element
                addToHashMap(index);
                // Update LRU iterator for moved element
                if (m_config.enableLRU && m_pool[index].lruIterator != m_lruList.end()) {
                    *m_pool[index].lruIterator = index;
                }
            }

            m_pool.pop_back();

            // Remove from LRU list - convert reverse iterator to forward iterator
            auto forwardIt = std::next(it).base();
            m_lruList.erase(forwardIt);
            return;
        }
    }
}

size_t BufferPool::createNewBuffer(BufferType type, BufferUsage usage, GLsizeiptr size) {
    if (m_pool.size() >= m_config.maxPoolSize) {
        evictLRU();

        if (m_pool.size() >= m_config.maxPoolSize) {
            return SIZE_MAX; // Pool still full after eviction
        }
    }

    auto buffer = std::make_shared<Buffer>(type, usage);
    buffer->setData(nullptr, size); // Pre-allocate

    size_t index = m_pool.size();
    m_pool.emplace_back(std::move(buffer), size, type, usage);

    addToHashMap(index);
    updateStats(false, true);

    return index;
}

void BufferPool::addToHashMap(size_t index) {
    if (index >= m_pool.size()) return;

    const BufferEntry& entry = m_pool[index];
    BufferKey key = createKey(entry.type, entry.usage, entry.allocatedSize);
    m_hashMap[key].push_back(index);
}

void BufferPool::removeFromHashMap(size_t index) {
    if (index >= m_pool.size()) return;

    const BufferEntry& entry = m_pool[index];
    BufferKey key = createKey(entry.type, entry.usage, entry.allocatedSize);

    auto hashIt = m_hashMap.find(key);
    if (hashIt != m_hashMap.end()) {
        auto& indices = hashIt->second;
        indices.erase(std::remove(indices.begin(), indices.end(), index), indices.end());

        if (indices.empty()) {
            m_hashMap.erase(hashIt);
        }
    }
}

void BufferPool::updateStats(bool hit, bool created, bool unpooled) const {
    if (!m_config.enableStats) return;

    if (hit) {
        ++m_stats.cacheHits;
    } else {
        ++m_stats.cacheMisses;
    }

    if (created) {
        ++m_stats.createdBuffers;
    }

    if (unpooled) {
        ++m_stats.unpooledBuffers;
    }
}

// --- Core API ---

std::shared_ptr<Buffer> BufferPool::acquireBuffer(BufferType type, BufferUsage usage, GLsizeiptr size) {
    if (size <= 0) {
        return nullptr;
    }

    // Try to find existing buffer
    BufferKey key = createKey(type, usage, size);
    size_t index = findBestMatch(key, size);

    if (index != SIZE_MAX) {
        // Found suitable buffer
        BufferEntry& entry = m_pool[index];
        entry.inUse = true;
        updateLRU(index);
        updateStats(true);

        return entry.buffer;
    }

    // Try to create new buffer in pool
    index = createNewBuffer(type, usage, size);
    if (index != SIZE_MAX) {
        BufferEntry& entry = m_pool[index];
        entry.inUse = true;
        updateLRU(index);

        return entry.buffer;
    }

    // Pool is full, create unpooled buffer
    updateStats(false, true, true);
    auto buffer = std::make_shared<Buffer>(type, usage);
    buffer->setData(nullptr, size);

    return buffer;
}

void BufferPool::returnBuffer(std::shared_ptr<Buffer> buffer) {
    if (!buffer) return;

    // Find buffer in pool
    for (size_t i = 0; i < m_pool.size(); ++i) {
        BufferEntry& entry = m_pool[i];
        if (entry.buffer == buffer) {
            entry.inUse = false;
            entry.allocatedSize = buffer->getSize(); // Update size if changed
            updateLRU(i);
            return;
        }
    }

    // Buffer not found in pool (was unpooled)
    // It will be automatically destroyed when shared_ptr goes out of scope
}

void BufferPool::cleanup() {
    // Remove unused buffers that are only referenced by the pool
    size_t writeIndex = 0;

    for (size_t readIndex = 0; readIndex < m_pool.size(); ++readIndex) {
        const BufferEntry& entry = m_pool[readIndex];

        bool shouldKeep = entry.inUse ||
                         (entry.buffer && entry.buffer.use_count() > 1);

        if (shouldKeep) {
            if (writeIndex != readIndex) {
                m_pool[writeIndex] = std::move(m_pool[readIndex]);
            }
            ++writeIndex;
        }
    }

    if (writeIndex < m_pool.size()) {
        m_pool.resize(writeIndex);
        optimize(); // Rebuild hash maps after resize
    }

    // Auto-cleanup if pool is getting large
    if (m_pool.size() > m_config.cleanupThreshold) {
        evictLRU();
    }
}

void BufferPool::clear() {
    m_pool.clear();
    m_hashMap.clear();
    m_lruList.clear();
    m_stats.totalBuffers = 0;
    m_stats.activeBuffers = 0;
    m_stats.availableBuffers = 0;
}

void BufferPool::optimize() {
    // Rebuild hash map and LRU list
    m_hashMap.clear();
    m_lruList.clear();

    for (size_t i = 0; i < m_pool.size(); ++i) {
        addToHashMap(i);
        if (m_config.enableLRU) {
            m_lruList.push_back(i);
            m_pool[i].lruIterator = std::prev(m_lruList.end());
        }
    }

    // Update stats
    m_stats.totalBuffers = m_pool.size();
    m_stats.activeBuffers = getActiveBufferCount();
    m_stats.availableBuffers = m_stats.totalBuffers - m_stats.activeBuffers;
    m_stats.totalMemoryBytes = getTotalMemoryUsage();
}

// --- Statistics and Monitoring ---

size_t BufferPool::getActiveBufferCount() const {
    return std::count_if(m_pool.begin(), m_pool.end(),
                        [](const BufferEntry& entry) { return entry.inUse; });
}

size_t BufferPool::getAvailableBufferCount() const {
    return m_pool.size() - getActiveBufferCount();
}

size_t BufferPool::getTotalMemoryUsage() const {
    size_t total = 0;
    for (const auto& entry : m_pool) {
        if (entry.buffer) {
            total += entry.buffer->getSize();
        }
    }
    return total;
}

void BufferPool::resetStats() {
    m_stats = {};
    m_stats.totalBuffers = m_pool.size();
    m_stats.activeBuffers = getActiveBufferCount();
    m_stats.availableBuffers = m_stats.totalBuffers - m_stats.activeBuffers;
    m_stats.totalMemoryBytes = getTotalMemoryUsage();
}

void BufferPool::updateConfig(const Config& newConfig) {
    m_config = newConfig;

    // Resize pool if needed
    if (m_pool.size() > newConfig.maxPoolSize) {
        cleanup(); // Remove excess buffers
    } else {
        m_pool.reserve(newConfig.maxPoolSize);
    }

    optimize(); // Rebuild structures with new config
}

// --- Advanced Features ---

void BufferPool::preallocate(BufferType type, BufferUsage usage, GLsizeiptr size, size_t count) {
    for (size_t i = 0; i < count && m_pool.size() < m_config.maxPoolSize; ++i) {
        size_t index = createNewBuffer(type, usage, size);
        if (index == SIZE_MAX) break; // Pool full

        m_pool[index].inUse = false; // Mark as available
    }
}

void BufferPool::reserve(size_t capacity) {
    m_pool.reserve(std::min(capacity, m_config.maxPoolSize));
}

void BufferPool::shrinkToFit() {
    m_pool.shrink_to_fit();
}