#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include "GL/glew.h"
#include "shader.h"
#include <string>
#include <vector>
#include <unordered_map>

struct Font
{
	std::string fontname;
	FT_Face	face;
	GLuint  font_tex;
};

struct TextRecord
{
	glm::vec2 pos;
	glm::vec2 size;
	std::string text;
	glm::vec4 color1;
	glm::vec4 color2;
	glm::vec2 gradient;
	bool use_gradient;
	glm::mat4 mv;
	glm::mat4 proj;
};


class FontLib
{
	static FT_Library	m_ft;
	std::unordered_map<std::string, Font>::iterator m_curr_font;
	std::unordered_map<std::string, Font> m_fonts;
	Shader				m_font_shader;
	GLuint				m_font_vbo;
	GLuint				m_font_vao;
	GLuint				m_font_res = 64;
	std::vector<TextRecord> m_content;
	
	glm::vec2	  m_canvas;

	void drawText(TextRecord entry);
	
public:
	bool init();
	void submitText(const TextRecord & text);
	void commitText();
	void setCanvas(glm::vec2 sz);
	bool setCurrentFont(std::string fontname);
	
};

