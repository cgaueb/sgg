#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <initializer_list>

#include "glm/glm.hpp"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

/** @def ASSERT_GL
 *  @brief Asserts no OpenGL errors have occurred.
 */
#define ASSERT_GL { assert(glGetError() == GL_NO_ERROR); }

// Forward declarations
class UniformHandle;
class Shader;

/** @typedef UniformValue
 *  @brief Variant type for holding different uniform values supported by shaders.
 */
using UniformValue = std::variant<
    int, float, unsigned int,
    glm::vec2, glm::vec3, glm::vec4,
    glm::ivec2, glm::ivec3, glm::ivec4,
    glm::mat3, glm::mat4
>;

/** @class Uniform
 *  @brief Manages a shader uniform with modern C++ features and type-safe value handling.
 */
class Uniform {
public:
    /** @brief Uniform ID, initialized to -1. */
    int id{-1};
    /** @brief Holds the uniform's value as a variant. */
    UniformValue value{};
    /** @brief Flag indicating if the uniform is smart (automatically applied). */
    bool smartFlag{false};

    /** @brief Default constructor with member initialization. */
    constexpr Uniform() = default;

    /** @brief Constructs a Uniform with a specified ID.
     *  @param i The uniform ID.
     */
    constexpr explicit Uniform(int i) : id(i) {}

    // Rule of 5 - explicitly defaulted
    Uniform(const Uniform&) = default;
    Uniform(Uniform&&) = default;
    Uniform& operator=(const Uniform&) = default;
    Uniform& operator=(Uniform&&) = default;
    ~Uniform() = default;

    /** @brief Converts to int, returning the uniform ID.
     *  @return The uniform ID.
     */
    constexpr operator int() const noexcept { return id; }

    /** @brief Assigns a value to the uniform.
     *  @tparam T Type of the value, must be convertible to UniformValue.
     *  @param val The value to assign.
     *  @return Reference to this Uniform.
     */
    template<typename T>
    Uniform& operator=(T&& val) noexcept {
        static_assert(std::is_convertible_v<std::decay_t<T>, UniformValue>,
                     "Type must be convertible to UniformValue");

        value = std::forward<T>(val);
        if (id != -1) {
            applyValue(std::forward<T>(val));
        }
        return *this;
    }

    /** @brief Assigns an int value to the uniform.
     *  @param val The int value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(int val) noexcept;

    /** @brief Assigns a float value to the uniform.
     *  @param val The float value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(float val) noexcept;

    /** @brief Assigns an unsigned int value to the uniform.
     *  @param val The unsigned int value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(unsigned int val) noexcept;

    /** @brief Assigns a glm::vec2 value to the uniform.
     *  @param val The glm::vec2 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::vec2& val) noexcept;

    /** @brief Assigns a glm::vec3 value to the uniform.
     *  @param val The glm::vec3 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::vec3& val) noexcept;

    /** @brief Assigns a glm::vec4 value to the uniform.
     *  @param val The glm::vec4 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::vec4& val) noexcept;

    /** @brief Assigns a glm::ivec2 value to the uniform.
     *  @param val The glm::ivec2 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::ivec2& val) noexcept;

    /** @brief Assigns a glm::ivec3 value to the uniform.
     *  @param val The glm::ivec3 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::ivec3& val) noexcept;

    /** @brief Assigns a glm::ivec4 value to the uniform.
     *  @param val The glm::ivec4 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::ivec4& val) noexcept;

    /** @brief Assigns a glm::mat3 value to the uniform.
     *  @param val The glm::mat3 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::mat3& val) noexcept;

    /** @brief Assigns a glm::mat4 value to the uniform.
     *  @param val The glm::mat4 value.
     *  @return Reference to this Uniform.
     */
    Uniform& operator=(const glm::mat4& val) noexcept;

    /** @brief Checks if the uniform is smart.
     *  @return True if the uniform is smart, false otherwise.
     */
    [[nodiscard]] constexpr bool isSmart() const noexcept { return smartFlag; }

    /** @brief Sets the smart flag for the uniform.
     *  @param smart The smart flag value.
     *  @return Reference to this Uniform.
     */
    Uniform& setSmart(bool smart) noexcept {
        smartFlag = smart;
        return *this;
    }

    /** @brief Applies the uniform's value to the shader program. */
    void apply() const noexcept;

    /** @brief Checks if the uniform is valid (has a valid ID).
     *  @return True if the uniform is valid, false otherwise.
     */
    [[nodiscard]] constexpr bool isValid() const noexcept { return id != -1; }

private:
    /** @brief Applies a specific value to the uniform.
     *  @tparam T Type of the value.
     *  @param val The value to apply.
     */
    template<typename T>
    inline void applyValue(const T& val) const noexcept {
        if (id == -1) return;

        if constexpr (std::is_same_v<T, int>) {
            glUniform1i(id, val);
        } else if constexpr (std::is_same_v<T, float>) {
            glUniform1f(id, val);
        } else if constexpr (std::is_same_v<T, unsigned int>) {
            glUniform1ui(id, val);
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            glUniform2fv(id, 1, glm::value_ptr(val));
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            glUniform3fv(id, 1, glm::value_ptr(val));
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            glUniform4fv(id, 1, glm::value_ptr(val));
        } else if constexpr (std::is_same_v<T, glm::ivec2>) {
            glUniform2iv(id, 1, glm::value_ptr(val));
        } else if constexpr (std::is_same_v<T, glm::ivec3>) {
            glUniform3iv(id, 1, glm::value_ptr(val));
        } else if constexpr (std::is_same_v<T, glm::ivec4>) {
            glUniform4iv(id, 1, glm::value_ptr(val));
        } else if constexpr (std::is_same_v<T, glm::mat3>) {
            glUniformMatrix3fv(id, 1, GL_FALSE, glm::value_ptr(val));
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            glUniformMatrix4fv(id, 1, GL_FALSE, glm::value_ptr(val));
        }
    }
};

/** @class Shader
 *  @brief Manages OpenGL shader programs with RAII and modern C++ features.
 */
class Shader {
public:
    /** @typedef UniformMap
     *  @brief Map of uniform names to Uniform objects.
     */
    using UniformMap = std::unordered_map<std::string, Uniform>;

    /** @typedef AttributeMap
     *  @brief Map of attribute names to their locations.
     */
    using AttributeMap = std::unordered_map<std::string, unsigned int>;

    /** @typedef UniformHandleVector
     *  @brief Vector of uniform name and pointer pairs for handles.
     */
    using UniformHandleVector = std::vector<std::pair<std::string, Uniform*>>;

    /** @typedef HandleIndexMap
     *  @brief Map of uniform names to their indices in the handle vector.
     */
    using HandleIndexMap = std::unordered_map<std::string_view, size_t>;

    /** @typedef FileCache
     *  @brief Cache for shader source files.
     */
    using FileCache = std::unordered_map<std::string, std::unique_ptr<char[]>>;

    /** @brief Constructs a Shader with optional vertex and fragment shader sources.
     *  @param vertexShader Optional vertex shader source or file path.
     *  @param fragmentShader Optional fragment shader source or file path.
     */
    explicit Shader(std::optional<std::string_view> vertexShader = std::nullopt,
                   std::optional<std::string_view> fragmentShader = std::nullopt);

    // Disable copy operations for resource safety
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Enable move semantics with noexcept guarantee
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /** @brief Destructor, cleans up OpenGL resources. */
    ~Shader();

    /** @brief Accesses a uniform by name (string_view overload).
     *  @param name The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& operator[](std::string_view name) noexcept {
        return getUniformFast(name);
    }

    /** @brief Accesses a uniform by name (C-string overload).
     *  @param name The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& operator[](const char* name) noexcept {
        return getUniform(name);
    }

    /** @brief Accesses a uniform by name (std::string overload).
     *  @param name The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& operator[](const std::string& name) noexcept {
        return getUniform(name.c_str());
    }

    /** @brief Accesses a uniform by name for pointer semantics (string_view overload).
     *  @param name The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& operator()(std::string_view name) noexcept {
        return getUniformFast(name);
    }

    /** @brief Accesses a uniform by name for pointer semantics (C-string overload).
     *  @param name The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& operator()(const char* name) noexcept {
        return getUniform(name);
    }

    /** @brief Accesses a uniform by name for pointer semantics (std::string overload).
     *  @param name The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& operator()(const std::string& name) noexcept {
        return getUniform(name.c_str());
    }

    /** @brief Binds the shader program for rendering. */
    void bind() const noexcept { use(true); }

    /** @brief Unbinds the shader program. */
    void unbind() const noexcept { use(false); }

    /** @brief Uses or unuses the shader program.
     *  @param useShader True to use the shader, false to unuse it.
     */
    void use(bool useShader = true) const noexcept;

    /** @brief Gets the location of an attribute.
     *  @param attributeName The attribute name.
     *  @return Optional containing the attribute location, or nullopt if not found.
     */
    [[nodiscard]] std::optional<unsigned int> getAttributeLocation(std::string_view attributeName) const noexcept;

    /** @brief Gets a uniform by name (backward compatible).
     *  @param uniformName The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& getUniform(const char* uniformName) const noexcept;

    /** @brief Gets a uniform by name (optimized for string_view).
     *  @param uniformName The uniform name.
     *  @return Reference to the Uniform object.
     */
    [[nodiscard]] Uniform& getUniformFast(std::string_view uniformName) const noexcept;

    /** @brief Creates a uniform handle for a given uniform name.
     *  @param uniformName The uniform name.
     *  @return UniformHandle for the specified uniform.
     */
    [[nodiscard]] UniformHandle createUniformHandle(std::string_view uniformName) const noexcept;

    /** @brief Precaches uniforms from an initializer list of string_views.
     *  @param uniformNames List of uniform names to precache.
     */
    void precacheUniforms(std::initializer_list<std::string_view> uniformNames) const noexcept;

    /** @brief Precaches uniforms from an initializer list of C-strings.
     *  @param uniformNames List of uniform names to precache.
     */
    void precacheUniforms(std::initializer_list<const char*> uniformNames) const noexcept;

    /** @brief Precaches uniforms from a vector of C-strings.
     *  @param uniformNames Vector of uniform names to precache.
     */
    void precacheUniforms(const std::vector<const char*>& uniformNames) const noexcept;

    /** @brief Precaches uniforms from any container.
     *  @tparam Container Type of the container holding uniform names.
     *  @param uniformNames Container of uniform names to precache.
     */
    template<typename Container>
    void precacheUniformsFromContainer(const Container& uniformNames) const noexcept;

    /** @brief Checks if the shader is ready for use.
     *  @return True if the shader is ready, false otherwise.
     */
    [[nodiscard]] constexpr bool isReady() const noexcept {
        return program != 0 && isLinked;
    }

    /** @brief Recompiles the shader with new vertex and fragment sources.
     *  @param vertexShader Vertex shader source or file path.
     *  @param fragmentShader Fragment shader source or file path.
     */
    void recompile(std::string_view vertexShader, std::string_view fragmentShader);

    /** @brief Prints active uniforms for debugging. */
    void printActiveUniforms() const noexcept;

    /** @brief Prints active attributes for debugging. */
    void printActiveAttributes() const noexcept;

    /** @brief OpenGL program handle. */
    unsigned int program{0};

private:
    /** @brief Map of uniforms, mutable for lazy initialization. */
    mutable UniformMap uniforms{};
    /** @brief Map of attributes, mutable for lazy initialization. */
    mutable AttributeMap attributes{};
    /** @brief Vector of uniform handles, mutable for lazy initialization. */
    mutable UniformHandleVector uniformHandles{};
    /** @brief Map of uniform handle indices, mutable for lazy initialization. */
    mutable HandleIndexMap uniformHandleMap{};
    /** @brief Cache for shader source files, mutable for lazy initialization. */
    mutable FileCache fileCache{};

    /** @brief Vertex shader handle. */
    unsigned int vshader{0};
    /** @brief Fragment shader handle. */
    unsigned int fshader{0};
    /** @brief Flag indicating if the program is linked. */
    bool isLinked{false};

    /** @brief Prints the log for an OpenGL object (shader or program).
     *  @param obj The OpenGL object handle.
     */
    static void printLog(unsigned int obj) noexcept;

    /** @brief Reads a file's contents.
     *  @param filePath Path to the file.
     *  @return Optional containing the file contents, or nullopt if reading fails.
     */
    [[nodiscard]] std::optional<std::string> readFile(std::string_view filePath) const noexcept;

    /** @brief Checks if a string is a file path.
     *  @param str The string to check.
     *  @return True if the string is a file path, false otherwise.
     */
    [[nodiscard]] static constexpr bool isFilePath(std::string_view str) noexcept;

    /** @brief Compiles a shader.
     *  @param shader The shader handle.
     *  @param source The shader source code.
     *  @param shaderType The type of shader (e.g., "Vertex", "Fragment").
     *  @return True if compilation succeeded, false otherwise.
     */
    [[nodiscard]] bool compileShader(unsigned int shader, std::string_view source, std::string_view shaderType) const noexcept;

    /** @brief Links the shader program.
     *  @return True if linking succeeded, false otherwise.
     */
    [[nodiscard]] bool linkProgram() const noexcept;

    /** @brief Cleans up OpenGL resources. */
    void cleanup() noexcept;

    /** @brief Finds a uniform by searching through a list of names.
     *  @tparam N Size of the name array.
     *  @param names Array of uniform names to search.
     *  @return Pointer to the found Uniform, or nullptr if not found.
     */
    template<size_t N>
    [[nodiscard]] Uniform* findUniform(const std::array<std::string_view, N>& names) const noexcept;
};

/** @class UniformHandle
 *  @brief Lightweight handle to a shader uniform for fast access.
 */
class UniformHandle {
private:
    /** @brief Pointer to the associated Uniform. */
    Uniform* uniform{nullptr};
    friend class Shader;

    /** @brief Constructs a UniformHandle from a Uniform pointer.
     *  @param u Pointer to the Uniform.
     */
    constexpr explicit UniformHandle(Uniform* u) noexcept : uniform(u) {}

public:
    /** @brief Default constructor. */
    constexpr UniformHandle() = default;

    /** @brief Assigns a value to the uniform.
     *  @tparam T Type of the value, must be convertible to UniformValue.
     *  @param val The value to assign.
     *  @return Reference to this UniformHandle.
     */
    template<typename T>
    UniformHandle& operator=(T&& val) noexcept {
        static_assert(std::is_convertible_v<std::decay_t<T>, UniformValue>,
                     "Type must be convertible to UniformValue");

        if (uniform) {
            *uniform = std::forward<T>(val);
        }
        return *this;
    }

    /** @brief Assigns an int value to the uniform.
     *  @param val The int value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(int val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a float value to the uniform.
     *  @param val The float value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(float val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns an unsigned int value to the uniform.
     *  @param val The unsigned int value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(unsigned int val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::vec2 value to the uniform.
     *  @param val The glm::vec2 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::vec2& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::vec3 value to the uniform.
     *  @param val The glm::vec3 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::vec3& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::vec4 value to the uniform.
     *  @param val The glm::vec4 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::vec4& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::ivec2 value to the uniform.
     *  @param val The glm::ivec2 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::ivec2& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::ivec3 value to the uniform.
     *  @param val The glm::ivec3 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::ivec3& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::ivec4 value to the uniform.
     *  @param val The glm::ivec4 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::ivec4& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::mat3 value to the uniform.
     *  @param val The glm::mat3 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::mat3& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Assigns a glm::mat4 value to the uniform.
     *  @param val The glm::mat4 value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& operator=(const glm::mat4& val) noexcept {
        if (uniform) *uniform = val;
        return *this;
    }

    /** @brief Checks if the uniform is smart.
     *  @return True if the uniform is smart, false otherwise.
     */
    [[nodiscard]] bool isSmart() const noexcept {
        return uniform ? uniform->isSmart() : false;
    }

    /** @brief Sets the smart flag for the uniform.
     *  @param smart The smart flag value.
     *  @return Reference to this UniformHandle.
     */
    UniformHandle& setSmart(bool smart) noexcept {
        if (uniform) uniform->setSmart(smart);
        return *this;
    }

    /** @brief Checks if the handle is valid (points to a valid uniform).
     *  @return True if the handle is valid, false otherwise.
     */
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return uniform && uniform->isValid();
    }

    /** @brief Conversion operator to bool, checks if the handle is valid.
     *  @return True if the handle is valid, false otherwise.
     */
    constexpr explicit operator bool() const noexcept {
        return isValid();
    }
};

/** @brief Precaches uniforms from any container type.
 *  @tparam Container Type of the container holding uniform names.
 *  @param uniformNames Container of uniform names to precache.
 */
template<typename Container>
void Shader::precacheUniformsFromContainer(const Container& uniformNames) const noexcept {
    if (!isReady()) return;

    uniformHandles.clear();
    uniformHandleMap.clear();
    uniformHandles.reserve(std::size(uniformNames));

    for (const auto& name : uniformNames) {
        Uniform& uniform = getUniformFast(std::string_view{name});
        if (uniform.isValid()) {
            uniformHandles.emplace_back(std::string{name}, &uniform);
            uniformHandleMap[std::string_view{uniformHandles.back().first}] = uniformHandles.size() - 1;
        }
    }
}

/** @brief Finds a uniform by searching through a list of names.
 *  @tparam N Size of the name array.
 *  @param names Array of uniform names to search.
 *  @return Pointer to the found Uniform, or nullptr if not found.
 */
template<size_t N>
Uniform* Shader::findUniform(const std::array<std::string_view, N>& names) const noexcept {
    if (!isReady()) return nullptr;

    for (const auto& name : names) {
        if (auto iter = uniforms.find(std::string{name}); iter != uniforms.end()) {
            return &iter->second;
        }
    }
    return nullptr;
}

/** @namespace ShaderConstants
 *  @brief Constants used by the Shader class.
 */
namespace ShaderConstants {
    /** @brief Prefix for smart uniforms. */
    constexpr std::string_view SMART_PREFIX = "SMART_";

    /** @brief Valid file extensions for shader files. */
    constexpr std::array<std::string_view, 4> VALID_EXTENSIONS = {
        ".vert", ".frag", ".glsl", ".shader"
    };
}
