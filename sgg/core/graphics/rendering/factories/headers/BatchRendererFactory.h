#pragma once

#include "core/graphics/rendering/batching/headers/BatchRenderer.h"
#include <memory>

/**
 * @brief Factory for creating BatchRenderer instances.
 *
 * Simple, stateless factory optimized for performance.
 * Uses static methods to avoid unnecessary object instantiation.
 */
class BatchRendererFactory {
public:
    // Deleted constructors - this is a utility class with only static methods
    BatchRendererFactory() = delete;
    ~BatchRendererFactory() = delete;
    BatchRendererFactory(const BatchRendererFactory&) = delete;
    BatchRendererFactory& operator=(const BatchRendererFactory&) = delete;
    BatchRendererFactory(BatchRendererFactory&&) = delete;
    BatchRendererFactory& operator=(BatchRendererFactory&&) = delete;

    /**
     * @brief Creates a new BatchRenderer instance.
     *
     * @param maxVertexCount Maximum number of vertices the batch can hold.
     * @param maxIndexCount Maximum number of indices the batch can hold.
     * @return A unique_ptr to the newly created BatchRenderer.
     * @throws std::invalid_argument if parameters are invalid.
     */
    [[nodiscard]] static std::unique_ptr<BatchRenderer> create(
        GLsizeiptr maxVertexCount,
        GLsizeiptr maxIndexCount
    );

    /**
     * @brief Creates a BatchRenderer with default reasonable sizes.
     *
     * @return A unique_ptr to a BatchRenderer with default capacity.
     */
    [[nodiscard]] static std::unique_ptr<BatchRenderer> createDefault();

    /**
     * @brief Creates a BatchRenderer optimized for 2D sprites.
     *
     * @param maxSpriteCount Maximum number of sprites (quads) to batch.
     * @return A unique_ptr to a BatchRenderer configured for sprite batching.
     */
    [[nodiscard]] static std::unique_ptr<BatchRenderer> createForSprites(
        size_t maxSpriteCount
    );
};