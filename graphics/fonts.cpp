
#include "fonts.h"
#include <algorithm>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

#ifdef __APPLE__
#define sggBindVertexArray glBindVertexArrayAPPLE
#define sggGenVertexArrays glGenVertexArraysAPPLE
#else
#define sggBindVertexArray glBindVertexArray
#define sggGenVertexArrays glGenVertexArrays
#endif

const char* __FontVertexShader = R"(
#version 120

attribute vec4 coord;
varying vec2 texcoord;
uniform mat4 projection;
uniform mat4 modelview;

void main(void) {
  gl_Position = projection * modelview * vec4(coord.xy, 0, 1);
  texcoord = coord.zw;
}
)";

const char* __FontFragmentShader = R"(
#version 120

varying vec2 texcoord;
uniform vec4 color1;
uniform vec4 color2;
uniform sampler2D tex;
uniform vec2 gradient;

void main(void) {
	vec4 color = mix( color1, color2, dot(texcoord,gradient));
	gl_FragColor = vec4(1, 1, 1, texture2D(tex, texcoord).r) * color;
}
)";



bool FontLib::init()
{
	if (FT_Init_FreeType(&m_ft))
	{
		return false;
	}
	
	glGetError();
	const GLubyte * s = glewGetErrorString(glGetError());

	m_font_shader = Shader(__FontVertexShader, __FontFragmentShader);
		

	if (!m_font_shader.init())
		return false;
	
	unsigned int attrib_position = m_font_shader.getAttributeLocation("coord");
		
	sggGenVertexArrays(1, &m_font_vao);
	sggBindVertexArray(m_font_vao);
		
	glGenBuffers(1, &m_font_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_font_vbo);
	glEnableVertexAttribArray(attrib_position); 
	glVertexAttribPointer(attrib_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

	GLfloat box[4][4] = {
		{ -1, 1    , 0, 0 },
		{ 1, 1    , 1, 0 },
		{ -1, -1, 0, 1 },
		{ 1, -1, 1, 1 },
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);

	m_curr_font = m_fonts.end();

	return true;
}

void FontLib::submitText(const TextRecord & text)
{
	m_content.push_back(text);
}

void FontLib::drawText(TextRecord entry)
{
	float x = 0.0f;  
	float y = 0.0f;  
	
	const char *p;
	if (m_curr_font == m_fonts.end())
		return;
	Font font = m_curr_font->second;
	FT_GlyphSlot g = font.face->glyph;

#ifndef __APPLE__
	//glFrontFace(GL_CW);
#endif
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, font.font_tex);
	static int c = 0;

	m_font_shader.use();

	m_font_shader["tex"] = 0;
	
	m_font_shader["color1"] = entry.color1;
	m_font_shader["color2"] = (entry.use_gradient? entry.color2 : entry.color1);
	m_font_shader["gradient"] = entry.gradient;
	m_font_shader["projection"] = entry.proj;
	   
	for (p = entry.text.c_str(); *p; p++) {
		if (FT_Load_Char(font.face, *p, FT_LOAD_RENDER))
			continue;
		
		
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			g->bitmap.width,
			g->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			g->bitmap.buffer
			);


		float w = g->bitmap.width;
		float h = g->bitmap.rows; 
		w = entry.size.x * g->bitmap.width / (float)m_font_res;
		h = entry.size.y * g->bitmap.rows / (float)m_font_res;
		float b = g->metrics.horiBearingY/(64* (float)m_font_res)*entry.size.y - h;

		GLfloat box[4][4] = {
			{ x,     y-b    , 0, 1 },
			{ x + w, y-b    , 1, 1 },
			{ x,     y - h - b, 0, 0 },
			{ x + w, y - h -b, 1, 0 },
		};
		
		m_font_shader["modelview"] = glm::translate(glm::vec3(entry.pos.x, entry.pos.y, 0.0f)) * entry.mv;


		sggBindVertexArray(m_font_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_font_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		x += std::max(w+ entry.size.x*0.05f, entry.size.x*0.15f);
		y += entry.size.y*1.1f*(g->advance.y);
	}
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glFrontFace(GL_CCW);
}

void FontLib::commitText()
{
	m_font_shader.use();
	glEnable(GL_SCISSOR_TEST);
	for (auto item : m_content)
	{
		drawText(item);
	}
	m_content.clear();
	
}

void FontLib::setCanvas(glm::vec2 sz)
{
	m_canvas = sz;
}

bool FontLib::setCurrentFont(std::string fontname)
{
	m_curr_font = m_fonts.find(fontname);
	if (m_curr_font != m_fonts.end())
		return true;

	Font font;
	if (FT_New_Face(m_ft, fontname.c_str(), 0, &font.face))
	{
		return false;
	}
	FT_Set_Pixel_Sizes(font.face, 0, m_font_res);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &font.font_tex);
	glBindTexture(GL_TEXTURE_2D, font.font_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	m_curr_font = m_fonts.insert(std::pair<std::string, Font>(fontname,font)).first;
	return true;
}



FT_Library FontLib::m_ft = nullptr;
