//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Texture.h"

#include "src/Utils/Helper.h"

Texture::Texture(
    int width,
    int height,
    GLenum internalFormat,
    Filter filter,
    Wrap wrap)
{
    // Initialize members
    _handle = 0;
    _width = width;
    _height = height;
    _internalFormat = internalFormat;

    // Create OpenGL texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &_handle);
    glBindTexture(GL_TEXTURE_2D, _handle);

    // Wrapping
    switch (wrap)
    {
    case Wrap::CLAMP:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    case Wrap::BORDER:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        break;
    case Wrap::MIRROR:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        break;
    case Wrap::REPEAT:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;
    }

    // Filtering (no mip mapping since it would have to be done nearly each frame and zooming out is not a standard behaviour)
    switch (filter)
    {
    case Filter::LINEAR:
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case Filter::NEAREST:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
    // Delete texture
    glDeleteTextures(1, &_handle);
}

void Texture::Bind(int slot) const
{
    // Choose slot.
    glActiveTexture(GL_TEXTURE0 + slot);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, _handle);
}

void Texture::Fill(
    int width,
    int height,
    GLenum inputFormat,
    unsigned char const * pBuffer,
    int unpackAlignment,
    bool forceReallocation)
{
    // Decide whether new allocation on GPU is necessary
    bool subImage = true;
    if(width != _width)
    {
        _width = width;
        subImage = false;
    }
    if(height != _height)
    {
        _height = height;
        subImage = false;
    }

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _handle);

    // Decide how to fill texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment); // set to given value for image transfer
    if(!_initialized || !subImage || forceReallocation)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, _width, _height, 0, inputFormat, GL_UNSIGNED_BYTE, pBuffer);
        _initialized = true;
    }
    else
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, inputFormat, GL_UNSIGNED_BYTE, pBuffer);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // set back to standard

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}

int Texture::GetWidth() const
{
    return _width;
}

int Texture::GetHeight() const
{
    return _height;
}

float Texture::GetAspectRatio() const
{
    return ((float)_width) / ((float)_height);
}

glm::vec4 Texture::GetAverageColor() const
{
	// TODO: depending on internal format...

    // Bind texture and create mip maps
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _handle);
	glGenerateMipmap(GL_TEXTURE_2D);

    // Calculate count of levels
	GLint maxLevel = MaximalMipMapLevel(_width, _height);

    // Get resolulution of max mipmap level (Seems to be 1 all time)
    //GLint width, height;
    //glGetTexLevelParameteriv(GL_TEXTURE_2D, maxLevel, GL_TEXTURE_WIDTH, &width);
    //glGetTexLevelParameteriv(GL_TEXTURE_2D, maxLevel, GL_TEXTURE_HEIGHT, &height);

    // Average color
    glm::vec4 averageColor;

    if(_initialized)
    {
        // Read pixel from GPU
        std::vector<unsigned char> img(4); // since always 1x1 pixels at that mipmap level
        glGetTexImage(GL_TEXTURE_2D, maxLevel, GL_RGBA, GL_UNSIGNED_BYTE, img.data());

        // Return average color
        int r = (int) img[0];
        int g = (int) img[1];
        int b = (int) img[2];
        int a = (int) img[3];
        averageColor = glm::vec4(
            ((float) r) / 255.f,
            ((float) g) / 255.f,
            ((float) b) / 255.f,
            ((float) a) / 255.f);
    }
    else
    {
        averageColor = glm::vec4(1,1,1,1);
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    return averageColor;
}

bool Texture::GetPixelsFromMipMap(int layer, int& rWidth, int& rHeight, std::vector<unsigned char>& rData)
{
	// TODO: depending on internal format

	if (_initialized)
	{
		// Bind texture and create mip maps
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _handle);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Calculate resolution
		layer = glm::clamp(layer, 0, MaximalMipMapLevel(_width, _height) - 1);
		rWidth = _width;
		rHeight = _height;
		for (int i = 0; i < layer; i++)
		{
			rWidth /= 2;
			rHeight /= 2;
		}

		// Resize vector
		rData.resize(rWidth * rHeight * 4);

		// Fill with mip map level content
		glGetTexImage(GL_TEXTURE_2D, layer, GL_RGBA, GL_UNSIGNED_BYTE, rData.data());	
		glBindTexture(GL_TEXTURE_2D, 0);

		return true;
	}

	return false;
}

// Experimenting with "dirty rects" when receiving the page's image from CEF in order to update certain regions
void Texture::drawRectangle(int width, int height, int x, int y)
{
	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _handle);

	unsigned char black = 'A';
	std::vector<glm::vec4> img;
	const int pixels = (width > height) ? width : height;

	for (int i = 0; i < pixels * 4; i++)
		img.push_back(glm::vec4(0,0,0,0));

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, 3, GL_BGRA, GL_UNSIGNED_BYTE, &img[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 3, height, GL_BGRA, GL_UNSIGNED_BYTE, &img[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x+width, y, 3, height, GL_BGRA, GL_UNSIGNED_BYTE, &img[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y+height, width, 3, GL_BGRA, GL_UNSIGNED_BYTE, &img[0]);


	// Unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);

}
