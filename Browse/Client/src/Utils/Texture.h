//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstraction of OpenGL texture object.

// Notes
// - sets active slot to 0 at construction / filling (why?)
// - no texture allocated after construction. fill must be called before usage

#ifndef TEXTURE_H_
#define TEXTURE_H_

#include "externals/OGL/gl_core_3_3.h"
#include "src/Utils/glmWrapper.h"
#include <string>
#include <vector>

class Texture
{
public:

    enum class Filter
    {
        NEAREST, LINEAR
    };

    enum class Wrap
    {
        CLAMP, BORDER, MIRROR, REPEAT
    };

    // Constructor
    Texture(
        int width,
        int height,
        GLenum internalFormat,
        Filter filter,
        Wrap wrap);

    // Destructor
    virtual ~Texture();

    // Bind texture to slot for rendering
    void Bind(int slot = 0) const;

    // Fill texture, automatically reallocates if size changes
    virtual void Fill(
        int width,
        int height,
        GLenum inputFormat,
        unsigned char const * pBuffer,
        int unpackAlignment = 4,
        bool forceReallocation = false);

    // Getter for width and height
    int GetWidth() const;
    int GetHeight() const;

    // Getter for aspect ratio
    float GetAspectRatio() const;

    // Get average color in texture (mip map is calculated)
    glm::vec4 GetAverageColor() const;

    // Get pixel data from one mip map level. Returns whether successful
    bool GetPixelsFromMipMap(int layer, int& rWidth, int& rHeight, std::vector<unsigned char>& rData);

	// TODO (Daniel): Experimenting with 'dirty rects' in CefRenderHandle's OnPaint method
	void drawRectangle(int width, int height, int x, int y);


private:

    // Members
    bool _initialized = false;
    GLuint _handle = 0;
    int _width = 0;
    int _height = 0;
    GLenum _internalFormat;
};

#endif // TEXTURE_H_
