//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Tab interface for creating overlays.

#ifndef TABOVERLAYINTERFACE_H_
#define TABOVERLAYINTERFACE_H_

#include "submodules/eyeGUI/include/eyeGUI.h"
#include <string>
#include <map>
#include <memory>
#include <functional>

class TabOverlayInterface
{
public:

    // Add floating frame from overlay  (not visible after creation)
    virtual int AddFloatingFrameToOverlay(
        std::string brickFilepath,
        float relativePositionX,
        float relativePositionY,
        float relativeSizeX,
        float relativeSizeY,
        std::map<std::string, std::string> idMapper) = 0;
	virtual int AddFloatingFrameToOverlay(
		std::string brickFilepath,
		float relativePositionX,
		float relativePositionY,
		float relativeSizeX,
		float relativeSizeY) = 0;

    // Move floating frame in overlay
    virtual void SetPositionOfFloatingFrameInOverlay(
        int index,
        float relativePositionX,
        float relativePositionY) = 0;

	// Size floating frame in overlay
	virtual void SetSizeOfFloatingFrameInOverlay(
		int index,
		float relativeWidth,
		float relativeHeight) = 0;

    // Set visibility of floating frame in overlay
    virtual void SetVisibilityOfFloatingFrameInOverlay(int index, bool visible) = 0;

    // Remove floating frame from overlay
    virtual void RemoveFloatingFrameFromOverlay(int index) = 0;

    // Register button listener callback in overlay
    virtual void RegisterButtonListenerInOverlay(std::string id, std::function<void(void)> downCallback, std::function<void(void)> upCallback, std::function<void(void)> selectedCallback = []() {}) = 0;

    // Unregister button listener callback in overlay
    virtual void UnregisterButtonListenerInOverlay(std::string id) = 0;

	// Classify button hit
	virtual void ClassifyButton(std::string id, bool accept) = 0;

    // Register keyboard listener in overlay
    virtual void RegisterKeyboardListenerInOverlay(std::string id, std::function<void(std::string)> selectCallback, std::function<void(std::u16string)> pressCallback) = 0;

    // Unregister keyboard listener callback in overlay
    virtual void UnregisterKeyboardListenerInOverlay(std::string id) = 0;

    // Set case of keyboard letters
    virtual void SetCaseOfKeyboardLetters(std::string id, bool upper) = 0;

	// Set keyboard 
	virtual void SetKeymapOfKeyboard(std::string id, unsigned int keymap) = 0;

	// Classify currently selected key
	virtual void ClassifyKey(std::string id, bool accept) = 0;

    // Register word suggest listener in overlay
    virtual void RegisterWordSuggestListenerInOverlay(std::string id, std::function<void(std::u16string)> callback) = 0;

    // Unregister word suggest listener callback in overlay
    virtual void UnregisterWordSuggestListenerInOverlay(std::string id) = 0;

    // Use word suggest to display suggestions. Chosen one can be received by callback. Empty input clears suggestions
    virtual void DisplaySuggestionsInWordSuggest(std::string id, std::u16string input) = 0;

    // Get scrolling offset
    virtual void GetScrollingOffset(double& rScrollingOffsetX, double& rScrollingOffsetY) const = 0;

    // Set content of text block directly or by key
    virtual void SetContentOfTextBlock(std::string id, std::u16string content) = 0;
	virtual void SetContentOfTextBlock(std::string id, std::string key) = 0;

	// Add content in text edit
	virtual void AddContentAtCursorInTextEdit(std::string id, std::u16string content) = 0;

	// Delete content in text edit
	virtual void DeleteContentAtCursorInTextEdit(std::string id, int letterCount) = 0;

	// Delete all content in text edit
	virtual void DeleteContentInTextEdit(std::string id) = 0;

	// Get content in active entity
	virtual std::u16string GetActiveEntityContentInTextEdit(std::string id) const = 0;

	// Set content in active entity
	virtual void SetActiveEntityContentInTextEdit(std::string id, std::u16string content) = 0;

	// Get content of text edit
	virtual std::u16string GetContentOfTextEdit(std::string id) = 0;

	// Move cursor over letters in text edit. Positive letter count means rightward movement, else leftward
	virtual void MoveCursorOverLettersInTextEdit(std::string id, int letterCount) = 0;

	// Move cursor over words in text edit. Positive word count means rightward movement, else leftward
	virtual void MoveCursorOverWordsInTextEdit(std::string id, int wordCount) = 0;

	// Set activity of element
	virtual void SetElementActivity(std::string id, bool active, bool fade) = 0;

	// Tell button to go up
	virtual void ButtonUp(std::string id) = 0;

	// Set global keyboard layout
	virtual void SetKeyboardLayout(eyegui::KeyboardLayout keyboardLayout) = 0;

	// Set space of flow
	virtual void SetSpaceOfFlow(std::string id, float space) = 0;

	// Add brick to stack
	virtual void AddBrickToStack(
		std::string id,
		std::string brickFilepath,
		std::map<std::string, std::string> idMapper) = 0;

	// Getter for values of interest
	virtual int GetWebViewX() const = 0;
	virtual int GetWebViewY() const = 0;
	virtual int GetWebViewWidth() const = 0;
	virtual int GetWebViewHeight() const = 0;
	virtual int GetWebViewResolutionX() const = 0;
	virtual int GetWebViewResolutionY() const = 0;
	virtual int GetWindowWidth() const = 0;
	virtual int GetWindowHeight() const = 0;

	// Use eyeGUI drift map to perform drift correction. Does nothing if USE_EYEGUI_DRIFT_MAP is false
	virtual void ApplyGazeDriftCorrection(float& rPixelX, float& rPixelY) const = 0;
};

#endif // TABOVERLAYINTERFACE_H_
