#include "core/graphics/rendering/factories/headers/BatchRendererFactory.h"
#include <stdexcept>

std::unique_ptr<BatchRenderer> BatchRendererFactory::create(
    GLsizeiptr maxVertexCount,
    GLsizeiptr maxIndexCount
) {
    // Early validation for better error reporting
    if (maxVertexCount <= 0 || maxIndexCount <= 0) {
        throw std::invalid_argument("Vertex and index counts must be positive");
    }

    // Use make_unique for exception safety and potential optimization
    return std::make_unique<BatchRenderer>(maxVertexCount, maxIndexCount);
}

std::unique_ptr<BatchRenderer> BatchRendererFactory::createDefault() {
    // Reasonable defaults for most use cases
    constexpr GLsizeiptr DEFAULT_VERTEX_COUNT = 10000;
    constexpr GLsizeiptr DEFAULT_INDEX_COUNT = 15000; // 1.5x vertices for typical meshes

    return create(DEFAULT_VERTEX_COUNT, DEFAULT_INDEX_COUNT);
}

std::unique_ptr<BatchRenderer> BatchRendererFactory::createForSprites(
    size_t maxSpriteCount
) {
    if (maxSpriteCount == 0) {
        throw std::invalid_argument("Sprite count must be positive");
    }

    // Each sprite (quad) needs 4 vertices and 6 indices (2 triangles)
    constexpr size_t VERTICES_PER_SPRITE = 4;
    constexpr size_t INDICES_PER_SPRITE = 6;

    const auto vertexCount = static_cast<GLsizeiptr>(maxSpriteCount * VERTICES_PER_SPRITE);
    const auto indexCount = static_cast<GLsizeiptr>(maxSpriteCount * INDICES_PER_SPRITE);

    return create(vertexCount, indexCount);
}