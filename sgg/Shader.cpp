#include "headers/Shader.h"

#include <codecvt>
#include <GL/glew.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <set>
#include <sstream>
#include <vector>

Shader::Shader(const char *vertex_str, const char *fragment_str) {
    if (!vertex_str || !fragment_str) {
        std::cerr << "Error: Shader paths cannot be null.\n";
        return;
    }
    const char *vertex = isFilePath(vertex_str) ? readFile(vertex_str) : vertex_str;
    const char *fragment = isFilePath(fragment_str) ? readFile(fragment_str) : fragment_str;

    GLint status;

    // Vertex Shader
    vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vertex, nullptr);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "Error: Vertex shader compilation failed.\n";
        printLog(vshader);
        glDeleteShader(vshader);
        return;
    }

    // Fragment Shader
    fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fragment, nullptr);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "Error: Fragment shader compilation failed.\n";
        printLog(fshader);
        glDeleteShader(vshader);
        glDeleteShader(fshader);
        return;
    }

    // Shader Program
    program = glCreateProgram();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "Error: Shader program linking failed.\n";
        printLog(program);
        glDeleteShader(vshader);
        glDeleteShader(fshader);
        glDeleteProgram(program);
        return;
    }

    // Validate Program
    glValidateProgram(program);
    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "Error: Shader program validation failed.\n";
        printLog(program);
        glDeleteShader(vshader);
        glDeleteShader(fshader);
        glDeleteProgram(program);
        return;
    }

    // Detach and cleanup shaders (no longer needed after linking)
    glDetachShader(program, vshader);
    glDetachShader(program, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
}

Shader::~Shader() {
    glDetachShader(program, vshader);
    glDeleteShader(vshader);
    glDetachShader(program, fshader);
    glDeleteShader(fshader);
}

void Shader::printLog(unsigned int object) {
    GLint log_length = 0;
    if (glIsShader(object)) {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else if (glIsProgram(object)) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else {
        std::cerr << "printLog: Error - Not a shader or a program.\n";
        return;
    }
    if (log_length <= 0) {
        return;
    }
    std::vector<char> log(log_length);
    if (glIsShader(object)) {
        glGetShaderInfoLog(object, log_length, nullptr, log.data());
    } else if (glIsProgram(object)) {
        glGetProgramInfoLog(object, log_length, nullptr, log.data());
    }
    std::cerr << "Shader/Program Log:\n" << log.data() << "\n";
}

const char *Shader::readFile(const char *filePath) {
    std::ifstream fileStream;
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        fileStream.open(filePath);
        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        fileStream.close();
        std::string content = buffer.str();
        char *cstr = strdup(content.c_str());
        return cstr;
    } catch (const std::ifstream::failure &e) {
        std::cerr << "Error reading file: " << filePath << "\n";
        return nullptr;
    }
}

bool Shader::isFilePath(const char *str) {
    if (str == nullptr) return false;
    std::string s(str);
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    if (s.empty() || s.find_last_of('.') == std::string::npos) {
        return false;
    }
    const std::set<std::string> validExtensions = {".vert", ".frag", ".vs", ".fs", ".glsl"};
    std::string extension = s.substr(s.find_last_of('.'));
    return validExtensions.find(extension) != validExtensions.end();
}

void Shader::use(bool use) {
    if (use) {
        glUseProgram(program);
        for (const auto &[name, uniform]: uniforms) {
            if (uniform.isSmart) {
                uniform.apply();
            }
        }
        return;
    }
    glUseProgram(0);
}

unsigned int Shader::getAttributeLocation(const char *attrib) {
    auto [iter, inserted] = attributes.emplace(attrib, glGetAttribLocation(program, attrib));

    if (inserted && iter->second == -1) {
        std::cerr << "Warning: Attribute '" << iter->first
                << "' not found in program " << program << std::endl;
    }
    return iter->second;
}

Uniform &Shader::getUniform(const char *_uniform) {
    // Convert to std::string and determine if it's a "SMART_" uniform
    const std::string prefix = "SMART_";
    const bool isSmart = (strncmp(_uniform, prefix.c_str(), prefix.size()) == 0);
    const std::string uniformName = isSmart ? _uniform + prefix.size() : _uniform;

    // Try to insert the uniform into the map
    auto [iter, inserted] = uniforms.try_emplace(uniformName, glGetUniformLocation(program, uniformName.c_str()));

    // Update the "isSmart" property if needed
    if (inserted) {
        if (iter->second == -1) {  // OpenGL returns -1 if the uniform is not found
            std::cerr << "Warning: Uniform '" << uniformName
                      << "' not found in program " << program << std::endl;
        }
        iter->second.isSmart = isSmart;
    }
    return iter->second;
}

Uniform::Uniform(const Uniform &right) {
    id = right.id;
}

Uniform &Shader::operator[](const char *name) {
    return getUniform(name);
}

Uniform &Uniform::operator=(const Uniform &right) {
    if (this != &right) {
        id = right.id;
        value = right.value;
    }
    return *this;
}

Uniform &Uniform::operator=(int i) {
    value = i;
    glUniform1i(this->id, i);
    return *this;
}

Uniform &Uniform::operator=(float f) {
    value = f;
    glUniform1f(this->id, f);
    return *this;
}

Uniform &Uniform::operator=(unsigned int i) {
    value = i;
    glUniform1ui(this->id, i);
    return *this;
}

Uniform &Uniform::operator=(glm::vec3 v) {
    value = v;
    glUniform3f(this->id, v.x, v.y, v.z);
    int err = glGetError();
    assert(err == GL_NO_ERROR);

    return *this;
}

Uniform &Uniform::operator=(glm::vec4 v) {
    value = v;
    glUniform4f(this->id, v.x, v.y, v.z, v.w);
    return *this;
}

Uniform &Uniform::operator=(glm::vec2 v) {
    value = v;
    glUniform2fv(this->id, 1, &(v[0]));
    return *this;
}

Uniform &Uniform::operator=(glm::ivec3 v) {
    value = v;
    glUniform3iv(this->id, 1, &(v[0]));
    return *this;
}

Uniform &Uniform::operator=(glm::ivec2 v) {
    value = v;
    glUniform2iv(this->id, 1, &(v[0]));
    return *this;
}

Uniform &Uniform::operator=(glm::ivec4 v) {
    value = v;
    glUniform4iv(this->id, 1, &(v[0]));
    return *this;
}

Uniform &Uniform::operator=(glm::mat4 m) {
    value = m;
    glUniformMatrix4fv(this->id, 1, false, &(m[0][0]));
    return *this;
}

Uniform &Uniform::operator=(glm::mat3 m) {
    value = m;
    glUniformMatrix3fv(this->id, 1, false, &(m[0][0]));
    return *this;
}

void Uniform::apply() const {
    if (id == -1) return;
    std::visit([this](auto &&val) {
        using T = std::decay_t<decltype(val)>;
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
    }, value);
};
