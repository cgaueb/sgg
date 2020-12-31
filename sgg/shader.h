#pragma once

#include <map>
#include <string>
#include "glm/glm.hpp"

#define ASSERT_GL {assert (glGetError()==GL_NO_ERROR);}

class Uniform
{
protected:
	int id;
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
};




class Shader
{
	std::map<const char *, Uniform> uniforms;
	bool ready = false;
	
	
	unsigned int vshader, fshader;

protected:
	void printLog(unsigned int obj);

public:
	unsigned int program;
	explicit operator bool () const { return ready; }
	Shader(const char * vertex = nullptr, const char * fragment = nullptr);
	Uniform & operator [] (const char * name);
	bool use(bool use = true);
	std::string loadShaderText(char * file);
	bool init();
	void setFragmentLocation(char * name, unsigned int location);
	unsigned int getAttributeLocation(const char * attrib);
	~Shader();
};
