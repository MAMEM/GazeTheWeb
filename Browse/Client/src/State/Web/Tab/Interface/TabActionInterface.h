//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Tab interface for manipulation from actions.

#ifndef TABACTIONINTERFACE_H_
#define TABACTIONINTERFACE_H_

#include "src/State/Web/Tab/WebViewParameters.h"
#include "src/Input/Filters/CustomTransformationInteface.h"
#include "src/Typedefs.h"
#include <memory>
#include <string>

// Forward declaration
class Pipeline;
class DOMNode;

class TabActionInterface
{
public:


    // Push back a pipeline
    virtual void PushBackPipeline(std::unique_ptr<Pipeline> upPipeline) = 0;

    // Emulate click in tab. Optionally converts WebViewPixel position to CEFpixel position before calling CEF method
    virtual void EmulateLeftMouseButtonClick(double x, double y, bool visualize = true, bool isWebViewPixelCoordinate = true, bool userTriggered = false) = 0;

	// Emulate mouse cursor in tab. Optionally converts WebViewPixel position to CEFpixel position before calling CEF method. Optional offset in rendered pixels
	virtual void EmulateMouseCursor(double x, double y, bool leftButtonPressed = false, bool isWebViewPixelCoordinate = true, double xOffset = 0, double yOffset = 0) = 0;

    // Emulate mouse wheel scrolling
    virtual void EmulateMouseWheelScrolling(double deltaX, double deltaY) = 0;

	// Emulate left mouse button down. Can be used to start text selection. Optional offset in rendered pixels
	virtual void EmulateLeftMouseButtonDown(double x, double y, bool isWebViewPixelCoordinate = true, double xOffset = 0, double yOffset = 0) = 0;

	// Emulate left mouse button up. Can be used to end text selection. Optional offset in rendered pixels
	virtual void EmulateLeftMouseButtonUp(double x, double y, bool isWebViewPixelCoordinate = true, double xOffset = 0, double yOffset = 0) = 0;

	// Asynchronous javascript call
	virtual void PutTextSelectionToClipboardAsync() = 0;

	// Get text out of global clipboard in mediator
	virtual std::string GetClipboardText() const = 0;

    // Get distance to next link and weak pointer to it. Returns empty weak pointer if no link available. Distance in page pixels
    virtual std::weak_ptr<const DOMNode> GetNearestLink(glm::vec2 pagePixelCoordinate, float& rDistance) const = 0;

	// Convert WebViewPixel coordinate to CEFPixel coordinate
	virtual void ConvertToCEFPixel(double& rWebViewPixelX, double& rWebViewPixelY) const = 0;

	// Convert CEFPixel coordinate to WebViewPixel coordinate
	virtual void ConvertToWebViewPixel(double& rCEFPixelX, double& rCEFPixelY) const = 0;

	// Reply JavaScript dialog callback
	virtual void ReplyJSDialog(bool clickedOk, std::string userInput) = 0;

	// Play sound
	virtual void PlaySound(std::string filepath) = 0;

	// Get interface for custom transformations of input
	virtual std::weak_ptr<CustomTransformationInterface> GetCustomTransformationInterface() const = 0;

	// Notify about text input
	virtual void NotifyTextInput(std::string tag, std::string id, int charCount, int charDistance, float x, float y, float duration) = 0;

    // ### METHODS WHICH SET PARAMETERS THAT MUST BE RESET WHEN NO PIPELINE / ACTION IS ACTIVE ###

    // Reset method (called by pipeline at destruction, finish and abort)
    void Reset()
    {
        SetWebViewParameters(WebViewParameters());
    }

    // Set WebViewParameters for web view
	virtual void SetWebViewParameters(WebViewParameters parameters) = 0;

	virtual void NotifyKeyboardActivation(bool keyboardActive) = 0;

protected:

};

#endif // TABACTIONINTERFACE_H_
