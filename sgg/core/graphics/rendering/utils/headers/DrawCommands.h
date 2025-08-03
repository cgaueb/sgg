#pragma once

#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include <cstddef>

/**
 * @brief High-performance interface for OpenGL draw commands with validation and optimization.
 *
 * Features:
 * - Optimized draw calls with minimal overhead
 * - Comprehensive input validation
 * - Support for modern OpenGL drawing techniques
 * - Debug-friendly error reporting
 * - Batch and multi-draw support
 */
class DrawCommands {
public:
    /**
     * @brief Common primitive types for convenience.
     */
    enum class PrimitiveType : GLenum {
        Points = GL_POINTS,
        Lines = GL_LINES,
        LineStrip = GL_LINE_STRIP,
        LineLoop = GL_LINE_LOOP,
        Triangles = GL_TRIANGLES,
        TriangleStrip = GL_TRIANGLE_STRIP,
        TriangleFan = GL_TRIANGLE_FAN,
        Patches = GL_PATCHES // For tessellation
    };

    /**
     * @brief Index data types for type safety.
     */
    enum class IndexType : GLenum {
        UnsignedByte = GL_UNSIGNED_BYTE,
        UnsignedShort = GL_UNSIGNED_SHORT,
        UnsignedInt = GL_UNSIGNED_INT
    };

    // --- Basic Draw Commands ---

    /**
     * @brief Issues a non-indexed draw call (optimized).
     *
     * @param mode Primitive type to render.
     * @param first Starting vertex index.
     * @param count Number of vertices to render.
     */
    static void drawArrays(PrimitiveType mode, GLint first, GLsizei count) noexcept;

    /**
     * @brief Legacy overload for backward compatibility.
     */
    [[deprecated("Use PrimitiveType enum instead")]]
    static void drawArrays(GLenum mode, GLint first, GLsizei count) noexcept;

    /**
     * @brief Issues an indexed draw call (optimized).
     *
     * @param mode Primitive type to render.
     * @param count Number of indices to render.
     * @param indexType Type of index data.
     * @param indices Pointer to indices or offset into bound EBO.
     */
    static void drawElements(PrimitiveType mode, GLsizei count, IndexType indexType,
                             const void *indices = nullptr) noexcept;

    /**
     * @brief Legacy overload for backward compatibility.
     */
    [[deprecated("Use typed enums instead")]]
    static void drawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) noexcept;

    /**
     * @brief Issues an instanced draw call.
     *
     * @param mode Primitive type to render.
     * @param first Starting vertex index.
     * @param count Number of vertices per instance.
     * @param instanceCount Number of instances to render.
     */
    static void drawArraysInstanced(PrimitiveType mode, GLint first, GLsizei count,
                                    GLsizei instanceCount) noexcept;

    /**
     * @brief Issues an indexed instanced draw call.
     *
     * @param mode Primitive type to render.
     * @param count Number of indices per instance.
     * @param indexType Type of index data.
     * @param indices Pointer to indices or offset.
     * @param instanceCount Number of instances to render.
     */
    static void drawElementsInstanced(PrimitiveType mode, GLsizei count, IndexType indexType,
                                      const void *indices, GLsizei instanceCount) noexcept;

    // --- Advanced Draw Commands ---

    /**
     * @brief Draw with base vertex offset (OpenGL 3.2+).
     *
     * Useful for rendering from a single large vertex buffer with multiple objects.
     */
    static void drawElementsBaseVertex(PrimitiveType mode, GLsizei count, IndexType indexType,
                                       void *indices, GLint baseVertex) noexcept;

    /**
     * @brief Draw instanced with base vertex offset.
     */
    static void drawElementsInstancedBaseVertex(PrimitiveType mode, GLsizei count, IndexType indexType,
                                                const void *indices, GLsizei instanceCount,
                                                GLint baseVertex) noexcept;

    /**
     * @brief Draw elements with range validation (OpenGL 3.0+).
     *
     * Provides additional validation by specifying the range of vertex indices.
     */
    static void drawRangeElements(PrimitiveType mode, GLuint start, GLuint end, GLsizei count,
                                  IndexType indexType, const void *indices) noexcept;

    // --- Multi-Draw Commands (OpenGL 1.4+) ---

    /**
     * @brief Draw multiple non-contiguous arrays in a single call.
     *
     * @param mode Primitive type.
     * @param first Array of starting indices.
     * @param count Array of vertex counts.
     * @param drawCount Number of draw calls.
     */
    static void multiDrawArrays(PrimitiveType mode, const GLint *first, const GLsizei *count,
                                GLsizei drawCount) noexcept;

    /**
     * @brief Draw multiple indexed primitives in a single call.
     *
     * @param mode Primitive type.
     * @param count Array of index counts.
     * @param indexType Type of index data.
     * @param indices Array of pointers to index data.
     * @param drawCount Number of draw calls.
     */
    static void multiDrawElements(PrimitiveType mode, const GLsizei *count, IndexType indexType,
                                  const void *const*indices, GLsizei drawCount) noexcept;

    // --- Convenience Methods ---

    /**
     * @brief Draw a full-screen quad (common for post-processing).
     *
     * Assumes triangle strip with 4 vertices arranged as a quad.
     */
    static void drawFullscreenQuad() noexcept;

    /**
     * @brief Draw common geometric shapes.
     */
    static void drawTriangles(GLsizei count, const void *indices = nullptr) noexcept;

    static void drawLines(GLsizei count, const void *indices = nullptr) noexcept;

    static void drawPoints(GLsizei count) noexcept;

    /**
     * @brief Draw with automatic primitive count calculation.
     *
     * @param mode Primitive type.
     * @param vertexCount Total number of vertices.
     * @param indices Optional index buffer offset.
     */
    static void drawAuto(PrimitiveType mode, GLsizei vertexCount, const void *indices = nullptr) noexcept;

    // --- Utility Methods ---

    /**
     * @brief Validates draw parameters without issuing draw call.
     *
     * @return true if parameters are valid for drawing.
     */
    static bool validateDrawParams(PrimitiveType mode, GLsizei count, GLint first = 0) noexcept;

    /**
     * @brief Gets the size in bytes for an index type.
     */
    static constexpr size_t getIndexSize(IndexType type) noexcept;

    /**
     * @brief Calculates memory usage for index buffer.
     */
    static constexpr size_t calculateIndexMemory(IndexType type, GLsizei count) noexcept;

    /**
     * @brief Gets primitive count from vertex count.
     */
    static constexpr GLsizei getPrimitiveCount(PrimitiveType mode, GLsizei vertexCount) noexcept;

private:
    // Internal validation helpers
    static bool isValidPrimitiveMode(GLenum mode) noexcept;

    static bool isValidIndexType(GLenum type) noexcept;

    // Deleted constructor - utility class only
    DrawCommands() = delete;

    ~DrawCommands() = delete;

    DrawCommands(const DrawCommands &) = delete;

    DrawCommands &operator=(const DrawCommands &) = delete;
};

// --- Inline Implementations ---

constexpr size_t DrawCommands::getIndexSize(IndexType type) noexcept {
    switch (type) {
        case IndexType::UnsignedByte: return sizeof(GLubyte);
        case IndexType::UnsignedShort: return sizeof(GLushort);
        case IndexType::UnsignedInt: return sizeof(GLuint);
        default: return 0;
    }
}

constexpr size_t DrawCommands::calculateIndexMemory(IndexType type, GLsizei count) noexcept {
    return count > 0 ? getIndexSize(type) * static_cast<size_t>(count) : 0;
}

constexpr GLsizei DrawCommands::getPrimitiveCount(PrimitiveType mode, GLsizei vertexCount) noexcept {
    switch (mode) {
        case PrimitiveType::Points: return vertexCount;
        case PrimitiveType::Lines: return vertexCount / 2;
        case PrimitiveType::LineStrip: return vertexCount > 1 ? vertexCount - 1 : 0;
        case PrimitiveType::LineLoop: return vertexCount > 2 ? vertexCount : 0;
        case PrimitiveType::Triangles: return vertexCount / 3;
        case PrimitiveType::TriangleStrip: return vertexCount > 2 ? vertexCount - 2 : 0;
        case PrimitiveType::TriangleFan: return vertexCount > 2 ? vertexCount - 2 : 0;
        case PrimitiveType::Patches: return vertexCount; // Depends on patch size
        default: return 0;
    }
}
