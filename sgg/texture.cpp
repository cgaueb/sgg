#include <sgg/texture.h>
#include <sgg/lodepng.h>
#include <vector>
#include <cmath>

void graphics::Texture::makePowerOfTwo()
{
	// Texture size must be power of two for the primitive OpenGL version this is written for. Find next power of two.
	int u2 = 1; while (u2 < m_width) u2 *= 2;
	int v2 = 1; while (v2 < m_height) v2 *= 2;
	if (u2 == m_width && v2 == m_height)
		return;

	// Make power of two version of the image.
	std::vector<unsigned char> image2(u2 * v2 * 4);
	for (int i = 0; i < v2; i++)
		for (int j = 0; j < u2; j++)
		{
			float x = (m_width-1) * j / (float)u2;
			float y = (m_height-1) * i / (float)v2;
			int x_L = (int)floorf(x);
			int x_U = (int)ceilf(x);
			int y_L = (int)floorf(y);
			int y_U = (int)ceilf(y);
			float sx = x - x_L;
			float sy = y - y_L;

			for (int c = 0; c < 4; c++)
			{
				float valx1 = (1.0f - sx) * m_buffer[4 * (y_L*m_width + x_L) + c] + sx * m_buffer[4 * (y_L*m_width + x_U) + c];
				float valx2 = (1.0f - sx) * m_buffer[4 * (y_U*m_width + x_L) + c] + sx * m_buffer[4 * (y_U*m_width + x_U) + c];
				float valy = (1.0f - sy) * valx1 + sy * valx2;
				image2[4 * (i*u2 + j) + c] = (unsigned char)roundf(valy);
			}
		}
	
	m_buffer = std::move(image2);
	m_width = u2;
	m_height = v2;
}

bool graphics::Texture::load(const std::string & file)
{
	unsigned int error = lodepng::decode(m_buffer, m_width, m_height, file.c_str());
	return !error;
}

void graphics::Texture::buildGLTexture()
{
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); 
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &m_buffer[0]);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void graphics::Texture::updateGLTexture()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, &m_buffer[0]);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

graphics::Texture::Texture(const std::string & filename)
	: m_channels(4), m_height(0), m_width(0)
{
	m_filename = filename;

	if (filename[0] == '@')
	{
		// The user requested an empty texture initialization
		size_t pos = filename.find(':');
		if (pos == std::string::npos)
			return;
		size_t delim = filename.find('X');

		std::string sz = filename.substr(pos + 1, delim - pos - 1);
		m_width = std::stoi(sz);
		sz = filename.substr(delim + 1);
		m_height = std::stoi(sz);
		m_buffer.resize(4*m_height*m_width);
	}
	else
	{
		if (!load(filename))
			return;
	}

	makePowerOfTwo();
	buildGLTexture();
}

GLuint graphics::TextureManager::getTexture(const std::string & file)
{
	auto iter = textures.find(file);
	if (iter != textures.end())
	{
		return iter->second.getID();
	}
	else
	{
		std::pair<std::string, Texture> texinfo = std::make_pair(file, Texture(file));
		textures.insert(texinfo);
		return texinfo.second.getID();
	}
	
}

graphics::Texture * graphics::TextureManager::getTextureObject(const std::string & file)
{
	auto iter = textures.find(file);
	if (iter != textures.end())
	{
		return &(iter->second);
	}
	else
	{
		return nullptr;
	}

}
