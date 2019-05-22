//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Pixels of rendered webpages are received here and passed to the
// corresponding WebView.

#ifndef CEF_RENDERER_H_
#define CEF_RENDERER_H_

#include "include/cef_client.h"
#include "include/cef_render_handler.h"

// Forward declaration
class Texture;
class Mediator;

// Does offscreen rendering
class Renderer : public CefRenderHandler
{
public:

	// Constructor
    Renderer(Mediator* pMediator);

    // Called by CEF to determine render size
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) OVERRIDE;

    // Called when paint happens, copy pixels over RAM to texture
    void OnPaint(
        CefRefPtr<CefBrowser> browser,
        PaintElementType type,
        const RectList &dirtyRects,
        const void *buffer,
        int width,
        int height) OVERRIDE;

    // Called when scrolling offset changes
    void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y) OVERRIDE;

private:

    // Members
    Mediator* _mediator;

    // Include CEF'S default reference counting implementation
    IMPLEMENT_REFCOUNTING(Renderer);
};

#endif // CEF_RENDERER_H_
