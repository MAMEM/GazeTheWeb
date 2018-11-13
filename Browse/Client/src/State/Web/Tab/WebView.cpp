//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "WebView.h"
#include "src/Utils/Texture.h"
#include "src/Setup.h"
#include "submodules/glm/glm/gtc/matrix_transform.hpp"

// Shaders
const std::string vertexShaderSource =
"#version 330 core\n"
"void main() {\n"
"}\n";

const std::string geometryShaderSource =
"#version 330 core\n"
"layout(points) in;\n"
"layout(triangle_strip, max_vertices = 4) out;\n"
"out vec2 uv;\n"
"out vec2 pos;\n" // relative position within quad in OpenGL space
"out vec2 size;\n" // size of quad (relative values)
"uniform vec4 position;\n" // minX, minY, maxX, maxY. OpenGL coordinate system!
"uniform vec4 textureCoordinate;\n" // minU, minV, maxU, maxV. OpenGL coordinate system!
"void main() {\n"
"    size = vec2(position.z - position.x, position.w - position.y);\n" // relative size of quad
"    gl_Position = vec4(position.zw, 0.0, 1.0);\n" // upper right corner
"    uv = vec2(textureCoordinate.zw);\n"
"    pos = vec2(1,1);\n"
"    EmitVertex();\n"
"    gl_Position = vec4(position.xw, 0.0, 1.0);\n" // upper left corner
"    uv = vec2(textureCoordinate.xw);\n"
"    pos = vec2(0,1);\n"
"    EmitVertex();\n"
"    gl_Position = vec4(position.zy, 0.0, 1.0);\n" // lower right corner
"    uv = vec2(textureCoordinate.zy);\n"
"    pos = vec2(1,0);\n"
"    EmitVertex();\n"
"    gl_Position = vec4(position.xy, 0.0, 1.0);\n" // lower left corner
"    uv = vec2(textureCoordinate.xy);\n"
"    pos = vec2(0,0);\n"
"    EmitVertex();\n"
"    EndPrimitive();\n"
"}\n";

const std::string webpageFragmentShaderSource =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 fragColor;\n"
"uniform sampler2D tex;\n"
"uniform float dim;\n"
"void main() {\n"
"	vec4 color = texture(tex, uv);\n"
"   color.rgb = (color.a * color.rgb) + (1.0 - color.a);" // white background
"   fragColor = vec4(color.rgb * (1.0 - dim), 1.0);\n"
"}\n";

const std::string highlightFragmentShaderSource =
"#version 330 core\n"
"in vec2 uv;\n"
"in vec2 pos;\n"
"in vec2 size;\n"
"out vec4 fragColor;\n"
"uniform sampler2D tex;\n"
"uniform float dim;\n"
"uniform float aspectRatio;\n"
"void main() {\n"
"	vec4 color = texture(tex, uv);\n"
"   color.rgb = (color.a * color.rgb) + (1.0 - color.a);" // white background
"	vec2 circle = vec2(0.015, 0.015);\n" // inner circle to display
"	circle.y *= aspectRatio;\n" // aspect ratio correction
"   circle /= size;\n" // realtive size of circle within mesh
"	vec2 halfCircle = circle/2;\n" // half circle necessary
"	float inside = min(length(abs(pos - 0.5) / halfCircle),1);\n" // calculate whether pos inside circle to display
"   inside = inside * inside;\n" // make circle stronger
"   inside = inside * inside;\n" // make circle stronger, again
"   inside = inside * inside;\n" // make circle stronger, again
"   vec3 mixture = mix(vec3(1, 0, 0), color.rgb * (1.0 - dim), min(inside + 0.4, 1));\n" // mix overlay circle and normal color
"   fragColor = vec4(mixture, 1.0);\n"
"}\n";

const std::string compositionFragmentShaderSource =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 fragColor;\n"
"uniform sampler2D tex;\n"
"uniform vec2 centerOffset;\n"
"uniform vec2 zoomPosition;\n"
"uniform float zoom;\n"
"void main() {\n"
"   vec2 coords = uv;\n"
"   coords += centerOffset;" // move towards center
"   coords -= zoomPosition;" // move zoom position to origin
"   coords *= zoom;" // scale coords
"   coords += zoomPosition;" // move it back
"   fragColor = texture(tex, coords);\n"
"}\n";

WebView::WebView(int x, int y, int width, int height)
{
	// Set members
	_x = x;
	_y = y;
	_width = width;
	_height = height;

    // Generate texture
    _spTexture = std::shared_ptr<Texture>(new Texture(_width, _height, GL_RGBA, Texture::Filter::LINEAR, Texture::Wrap::BORDER));

    // Render items
	_upWebpageRenderItem = std::unique_ptr<RenderItem>(new RenderItem(vertexShaderSource, geometryShaderSource, webpageFragmentShaderSource));
    _upHighlightRenderItem = std::unique_ptr<RenderItem>(new RenderItem(vertexShaderSource, geometryShaderSource, highlightFragmentShaderSource));
    _upCompositeRenderItem = std::unique_ptr<RenderItem>(new RenderItem(vertexShaderSource, geometryShaderSource, compositionFragmentShaderSource));

    // Framebuffer
    _upFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(_width, _height));
    _upFramebuffer->Bind();
    _upFramebuffer->AddAttachment(Framebuffer::ColorFormat::RGB, true);
    _upFramebuffer->Unbind();
}

WebView::~WebView()
{
    // Nothing to do
}

void WebView::Update(
    int x,
    int y,
    int width,
    int height)
{
    // Update framebuffer size
    if((width != _width) || (height != _height))
    {
        _upFramebuffer->Bind();
        _upFramebuffer->Resize(width, height);
        _upFramebuffer->Unbind();
    }

    // Set members
    _x = x;
    _y = y;
    _width = width;
    _height = height;
}

void WebView::Draw(
	const WebViewParameters& parameters,
	int windowWidth,
	int windowHeight,
	double scrollingOffsetX,
	double scrollingOffsetY) const
{
    // ### FILL FRAMEBUFFER ###

    // Just render to framebuffer
    _upFramebuffer->Bind();

    // Rescue current viewport and set own which fits rendered webpage
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, _width, _height);

    // Bind render item for web page
	_upWebpageRenderItem->Bind();

    // Bind texture with rendered web page
    _spTexture->Bind();

    // Fill uniforms
	_upWebpageRenderItem->GetShader()->UpdateValue("position", glm::vec4(-1.f, -1.f, 1.f, 1.f)); // normalized device coordinates
	_upWebpageRenderItem->GetShader()->UpdateValue("textureCoordinate", glm::vec4(0.f, 1.f, 1.f, 0.f)); // using texture coordinates to flip image in v direction
	_upWebpageRenderItem->GetShader()->UpdateValue("dim", parameters.dim);

    // Draw webpage completely into framebuffer
	_upWebpageRenderItem->Draw(GL_POINTS);

    // Render highlighting
    if(parameters.dim > 0.f)
    {
		// Bind render item for highlighting
		_upHighlightRenderItem->Bind();

        // TODO: use value from highlight or so
        // For now: just reset dimming to zero for the rect rendering
		_upHighlightRenderItem->GetShader()->UpdateValue("dim", 0.f);

		// Aspect ratio of web view
		_upHighlightRenderItem->GetShader()->UpdateValue("aspectRatio", (float)_width / (float)_height);

        // Go over rects and render them
        for(Rect rect : _rects)
        {
			// Move rect by scrolling
			rect.left -= scrollingOffsetX;
			rect.right -= scrollingOffsetX;
			rect.bottom -= scrollingOffsetY;
			rect.top -= scrollingOffsetY;

			// Scale from web view resolution to real one
			rect.left = (rect.left / (float)GetResolutionX()) * (float)_width;
			rect.right = (rect.right / (float)GetResolutionX()) * (float)_width;
			rect.bottom = (rect.bottom / (float)GetResolutionY()) * (float)_height;
			rect.top = (rect.top / (float)GetResolutionY()) * (float)_height;

            // Setup position
			_upHighlightRenderItem->GetShader()->UpdateValue(
                "position",
                glm::vec4(
                    (((float)rect.left / (float)_width) * 2.f) - 1.f,
                    ((((float)(_height - rect.bottom)) / (float)_height) * 2.f) - 1.f,
                    (((float)(rect.right) / (float)_width) * 2.f) - 1.f,
                    ((((float)(_height - rect.top)) / (float)_height) * 2.f) - 1.f)); // normalized device coordinates

            // Setup texture coordinate
			_upHighlightRenderItem->GetShader()->UpdateValue(
                "textureCoordinate",
                glm::vec4(
                    (float)rect.left / (float)_width,
                    1.f - (float)(_height - rect.bottom) / (float)_height,
                    (float)rect.right / (float)_width,
                    1.f - (float)(_height - rect.top) / (float)_height)); // using texture coordinates to flip image in v direction

            // Draw the quad
			_upHighlightRenderItem->Draw(GL_POINTS);
        }
    }

    // Restore old viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    // Unbind framebuffer
    _upFramebuffer->Unbind();

    // ### COMPOSITING INCLUSIVE ZOOMING ###

    // Render composited inclusive zooming
    _upCompositeRenderItem->Bind();

    // Bind texture with webpage and custom rendered elements
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _upFramebuffer->GetAttachment(0));

    // Fill uniforms (TODO: here, coordinate sytem is not completely correctly translated. Would be only a problem at vertical transformation)
    _upCompositeRenderItem->GetShader()->UpdateValue(
        "position",
        glm::vec4(
            ((_x / (float)windowWidth) * 2.f) - 1.f, // minX
            ((_y / (float)windowHeight) * 2.f) - 1.f, // minY
            (((_x + _width) / (float)windowWidth) * 2.f) - 1.f, // maxX
            (((_y + _height) / (float)windowHeight) * 2.f) - 1.f // maxY
            )); // normalized device coordinates
    _upCompositeRenderItem->GetShader()->UpdateValue("textureCoordinate", glm::vec4(0.f, 0.f, 1.f, 1.f)); // everything is rendered correctly into framebuffer, just display it
    _upCompositeRenderItem->GetShader()->UpdateValue("centerOffset", glm::vec2(parameters.centerOffset.x, -parameters.centerOffset.y)); // center offset y has to be taken negative because OpenGL coordinates
    _upCompositeRenderItem->GetShader()->UpdateValue("zoomPosition", glm::vec2(parameters.zoomPosition.x, 1.f - parameters.zoomPosition.y)); // zoomPosition has origin in upper left but lower left is necessary
    _upCompositeRenderItem->GetShader()->UpdateValue("zoom", parameters.zoom);
    _upCompositeRenderItem->Draw(GL_POINTS);
}

std::weak_ptr<Texture> WebView::GetTexture()
{
    return _spTexture;
}

void WebView::SetHighlightRects(std::vector<Rect> rects)
{
    _rects = rects;
}

int WebView::GetX() const
{
	return _x;
}

int WebView::GetY() const
{
	return _y;
}

int WebView::GetWidth() const
{
	return _width;
}

int WebView::GetHeight() const
{
	return _height;
}

int WebView::GetResolutionX() const
{
	return _spTexture->GetWidth();
}

int WebView::GetResolutionY() const
{
	return _spTexture->GetHeight();
}
