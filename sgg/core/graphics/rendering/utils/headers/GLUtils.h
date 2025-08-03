#pragma once

#include <GL/glew.h>
#include <string>
#include <string_view>
#include <stdexcept>
#include <cstdint>
#include <type_traits>
#include <glm/glm.hpp>

// --- Performance-optimized error checking ---

/**
 * @brief Optimized OpenGL error checking with detailed reporting.
 *
 * @param filename Source file name (use __FILE__)
 * @param line Line number (use __LINE__)
 * @param function Function name (use __func__)
 * @return true if no errors, false if errors found
 */
[[nodiscard]] bool checkGlError(const char* filename, int line, const char* function = nullptr) noexcept;

/**
 * @brief Get error string without throwing (for performance-critical paths).
 */
[[nodiscard]] std::string_view getGlErrorString(GLenum error) noexcept;

/**
 * @brief Fast error check that only returns boolean (no string formatting).
 */
[[nodiscard]] inline bool hasGlError() noexcept {
    return glGetError() != GL_NO_ERROR;
}

// Optimized GL_CHECK macros with different levels
#ifdef NDEBUG
    // Release: No error checking for maximum performance
    #define GL_CHECK(stmt) stmt
    #define GL_CHECK_CRITICAL(stmt) stmt
    #define GL_CHECK_DEBUG(stmt) stmt
#else
    // Debug: Full error checking
    #define GL_CHECK(stmt) do { \
        stmt; \
        checkGlError(__FILE__, __LINE__, __func__); \
    } while (0)

    // Critical operations always checked (even in optimized debug)
    #define GL_CHECK_CRITICAL(stmt) GL_CHECK(stmt)

    // Debug-only checks for non-critical operations
    #define GL_CHECK_DEBUG(stmt) GL_CHECK(stmt)
#endif

// Performance variant that doesn't throw
#define GL_CHECK_NOTHROW(stmt) do { \
    stmt; \
    hasGlError(); \
} while (0)

// --- Enhanced Strong Types with Better Performance ---

/**
 * @brief Template base for strong ID types with optimized operations.
 */
template<typename Tag, typename ValueType = uint32_t>
struct StrongId {
    using value_type = ValueType;

    ValueType value = 0;

    constexpr StrongId() = default;
    constexpr explicit StrongId(ValueType v) noexcept : value(v) {}

    // Fast validity check
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return value != 0;
    }

    // Efficient comparison operators
    [[nodiscard]] constexpr bool operator==(const StrongId& other) const noexcept {
        return value == other.value;
    }
    [[nodiscard]] constexpr bool operator!=(const StrongId& other) const noexcept {
        return value != other.value;
    }
    [[nodiscard]] constexpr bool operator<(const StrongId& other) const noexcept {
        return value < other.value;
    }
    [[nodiscard]] constexpr bool operator<=(const StrongId& other) const noexcept {
        return value <= other.value;
    }
    [[nodiscard]] constexpr bool operator>(const StrongId& other) const noexcept {
        return value > other.value;
    }
    [[nodiscard]] constexpr bool operator>=(const StrongId& other) const noexcept {
        return value >= other.value;
    }

    // Increment/decrement for ID generation
    StrongId& operator++() noexcept { ++value; return *this; }
    StrongId& operator--() noexcept { --value; return *this; }
    StrongId operator++(int) noexcept { StrongId tmp = *this; ++value; return tmp; }
    StrongId operator--(int) noexcept { StrongId tmp = *this; --value; return tmp; }

    // Arithmetic for offset calculations
    [[nodiscard]] constexpr StrongId operator+(ValueType offset) const noexcept {
        return StrongId{value + offset};
    }
    [[nodiscard]] constexpr StrongId operator-(ValueType offset) const noexcept {
        return StrongId{value - offset};
    }

    // Reset to invalid state
    void reset() noexcept { value = 0; }

    // Check if valid
    [[nodiscard]] constexpr bool isValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool isInvalid() const noexcept { return value == 0; }
};

// Tag types for different ID categories
struct BufferIdTag {};
struct VAOIdTag {};
struct TextureIdTag {};
struct ShaderIdTag {};
struct ProgramIdTag {};
struct FramebufferIdTag {};

// Specific ID types
using BufferId = StrongId<BufferIdTag>;
using VAOId = StrongId<VAOIdTag>;
using TextureId = StrongId<TextureIdTag>;
using ShaderId = StrongId<ShaderIdTag>;
using ProgramId = StrongId<ProgramIdTag>;
using FramebufferId = StrongId<FramebufferIdTag>;

// Hash support for unordered containers
namespace std {
    template<typename Tag, typename ValueType>
    struct hash<StrongId<Tag, ValueType>> {
        [[nodiscard]] constexpr size_t operator()(const StrongId<Tag, ValueType>& id) const noexcept {
            return hash<ValueType>{}(id.value);
        }
    };
}

// --- Enhanced Enums with Better Type Safety ---

/**
 * @brief Buffer usage patterns with performance hints.
 */
enum class BufferUsage : GLenum {
    StaticDraw = GL_STATIC_DRAW,     ///< Written once, read many times
    DynamicDraw = GL_DYNAMIC_DRAW,   ///< Written repeatedly, read many times
    StreamDraw = GL_STREAM_DRAW,     ///< Written once, read few times
    StaticRead = GL_STATIC_READ,     ///< Written once by GL, read many times by app
    DynamicRead = GL_DYNAMIC_READ,   ///< Written repeatedly by GL, read many times by app
    StreamRead = GL_STREAM_READ,     ///< Written once by GL, read few times by app
    StaticCopy = GL_STATIC_COPY,     ///< Written once by GL, read many times by GL
    DynamicCopy = GL_DYNAMIC_COPY,   ///< Written repeatedly by GL, read many times by GL
    StreamCopy = GL_STREAM_COPY      ///< Written once by GL, read few times by GL
};

/**
 * @brief Buffer types with binding targets.
 */
enum class BufferType : GLenum {
    Vertex = GL_ARRAY_BUFFER,
    Index = GL_ELEMENT_ARRAY_BUFFER,
    Uniform = GL_UNIFORM_BUFFER,
    ShaderStorage = GL_SHADER_STORAGE_BUFFER,
    Texture = GL_TEXTURE_BUFFER,
    Transform = GL_TRANSFORM_FEEDBACK_BUFFER,
    AtomicCounter = GL_ATOMIC_COUNTER_BUFFER,
    DrawIndirect = GL_DRAW_INDIRECT_BUFFER,
    DispatchIndirect = GL_DISPATCH_INDIRECT_BUFFER
};

/**
 * @brief Texture targets for different texture types.
 */
enum class TextureType : GLenum {
    Texture1D = GL_TEXTURE_1D,
    Texture2D = GL_TEXTURE_2D,
    Texture3D = GL_TEXTURE_3D,
    TextureCubeMap = GL_TEXTURE_CUBE_MAP,
    Texture1DArray = GL_TEXTURE_1D_ARRAY,
    Texture2DArray = GL_TEXTURE_2D_ARRAY,
    TextureCubeMapArray = GL_TEXTURE_CUBE_MAP_ARRAY,
    Texture2DMultisample = GL_TEXTURE_2D_MULTISAMPLE,
    Texture2DMultisampleArray = GL_TEXTURE_2D_MULTISAMPLE_ARRAY
};

/**
 * @brief Shader types.
 */
enum class ShaderType : GLenum {
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    TessControl = GL_TESS_CONTROL_SHADER,
    TessEvaluation = GL_TESS_EVALUATION_SHADER,
    Compute = GL_COMPUTE_SHADER
};

// --- Optimized Vertex Attribute Description ---

/**
 * @brief Optimized vertex attribute descriptor with constexpr support.
 */
struct VertexAttribute {
    GLuint location;           ///< Attribute location in shader
    GLint componentCount;      ///< Components per attribute (1-4)
    GLenum componentType;      ///< Data type (GL_FLOAT, GL_INT, etc.)
    GLboolean normalized;      ///< Normalize fixed-point values
    GLsizei stride;           ///< Byte stride between attributes
    GLsizeiptr offset;        ///< Byte offset from start of vertex

    // Constructors for common patterns
    constexpr VertexAttribute(GLuint loc, GLint components, GLenum type,
                             GLsizei str = 0, GLsizeiptr off = 0,
                             GLboolean norm = GL_FALSE) noexcept
        : location(loc), componentCount(components), componentType(type)
        , normalized(norm), stride(str), offset(off) {}

    // Helper for pointer-based offset (legacy compatibility)
    [[nodiscard]] const void* getOffsetPtr() const noexcept {
        return reinterpret_cast<const void*>(offset);
    }

    // Common attribute types
    [[nodiscard]] static constexpr VertexAttribute position(GLuint location, GLsizei stride = 0, GLsizeiptr offset = 0) noexcept {
        return {location, 3, GL_FLOAT, stride, offset};
    }

    [[nodiscard]] static constexpr VertexAttribute normal(GLuint location, GLsizei stride = 0, GLsizeiptr offset = 0) noexcept {
        return {location, 3, GL_FLOAT, stride, offset};
    }

    [[nodiscard]] static constexpr VertexAttribute texCoord(GLuint location, GLsizei stride = 0, GLsizeiptr offset = 0) noexcept {
        return {location, 2, GL_FLOAT, stride, offset};
    }

    [[nodiscard]] static constexpr VertexAttribute color(GLuint location, GLsizei stride = 0, GLsizeiptr offset = 0) noexcept {
        return {location, 4, GL_FLOAT, stride, offset};
    }
};

// --- Fast Enum Conversion Functions ---

/**
 * @brief Convert BufferType to GL target (constexpr for compile-time optimization).
 */
[[nodiscard]] constexpr GLenum toGLenum(BufferType type) noexcept {
    return static_cast<GLenum>(type);
}

/**
 * @brief Convert BufferUsage to GL usage (constexpr for compile-time optimization).
 */
[[nodiscard]] constexpr GLenum toGLenum(BufferUsage usage) noexcept {
    return static_cast<GLenum>(usage);
}

/**
 * @brief Legacy function names for backward compatibility.
 */
[[deprecated("Use toGLenum(BufferType) instead")]]
[[nodiscard]] constexpr GLenum toGlBufferTarget(BufferType type) noexcept {
    return toGLenum(type);
}

[[deprecated("Use toGLenum(BufferUsage) instead")]]
[[nodiscard]] constexpr GLenum toGlBufferUsage(BufferUsage usage) noexcept {
    return toGLenum(usage);
}

// --- Utility Functions ---

/**
 * @brief Get size in bytes for common GL types.
 */
[[nodiscard]] constexpr size_t getGLTypeSize(GLenum type) noexcept {
    switch (type) {
        case GL_BYTE:           return sizeof(GLbyte);
        case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
        case GL_SHORT:          return sizeof(GLshort);
        case GL_UNSIGNED_SHORT: return sizeof(GLushort);
        case GL_INT:            return sizeof(GLint);
        case GL_UNSIGNED_INT:   return sizeof(GLuint);
        case GL_FLOAT:          return sizeof(GLfloat);
        case GL_DOUBLE:         return sizeof(GLdouble);
        default:                return 0;
    }
}

/**
 * @brief Calculate vertex attribute size in bytes.
 */
[[nodiscard]] constexpr size_t calculateAttributeSize(const VertexAttribute& attr) noexcept {
    return getGLTypeSize(attr.componentType) * attr.componentCount;
}

/**
 * @brief OpenGL capability management.
 */
class GLCapabilities {
public:
    [[nodiscard]] static bool isExtensionSupported(std::string_view extension) noexcept;
    [[nodiscard]] static int getMaxTextureUnits() noexcept;
    [[nodiscard]] static int getMaxVertexAttributes() noexcept;
    [[nodiscard]] static int getMaxUniformBlockSize() noexcept;
    [[nodiscard]] static std::string getRenderer() noexcept;
    [[nodiscard]] static std::string getVersion() noexcept;

private:
    GLCapabilities() = delete;
};

// --- Debug Utilities ---

#ifndef NDEBUG
/**
 * @brief Debug label for OpenGL objects (debug builds only).
 */
void setDebugLabel(GLenum identifier, GLuint name, std::string_view label) noexcept;

/**
 * @brief Push debug group for hierarchical debugging.
 */
void pushDebugGroup(std::string_view message) noexcept;

/**
 * @brief Pop debug group.
 */
void popDebugGroup() noexcept;

// RAII debug group helper
class DebugGroup {
    bool m_active;
public:
    explicit DebugGroup(std::string_view message) noexcept : m_active(true) {
        pushDebugGroup(message);
    }

    ~DebugGroup() noexcept {
        if (m_active) popDebugGroup();
    }

    DebugGroup(const DebugGroup&) = delete;
    DebugGroup& operator=(const DebugGroup&) = delete;
    DebugGroup(DebugGroup&& other) noexcept : m_active(other.m_active) {
        other.m_active = false;
    }
    DebugGroup& operator=(DebugGroup&& other) noexcept {
        if (this != &other) {
            if (m_active) popDebugGroup();
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }
};

#define GL_DEBUG_GROUP(message) DebugGroup _debug_group(message)
#else
#define GL_DEBUG_GROUP(message) ((void)0)
#endif