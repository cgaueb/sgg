#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "glm/glm.hpp"

#define ASSERT_GL {assert (glGetError()==GL_NO_ERROR);}

class Uniform
{
public:
	int id = -1;
	using ValueType = std::variant<
		int,
		float,
		unsigned int,
		glm::vec2,
		glm::vec3,
		glm::vec4,
		glm::ivec2,
		glm::ivec3,
		glm::ivec4,
		glm::mat3,
		glm::mat4
	>;
	ValueType value;
public:
	operator int() { return id; }
	Uniform(int i) : id(i) {}
	Uniform() : id(-1) {}
	Uniform(const Uniform & right);
	Uniform & operator = (const Uniform & right);
	Uniform & operator = (int);
	Uniform & operator = (float);
	Uniform & operator = (unsigned int);
	Uniform & operator = (glm::vec3);
	Uniform & operator = (glm::vec4);
	Uniform & operator = (glm::vec2);
	Uniform & operator = (glm::ivec3);
	Uniform & operator = (glm::ivec2);
	Uniform & operator = (glm::ivec4);
	Uniform & operator = (glm::mat4);
	Uniform & operator = (glm::mat3);
	bool isSmart = false;
	void apply() const;
};

class Shader
{
	std::map<std::string, Uniform> uniforms;
	std::map<std::string, unsigned int> attributes;
	unsigned int vshader, fshader;

protected:
	static void printLog(unsigned int obj);
	static const char *readFile(const char *str);
	static bool isFilePath(const char *str);

public:
	unsigned int program;
	Shader(const char * vertex = nullptr, const char * fragment = nullptr);
	Uniform & operator [] (const char * name);
	void use(bool use = true);
	unsigned int getAttributeLocation(const char * attrib);
	Uniform& getUniform(const char *uniform);
	~Shader();
};
