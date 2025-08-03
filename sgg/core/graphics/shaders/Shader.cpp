#include "core/graphics/shaders/headers/Shader.h"
#include "core/backend/GLBackend.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

// (applyValue template moved to Shader.h)

Uniform& Uniform::operator=(int val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(float val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(unsigned int val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::vec2& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::vec3& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::vec4& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::ivec2& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::ivec3& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::ivec4& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::mat3& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

Uniform& Uniform::operator=(const glm::mat4& val) noexcept {
    value = val;
    applyValue(val);
    return *this;
}

void Uniform::apply() const noexcept {
    if (id == -1) return;

    std::visit([this](const auto& val) noexcept {
        applyValue(val);
    }, value);
}

// Modern Shader implementation
Shader::Shader(std::optional<std::string_view> vertexShader, std::optional<std::string_view> fragmentShader) {
    if (!GLEW_INIT) {
        std::cerr << "Error: GLEW not initialized\n";
        return;
    }

    if (!vertexShader.has_value() || !fragmentShader.has_value()) {
        std::cerr << "Error: Shader sources must be provided\n";
        return;
    }

    // Modern shader source handling with optional
    const auto vertexSource = isFilePath(vertexShader.value()) ?
        readFile(vertexShader.value()) :
        std::make_optional(std::string{vertexShader.value()});

    const auto fragmentSource = isFilePath(fragmentShader.value()) ?
        readFile(fragmentShader.value()) :
        std::make_optional(std::string{fragmentShader.value()});

    if (!vertexSource.has_value() || !fragmentSource.has_value()) {
        std::cerr << "Error: Failed to read shader sources\n";
        cleanup();
        return;
    }

    // Create and compile shaders with modern error handling
    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    if (!compileShader(vshader, vertexSource.value(), "vertex") ||
        !compileShader(fshader, fragmentSource.value(), "fragment")) {
        cleanup();
        return;
    }

    // Create and link program
    program = glCreateProgram();
    if (program == 0) {
        std::cerr << "Error: Failed to create shader program\n";
        cleanup();
        return;
    }

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);

    if (!linkProgram()) {
        cleanup();
        return;
    }

    isLinked = true;

    // Clean up shaders (RAII style)
    glDetachShader(program, vshader);
    glDetachShader(program, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
    vshader = fshader = 0;

    // Users can create their own uniform handles as needed
}

Shader::Shader(Shader&& other) noexcept
    : uniforms(std::move(other.uniforms))
    , attributes(std::move(other.attributes))
    , uniformHandles(std::move(other.uniformHandles))
    , uniformHandleMap(std::move(other.uniformHandleMap))
    , fileCache(std::move(other.fileCache))
    , vshader(std::exchange(other.vshader, 0))
    , fshader(std::exchange(other.fshader, 0))
    , program(std::exchange(other.program, 0))
    , isLinked(std::exchange(other.isLinked, false)) {
    // Modern move constructor using std::exchange
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        cleanup();

        // Modern move assignment with std::exchange
        uniforms = std::move(other.uniforms);
        attributes = std::move(other.attributes);
        uniformHandles = std::move(other.uniformHandles);
        uniformHandleMap = std::move(other.uniformHandleMap);
        fileCache = std::move(other.fileCache);
        vshader = std::exchange(other.vshader, 0);
        fshader = std::exchange(other.fshader, 0);
        program = std::exchange(other.program, 0);
        isLinked = std::exchange(other.isLinked, false);
    }
    return *this;
}

Shader::~Shader() {
    cleanup();
}

void Shader::cleanup() noexcept {
    if (program != 0) {
        if (vshader != 0) {
            glDetachShader(program, vshader);
            glDeleteShader(vshader);
        }
        if (fshader != 0) {
            glDetachShader(program, fshader);
            glDeleteShader(fshader);
        }
        glDeleteProgram(program);
    }

    // Modern cleanup with proper initialization
    program = vshader = fshader = 0;
    isLinked = false;
    uniforms.clear();
    attributes.clear();
    uniformHandles.clear();
    uniformHandleMap.clear();
}

bool Shader::compileShader(unsigned int shader, std::string_view source, std::string_view shaderType) const noexcept {
    const char* sourcePtr = source.data();
    const GLint sourceLength = static_cast<GLint>(source.length());

    glShaderSource(shader, 1, &sourcePtr, &sourceLength);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "Error: " << shaderType << " shader compilation failed\n";
        printLog(shader);
        glDeleteShader(shader);
        return false;
    }
    return true;
}

bool Shader::linkProgram() const noexcept {
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "Error: Shader program linking failed\n";
        printLog(program);
        return false;
    }

    // Validate program
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "Error: Shader program validation failed\n";
        printLog(program);
        return false;
    }

    return true;
}

void Shader::use(bool useShader) const noexcept {
    if (useShader && isReady()) {
        glUseProgram(program);

        // Apply smart uniforms using modern range-based loop
        for (const auto& [name, uniform] : uniforms) {
            if (uniform.isSmart()) {
                uniform.apply();
            }
        }
    } else {
        glUseProgram(0);
    }
}

std::optional<unsigned int> Shader::getAttributeLocation(std::string_view attributeName) const noexcept {
    if (!isReady()) return std::nullopt;

    const std::string name{attributeName};
    auto [iter, inserted] = attributes.try_emplace(name, glGetAttribLocation(program, name.c_str()));

    if (inserted && iter->second == static_cast<unsigned int>(-1)) {
        std::cerr << "Warning: Attribute '" << name
                 << "' not found in shader program " << program << std::endl;
        return std::nullopt;
    }

    return iter->second;
}

Uniform& Shader::getUniform(const char* uniformName) const noexcept {
    if (!isReady()) {
        static Uniform invalidUniform{};
        return invalidUniform;
    }

    // Modern string_view construction for better performance
    return getUniformFast(std::string_view{uniformName});
}

Uniform& Shader::getUniformFast(std::string_view uniformName) const noexcept {
    if (!isReady()) {
        static Uniform invalidUniform{};
        return invalidUniform;
    }

    // Check handle map first for ultra-fast access
    if (auto handleIter = uniformHandleMap.find(uniformName);
        handleIter != uniformHandleMap.end()) {
        return *uniformHandles[handleIter->second].second;
    }

    // Legacy support: Check for SMART_ prefix for backward compatibility
    const bool isSmartLegacy = uniformName.size() >= ShaderConstants::SMART_PREFIX.size() &&
                               uniformName.substr(0, ShaderConstants::SMART_PREFIX.size()) == ShaderConstants::SMART_PREFIX;

    const std::string name = isSmartLegacy ?
        std::string{uniformName.substr(ShaderConstants::SMART_PREFIX.size())} :
        std::string{uniformName};

    auto [iter, inserted] = uniforms.try_emplace(name, glGetUniformLocation(program, name.c_str()));

    if (inserted) {
        if (iter->second.id == -1) {
            std::cerr << "Warning: Uniform '" << name
                     << "' not found in shader program " << program << std::endl;
        }
        // Legacy support: Auto-set smart flag if SMART_ prefix was used
        if (isSmartLegacy) {
            iter->second.smartFlag = true;
        }
    }

    return iter->second;
}

UniformHandle Shader::createUniformHandle(std::string_view uniformName) const noexcept {
    Uniform& uniform = getUniformFast(uniformName);
    return UniformHandle{&uniform};
}

void Shader::precacheUniforms(std::initializer_list<const char*> uniformNames) const noexcept {
    if (!isReady()) return;

    uniformHandles.clear();
    uniformHandleMap.clear();
    uniformHandles.reserve(uniformNames.size());

    for (const char* name : uniformNames) {
        Uniform& uniform = getUniform(name);
        if (uniform.isValid()) {
            uniformHandles.emplace_back(name, &uniform);
            uniformHandleMap[std::string_view{uniformHandles.back().first}] = uniformHandles.size() - 1;
        }
    }
}

void Shader::recompile(std::string_view vertexShader, std::string_view fragmentShader) {
    cleanup();

    // Modern shader source handling with optional
    const auto vertexSource = isFilePath(vertexShader) ?
        readFile(vertexShader) :
        std::make_optional(std::string{vertexShader});

    const auto fragmentSource = isFilePath(fragmentShader) ?
        readFile(fragmentShader) :
        std::make_optional(std::string{fragmentShader});

    if (!vertexSource.has_value() || !fragmentSource.has_value()) {
        std::cerr << "Error: Failed to read shader sources during recompilation\n";
        return;
    }

    // Create and compile shaders with modern error handling
    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    if (!compileShader(vshader, vertexSource.value(), "vertex") ||
        !compileShader(fshader, fragmentSource.value(), "fragment")) {
        cleanup();
        return;
    }

    // Create and link program
    program = glCreateProgram();
    if (program == 0) {
        std::cerr << "Error: Failed to create shader program during recompilation\n";
        cleanup();
        return;
    }

    glAttachShader(program, vshader);
    glAttachShader(program, fshader);

    if (!linkProgram()) {
        cleanup();
        return;
    }

    isLinked = true;

    // Clean up shaders (RAII style)
    glDetachShader(program, vshader);
    glDetachShader(program, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
    vshader = fshader = 0;
}

std::optional<std::string> Shader::readFile(std::string_view filePath) const noexcept {
    try {
        std::ifstream file{std::string{filePath}};
        if (!file.is_open()) {
            std::cerr << "Error: Could not open shader file: " << filePath << std::endl;
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } catch (const std::exception& e) {
        std::cerr << "Error reading shader file " << filePath << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

constexpr bool Shader::isFilePath(std::string_view str) noexcept {
    // Check if string contains file extension indicators
    if (str.empty()) return false;

    return std::any_of(ShaderConstants::VALID_EXTENSIONS.begin(),
                       ShaderConstants::VALID_EXTENSIONS.end(),
                       [str](std::string_view ext) noexcept {
                           return str.size() >= ext.size() &&
                                  str.substr(str.size() - ext.size()) == ext;
                       });
}

void Shader::printLog(unsigned int obj) noexcept {
    GLint logLength = 0;
    if (glIsShader(obj)) {
        glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &logLength);
    } else {
        glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &logLength);
    }

    if (logLength > 0) {
        std::vector<char> log(static_cast<size_t>(logLength));
        if (glIsShader(obj)) {
            glGetShaderInfoLog(obj, logLength, nullptr, log.data());
        } else {
            glGetProgramInfoLog(obj, logLength, nullptr, log.data());
        }
        std::cerr << log.data() << std::endl;
    }
}

void Shader::printActiveUniforms() const noexcept {
    if (!isReady()) {
        std::cout << "Shader not ready\n";
        return;
    }

    GLint count;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    std::cout << "Active uniforms (" << count << "):\n";

    // Modern loop with proper types
    for (GLint i = 0; i < count; ++i) {
        std::array<char, 256> name{};
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveUniform(program, static_cast<GLuint>(i), name.size(),
                          &length, &size, &type, name.data());
        std::cout << "  " << name.data()
                  << " (location: " << glGetUniformLocation(program, name.data()) << ")\n";
    }
}

void Shader::printActiveAttributes() const noexcept {
    if (!isReady()) {
        std::cout << "Shader not ready\n";
        return;
    }

    GLint count;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);
    std::cout << "Active attributes (" << count << "):\n";

    // Modern loop with proper types
    for (GLint i = 0; i < count; ++i) {
        std::array<char, 256> name{};
        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveAttrib(program, static_cast<GLuint>(i), name.size(),
                         &length, &size, &type, name.data());
        std::cout << "  " << name.data()
                  << " (location: " << glGetAttribLocation(program, name.data()) << ")\n";
    }
}
