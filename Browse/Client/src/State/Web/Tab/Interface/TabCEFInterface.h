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
#include "src/CEF/Data/DOMNodeType.h"
#include "src/CEF/JavaScriptDialogType.h"

// Forward declaration
class Texture;
class DOMNode;
class OverflowElement;
class DOMSelectField;

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
    virtual void AddDOMNode(std::shared_ptr<DOMNode> spNode) = 0;
	virtual void AddDOMNode(std::shared_ptr<DOMSelectField> spNode) = 0;
	virtual std::weak_ptr<DOMNode> GetDOMNode(DOMNodeType type, int nodeID) = 0;
	virtual std::weak_ptr<DOMSelectField> GetDOMSelectFieldNode(int nodeId) = 0;
    virtual void ClearDOMNodes() = 0;
	virtual void RemoveDOMNode(DOMNodeType type, int nodeID) = 0;

    // Receive callbacks from CefMediator upon scrolling offset changes
    virtual void SetScrollingOffset(double x, double y) = 0;

    // Getter for URL
    virtual std::string GetURL() const = 0;

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
    virtual void AddTabAfter(std::string URL) = 0;

	// Receive current loading status of each frame
	virtual void SetLoadingStatus(int64 frameID, bool isMain, bool isLoading) = 0;

	// Overflow elements
	virtual void AddOverflowElement(std::shared_ptr<OverflowElement> overflowElem) = 0;
	virtual std::shared_ptr<OverflowElement> GetOverflowElement(int id) = 0;
	virtual void RemoveOverflowElement(int id) = 0;

	// Tell about JavaScript dialog
	virtual void RequestJSDialog(JavaScriptDialogType type, std::string message) = 0;
};

#endif // TABCEFINTERFACE_H_
