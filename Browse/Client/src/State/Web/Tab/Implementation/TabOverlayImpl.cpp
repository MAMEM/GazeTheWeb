//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/State/Web/Tab/Tab.h"
#include "src/Master/Master.h"
#include "src/Utils/Logger.h"

int Tab::AddFloatingFrameToOverlay(
	std::string brickFilepath,
	float relativePositionX,
	float relativePositionY,
	float relativeSizeX,
	float relativeSizeY,
	std::map<std::string, std::string> idMapper)
{
	return eyegui::addFloatingFrameWithBrick(
		_pOverlayLayout,
		brickFilepath,
		relativePositionX,
		relativePositionY,
		relativeSizeX,
		relativeSizeY,
		idMapper,
		false,
		false);
}

int Tab::AddFloatingFrameToOverlay(
	std::string brickFilepath,
	float relativePositionX,
	float relativePositionY,
	float relativeSizeX,
	float relativeSizeY)
{
	return eyegui::addFloatingFrameWithBrick(
		_pOverlayLayout,
		brickFilepath,
		relativePositionX,
		relativePositionY,
		relativeSizeX,
		relativeSizeY,
		false,
		false);
}

void Tab::SetPositionOfFloatingFrameInOverlay(
	int index,
	float relativePositionX,
	float relativePositionY)
{
	eyegui::setPositionOfFloatingFrame(_pOverlayLayout, index, relativePositionX, relativePositionY);
}

void Tab::SetSizeOfFloatingFrameInOverlay(
	int index,
	float relativeWidth,
	float relativeHeight)
{
	eyegui::setSizeOfFloatingFrame(_pOverlayLayout, index, relativeWidth, relativeHeight);
}

void Tab::SetVisibilityOfFloatingFrameInOverlay(int index, bool visible)
{
	// Reset when becoming visible
	eyegui::setVisibilityOFloatingFrame(_pOverlayLayout, index, visible, visible, true);
}

void Tab::RemoveFloatingFrameFromOverlay(int index)
{
	eyegui::removeFloatingFrame(_pOverlayLayout, index, true);
}

void Tab::RegisterButtonListenerInOverlay(std::string id, std::function<void(void)> downCallback, std::function<void(void)> upCallback, std::function<void(void)> selectedCallback)
{
	// Log bug when id already existing
	auto iter = _overlayButtonDownCallbacks.find(id);
	if (iter != _overlayButtonDownCallbacks.end())
	{
		LogBug("Tab: Element id exists already in overlay button down callbacks: ", id);
	}
	iter = _overlayButtonUpCallbacks.find(id);
	if (iter != _overlayButtonUpCallbacks.end())
	{
		LogBug("Tab: Element id exists already in overlay button up callbacks: ", id);
	}
	iter = _overlayButtonSelectedCallbacks.find(id);
	if (iter != _overlayButtonSelectedCallbacks.end())
	{
		LogBug("Tab: Element id exists already in overlay button selected callbacks: ", id);
	}

	_overlayButtonDownCallbacks.emplace(id, downCallback);
	_overlayButtonUpCallbacks.emplace(id, upCallback);
	_overlayButtonSelectedCallbacks.emplace(id, selectedCallback);

	// Tell eyeGUI about it
	eyegui::registerButtonListener(_pOverlayLayout, id, _spTabOverlayButtonListener);
}

void Tab::UnregisterButtonListenerInOverlay(std::string id)
{
	_overlayButtonDownCallbacks.erase(id);
	_overlayButtonUpCallbacks.erase(id);
	_overlayButtonSelectedCallbacks.erase(id);
}

void Tab::ClassifyButton(std::string id, bool accept)
{
	eyegui::classifyButton(_pOverlayLayout, id, accept);
}

void Tab::RegisterKeyboardListenerInOverlay(std::string id, std::function<void(std::string)> selectCallback, std::function<void(std::u16string)> pressCallback)
{
	// Log bug when id already existing
	auto iterSelect = _overlayKeyboardSelectCallbacks.find(id);
	if (iterSelect != _overlayKeyboardSelectCallbacks.end())
	{
		LogBug("Tab: Element id exists already in overlay keyboard key select callbacks: ", id);
	}
	auto iterPress = _overlayKeyboardPressCallbacks.find(id);
	if (iterPress != _overlayKeyboardPressCallbacks.end())
	{
		LogBug("Tab: Element id exists already in overlay keyboard key press callbacks: ", id);
	}
	_overlayKeyboardSelectCallbacks.emplace(id, selectCallback);
	_overlayKeyboardPressCallbacks.emplace(id, pressCallback);

	// Tell eyeGUI about it
	eyegui::registerKeyboardListener(_pOverlayLayout, id, _spTabOverlayKeyboardListener);
}

void Tab::UnregisterKeyboardListenerInOverlay(std::string id)
{
	_overlayKeyboardSelectCallbacks.erase(id);
	_overlayKeyboardPressCallbacks.erase(id);
}

void Tab::SetCaseOfKeyboardLetters(std::string id, bool upper)
{
	eyegui::setCaseOfKeyboard(_pOverlayLayout, id, upper ? eyegui::KeyboardCase::UPPER : eyegui::KeyboardCase::LOWER);
}

void Tab::SetKeymapOfKeyboard(std::string id, unsigned int keymap)
{
	eyegui::setKeymapOfKeyboard(_pOverlayLayout, id, keymap);
}

void Tab::ClassifyKey(std::string id, bool accept)
{
	eyegui::classifyKey(_pOverlayLayout, id, accept);
}

void Tab::RegisterWordSuggestListenerInOverlay(std::string id, std::function<void(std::u16string)> callback)
{
	// Log bug when id already existing
	auto iter = _overlayWordSuggestCallbacks.find(id);
	if (iter != _overlayWordSuggestCallbacks.end())
	{
		LogBug("Tab: Element id exists already in overlay word suggest callbacks: ", id);
	}

	_overlayWordSuggestCallbacks.emplace(id, callback);

	// Tell eyeGUI about it
	eyegui::registerWordSuggestListener(_pOverlayLayout, id, _spTabOverlayWordSuggestListener);
}

void Tab::UnregisterWordSuggestListenerInOverlay(std::string id)
{
	_overlayWordSuggestCallbacks.erase(id);
}

void Tab::DisplaySuggestionsInWordSuggest(std::string id, std::u16string input)
{
	if (input.empty())
	{
		eyegui::clearSuggestions(_pOverlayLayout, id);
	}
	else
	{
		eyegui::suggestWords(_pOverlayLayout, id, input, _pMaster->GetDictionary());
	}
}

void Tab::GetScrollingOffset(double& rScrollingOffsetX, double& rScrollingOffsetY) const
{
	rScrollingOffsetX = _scrollingOffsetX;
	rScrollingOffsetY = _scrollingOffsetY;
}

void Tab::SetContentOfTextBlock(std::string id, std::u16string content)
{
	eyegui::setContentOfTextBlock(_pOverlayLayout, id, content);
}

void Tab::SetContentOfTextBlock(std::string id, std::string key)
{
	eyegui::setContentOfTextBlock(_pOverlayLayout, id, _pMaster->FetchLocalization(key));
}

void Tab::AddContentAtCursorInTextEdit(std::string id, std::u16string content)
{
	eyegui::addContentAtCursorInTextEdit(_pOverlayLayout, id, content);
}

void Tab::DeleteContentAtCursorInTextEdit(std::string id, int letterCount)
{
	eyegui::deleteContentAtCursorInTextEdit(_pOverlayLayout, id, letterCount);
}

void Tab::DeleteContentInTextEdit(std::string id)
{
	// Replace by empty string
	eyegui::setContentOfTextEdit(_pOverlayLayout, id, u"");
}

std::u16string Tab::GetActiveEntityContentInTextEdit(std::string id) const
{
	return eyegui::getActiveEntityContentInTextEdit(_pOverlayLayout, id);
}

void Tab::SetActiveEntityContentInTextEdit(std::string id, std::u16string content)
{
	eyegui::setActiveEntityContentInTextEdit(_pOverlayLayout, id, content);
}

std::u16string Tab::GetContentOfTextEdit(std::string id)
{
	return eyegui::getContentOfTextEdit(_pOverlayLayout, id);
}

void Tab::MoveCursorOverLettersInTextEdit(std::string id, int letterCount)
{
	eyegui::moveCursorOverLettersInTextEdit(_pOverlayLayout, id, letterCount);
}

void Tab::MoveCursorOverWordsInTextEdit(std::string id, int wordCount)
{
	eyegui::moveCursorOverWordsInTextEdit(_pOverlayLayout, id, wordCount);
}

void Tab::SetElementActivity(std::string id, bool active, bool fade)
{
	eyegui::setElementActivity(_pOverlayLayout, id, active, fade);
}

void Tab::ButtonUp(std::string id)
{
	eyegui::buttonUp(_pOverlayLayout, id, false);
}

void Tab::SetKeyboardLayout(eyegui::KeyboardLayout keyboardLayout)
{
	_pMaster->SetKeyboardLayout(keyboardLayout);
}

void Tab::SetSpaceOfFlow(std::string id, float space)
{
	eyegui::setSpaceOfFlow(_pOverlayLayout, id, space);
}

void Tab::AddBrickToStack(
	std::string id,
	std::string brickFilepath,
	std::map<std::string, std::string> idMapper)
{
	eyegui::addBrickToStack(_pOverlayLayout, id, brickFilepath, idMapper);
}

int Tab::GetWebViewX() const
{
	return _upWebView->GetX();
}

int Tab::GetWebViewY() const
{
	return _upWebView->GetY();
}

int Tab::GetWebViewWidth() const
{
	return _upWebView->GetWidth();
}

int Tab::GetWebViewHeight() const
{
	return _upWebView->GetHeight();
}

int Tab::GetWebViewResolutionX() const
{
	return _upWebView->GetResolutionX();
}

int Tab::GetWebViewResolutionY() const
{
	return _upWebView->GetResolutionY();
}

int Tab::GetWindowWidth() const
{
	return _pMaster->GetWindowWidth();
}

int Tab::GetWindowHeight() const
{
	return _pMaster->GetWindowHeight();
}

void Tab::ApplyGazeDriftCorrection(float& rPixelX, float& rPixelY) const
{
	_pMaster->ApplyGazeDriftCorrection(rPixelX, rPixelY);
}