#include "core/graphics/rendering/utils/headers/GLUtils.h"
#include <iostream>
#include <unordered_map>

// --- Optimized Error Checking ---

std::string_view getGlErrorString(GLenum error) noexcept {
    // Using string_view for zero-allocation error strings
    switch (error) {
        case GL_NO_ERROR:                      return "NO_ERROR";
        case GL_INVALID_ENUM:                  return "INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "INVALID_OPERATION";
        case GL_STACK_OVERFLOW:                return "STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW:               return "STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY:                 return "OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "INVALID_FRAMEBUFFER_OPERATION";
        case GL_CONTEXT_LOST:                  return "CONTEXT_LOST";
        default:                               return "UNKNOWN_ERROR";
    }
}

bool checkGlError(const char* filename, int line, const char* function) noexcept {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        return true;
    }

    // Handle multiple errors efficiently
    bool hasError = false;
    do {
        hasError = true;

        // Extract just the filename from full path for cleaner logs
        const char* fileBase = filename;
        const char* lastSlash = nullptr;
        for (const char* p = filename; *p; ++p) {
            if (*p == '/' || *p == '\\') {
                lastSlash = p;
            }
        }
        if (lastSlash) {
            fileBase = lastSlash + 1;
        }

        // Construct error message efficiently
        std::cerr << "OpenGL Error: " << getGlErrorString(error)
                  << " (0x" << std::hex << error << std::dec << ")";

        if (function) {
            std::cerr << " in " << function;
        }

        std::cerr << " at " << fileBase << ":" << line << std::endl;

        error = glGetError();
    } while (error != GL_NO_ERROR);

#ifndef NDEBUG
    if (hasError) {}
#endif

    return !hasError;
}

// --- OpenGL Capabilities ---

bool GLCapabilities::isExtensionSupported(std::string_view extension) noexcept {
    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    for (GLint i = 0; i < numExtensions; ++i) {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (ext && extension == ext) {
            return true;
        }
    }
    return false;
}

int GLCapabilities::getMaxTextureUnits() noexcept {
    static int maxUnits = -1;
    if (maxUnits == -1) {
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
    }
    return maxUnits;
}

int GLCapabilities::getMaxVertexAttributes() noexcept {
    static int maxAttribs = -1;
    if (maxAttribs == -1) {
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    }
    return maxAttribs;
}

int GLCapabilities::getMaxUniformBlockSize() noexcept {
    static int maxSize = -1;
    if (maxSize == -1) {
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxSize);
    }
    return maxSize;
}

std::string GLCapabilities::getRenderer() noexcept {
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    return renderer ? std::string(renderer) : "Unknown";
}

std::string GLCapabilities::getVersion() noexcept {
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    return version ? std::string(version) : "Unknown";
}

// --- Debug Utilities (Debug builds only) ---

#ifndef NDEBUG

void setDebugLabel(GLenum identifier, GLuint name, std::string_view label) noexcept {
    if (glObjectLabel) {
        glObjectLabel(identifier, name, static_cast<GLsizei>(label.length()), label.data());
    }
}

void pushDebugGroup(std::string_view message) noexcept {
    if (glPushDebugGroup) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0,
                        static_cast<GLsizei>(message.length()), message.data());
    }
}

void popDebugGroup() noexcept {
    if (glPopDebugGroup) {
        glPopDebugGroup();
    }
}

#endif // NDEBUG