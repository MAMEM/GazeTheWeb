//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Tab interface for methods that may be accessed by CEFMediator. Pixel values
// are taken in screen coordinates and are converted to render coordinates.

#ifndef TABCEFINTERFACE_H_
#define TABCEFINTERFACE_H_

#include <memory>
#include <vector>
#include "src/CEF/Data/Rect.h"
#include "src/Utils/glmWrapper.h"
#include "src/CEF/JavaScriptDialogType.h"
#include "include/cef_browser.h"

// Forward declaration
class Texture;
class DOMTextInput;
class DOMLink;
class DOMSelectField;
class DOMOverflowElement;
class DOMVideo;
class DOMCheckbox;


class TabCEFInterface
{
public:

    // Tell CEF callback which resolution web view texture should have
    virtual void GetWebRenderResolution(int& rWidth, int& rHeight) const = 0;

    // Getter and setter for favicon URL
    virtual std::string GetFavIconURL() const = 0;
    virtual void SetFavIconURL(std::string url) = 0;

    // Setter of URL. Does not load it. Should be called by CefMediator only
    virtual void SetURL(std::string URL) = 0;

    // Setter for can go back / go forward
    virtual void SetCanGoBack(bool canGoBack) = 0;
    virtual void SetCanGoForward(bool canGoForward) = 0;

    // Receive favicon bytes as char vector ordered in RGBA
    virtual void ReceiveFaviconBytes(std::unique_ptr< std::vector<unsigned char> > upData, int width, int height) = 0;
    virtual void ResetFaviconBytes() = 0;

    // Get weak pointer to texture of web view
    virtual std::weak_ptr<Texture> GetWebViewTexture() = 0;

	// Add, remove and update Tab's current DOMNodes
    virtual void AddDOMTextInput(int id) = 0;
	virtual void AddDOMLink(int id) = 0;
	virtual void AddDOMSelectField(int id) = 0;
	virtual void AddDOMOverflowElement(int id) = 0;
	virtual void AddDOMVideo(int id) = 0;
	virtual void AddDOMCheckbox(int id) = 0;

	virtual std::weak_ptr<DOMTextInput> GetDOMTextInput(int id) = 0;
	virtual std::weak_ptr<DOMLink> GetDOMLink(int id) = 0;
	virtual std::weak_ptr<DOMSelectField> GetDOMSelectField(int id) = 0;
	virtual std::weak_ptr<DOMOverflowElement> GetDOMOverflowElement(int id) = 0;
	virtual std::weak_ptr<DOMVideo> GetDOMVideo(int id) = 0;
	virtual std::weak_ptr<DOMCheckbox> GetDOMCheckbox(int id) = 0;

	virtual void RemoveDOMTextInput(int id) = 0;
	virtual void RemoveDOMLink(int id) = 0;
	virtual void RemoveDOMSelectField(int id) = 0;
	virtual void RemoveDOMOverflowElement(int id) = 0;
	virtual void RemoveDOMVideo(int id) = 0;
	virtual void RemoveDOMCheckbox(int id) = 0;
	virtual void ClearDOMNodes() = 0;

	virtual void SetMetaKeywords(std::string content) = 0;

    // Receive callbacks from CefMediator upon scrolling offset changes
    virtual void SetScrollingOffset(double x, double y) = 0;

    // Getter for current zoom level of corresponding browser
    virtual double GetZoomLevel() const = 0;

    // Receive size of current page for scrolling purposes
    virtual void SetPageResolution(double width, double height) = 0;

    // Fixed elements' coordinates
    virtual void AddFixedElementsCoordinates(int id, std::vector<Rect> elements) = 0;
    virtual void RemoveFixedElement(int id) = 0;

    // Set Tab's title text
    virtual void SetTitle(std::string title) = 0;

    // Add new Tab after that one
    virtual void AddTabAfter(std::string URL, CefRefPtr<CefRequestContext> request_context) = 0;

	// Receive current loading status of each frame
	virtual void SetLoadingStatus(bool isLoading, bool isMainFrame) = 0;

	// Tell about JavaScript dialog
	virtual void RequestJSDialog(JavaScriptDialogType type, std::string message) = 0;

	// Mediator checks first if favicon image has to be loaded
	virtual bool IsFaviconAlreadyAvailable(std::string img_url) = 0;

	int _current_favicon_bytes = 0;
	std::vector<std::string> _loaded_favicon_urls;
};

#endif // TABCEFINTERFACE_H_
