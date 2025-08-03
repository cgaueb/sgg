#include "core/graphics/rendering/utils/headers/DrawCommands.h"

// --- Basic Draw Commands ---

void DrawCommands::drawArrays(PrimitiveType mode, GLint first, GLsizei count) noexcept {
    if (!validateDrawParams(mode, count, first)) return;

    glDrawArrays(static_cast<GLenum>(mode), first, count);
}

void DrawCommands::drawArrays(GLenum mode, GLint first, GLsizei count) noexcept {
    drawArrays(static_cast<PrimitiveType>(mode), first, count);
}

void DrawCommands::drawElements(PrimitiveType mode, GLsizei count, IndexType indexType,
                               const void* indices) noexcept {
    if (!validateDrawParams(mode, count)) return;

    glDrawElements(static_cast<GLenum>(mode), count, static_cast<GLenum>(indexType), indices);
}

void DrawCommands::drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) noexcept {
    drawElements(static_cast<PrimitiveType>(mode), count, static_cast<IndexType>(type), indices);
}

void DrawCommands::drawArraysInstanced(PrimitiveType mode, GLint first, GLsizei count,
                                      GLsizei instanceCount) noexcept {
    if (!validateDrawParams(mode, count, first) || instanceCount <= 0) return;

    glDrawArraysInstanced(static_cast<GLenum>(mode), first, count, instanceCount);
}

void DrawCommands::drawElementsInstanced(PrimitiveType mode, GLsizei count, IndexType indexType,
                                        const void* indices, GLsizei instanceCount) noexcept {
    if (!validateDrawParams(mode, count) || instanceCount <= 0) return;

    glDrawElementsInstanced(static_cast<GLenum>(mode), count, static_cast<GLenum>(indexType),
                           indices, instanceCount);
}

// --- Advanced Draw Commands ---

void DrawCommands::drawElementsBaseVertex(PrimitiveType mode, GLsizei count, IndexType indexType,
                                          void *indices, GLint baseVertex) noexcept {
    if (!validateDrawParams(mode, count)) return;

    glDrawElementsBaseVertex(static_cast<GLenum>(mode), count, static_cast<GLenum>(indexType),
                            indices, baseVertex);
}

void DrawCommands::drawElementsInstancedBaseVertex(PrimitiveType mode, GLsizei count, IndexType indexType,
                                                  const void* indices, GLsizei instanceCount,
                                                  GLint baseVertex) noexcept {
    if (!validateDrawParams(mode, count) || instanceCount <= 0) return;

    glDrawElementsInstancedBaseVertex(static_cast<GLenum>(mode), count, static_cast<GLenum>(indexType),
                                     indices, instanceCount, baseVertex);
}

void DrawCommands::drawRangeElements(PrimitiveType mode, GLuint start, GLuint end, GLsizei count,
                                   IndexType indexType, const void* indices) noexcept {
    if (!validateDrawParams(mode, count) || start > end) return;

    glDrawRangeElements(static_cast<GLenum>(mode), start, end, count,
                       static_cast<GLenum>(indexType), indices);
}

// --- Multi-Draw Commands ---

void DrawCommands::multiDrawArrays(PrimitiveType mode, const GLint* first, const GLsizei* count,
                                  GLsizei drawCount) noexcept {
    if (drawCount <= 0 || !first || !count) return;

    // Validate all draw calls
    for (GLsizei i = 0; i < drawCount; ++i) {
        if (!validateDrawParams(mode, count[i], first[i])) return;
    }

    glMultiDrawArrays(static_cast<GLenum>(mode), first, count, drawCount);
}

void DrawCommands::multiDrawElements(PrimitiveType mode, const GLsizei* count, IndexType indexType,
                                    const void* const* indices, GLsizei drawCount) noexcept {
    if (drawCount <= 0 || !count || !indices) return;

    // Validate all draw calls
    for (GLsizei i = 0; i < drawCount; ++i) {
        if (!validateDrawParams(mode, count[i])) return;
    }

    glMultiDrawElements(static_cast<GLenum>(mode), count, static_cast<GLenum>(indexType),
                       indices, drawCount);
}

// --- Convenience Methods ---

void DrawCommands::drawFullscreenQuad() noexcept {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void DrawCommands::drawTriangles(GLsizei count, const void* indices) noexcept {
    if (indices) {
        drawElements(PrimitiveType::Triangles, count, IndexType::UnsignedInt, indices);
    } else {
        drawArrays(PrimitiveType::Triangles, 0, count);
    }
}

void DrawCommands::drawLines(GLsizei count, const void* indices) noexcept {
    if (indices) {
        drawElements(PrimitiveType::Lines, count, IndexType::UnsignedInt, indices);
    } else {
        drawArrays(PrimitiveType::Lines, 0, count);
    }
}

void DrawCommands::drawPoints(GLsizei count) noexcept {
    drawArrays(PrimitiveType::Points, 0, count);
}

void DrawCommands::drawAuto(PrimitiveType mode, GLsizei vertexCount, const void* indices) noexcept {
    if (indices) {
        // Calculate index count based on primitive type and vertex count
        const GLsizei indexCount = getPrimitiveCount(mode, vertexCount) *
                                  (mode == PrimitiveType::Triangles ? 3 :
                                   mode == PrimitiveType::Lines ? 2 : 1);
        drawElements(mode, indexCount, IndexType::UnsignedInt, indices);
    } else {
        drawArrays(mode, 0, vertexCount);
    }
}

// --- Utility Methods ---

bool DrawCommands::validateDrawParams(PrimitiveType mode, GLsizei count, GLint first) noexcept {
    // Fast validation for performance-critical paths
    return count > 0 &&
           first >= 0 &&
           isValidPrimitiveMode(static_cast<GLenum>(mode));
}

bool DrawCommands::isValidPrimitiveMode(GLenum mode) noexcept {
    switch (mode) {
        case GL_POINTS:
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_PATCHES:
            return true;
        default:
            return false;
    }
}

bool DrawCommands::isValidIndexType(GLenum type) noexcept {
    switch (type) {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT:
        case GL_UNSIGNED_INT:
            return true;
        default:
            return false;
    }
}