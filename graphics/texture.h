#pragma once
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace graphics
{
	class Texture
	{
	private:
		GLuint m_id = 0;
		std::string	m_filename;
		unsigned int m_width, m_height;
		unsigned int m_channels;
		std::vector<unsigned char> m_buffer;
		bool m_ready = false;
		void makePowerOfTwo();
		bool load(const std::string & file);
		void buildGLTexture();
	public:
		Texture(const std::string & filename);
		GLuint getID() { return m_id; }
		int getWidth() { return m_width; }
		int getHeight() { return m_height; }
		
	};

	class TextureManager
	{
	private:
		std::unordered_map<std::string, Texture> textures;
	public:
		GLuint getTexture(std::string file);
	};
}
