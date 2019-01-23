//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "TextSelectionAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/Setup.h"

TextSelectionAction::TextSelectionAction(TabInteractionInterface *pTab) : ZoomCoordinateAction(pTab, false)
{
	// Add in- and output data slots
	AddVec2InputSlot("coordinate");
}

bool TextSelectionAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
	// Call standard zoom coordinate update function
	bool done = ZoomCoordinateAction::Update(tpf, spInput, spVoiceInput, spVoiceInputObject);

	// Calculate current coordinate in CEFPixel space
	glm::vec2 cefPixels(_pTab->GetWebViewResolutionX(), _pTab->GetWebViewResolutionY());
	glm::vec2 zoomCoordinate = _relativeZoomCoordinate * cefPixels; // _coordinate is given by superclass

	// When finished, set end position of text selection
	if (done)
	{
		// End selection procedure
		glm::vec2 coordinate;
		this->GetOutputValue("coordinate", coordinate);
		_pTab->EmulateLeftMouseButtonUp(coordinate.x, coordinate.y, false, setup::TEXT_SELECTION_MARGIN);

		LogInfo("Up: ", coordinate.x, ", ", coordinate.y);

		// Copy selected string to clipboard. Maybe create extra action for this later
		_pTab->PutTextSelectionToClipboardAsync();
	}
	else
	{
		// Keep emulating mouse cursor
		_pTab->EmulateMouseCursor(zoomCoordinate.x, zoomCoordinate.y, true, setup::TEXT_SELECTION_MARGIN);
	}

    return done;
}

void TextSelectionAction::Activate()
{
	// Call super method
	ZoomCoordinateAction::Activate();

	// Set starting point of selection
	glm::vec2 startCoordinate;
	GetInputValue("coordinate", startCoordinate);
	_pTab->EmulateLeftMouseButtonDown(startCoordinate.x, startCoordinate.y, false, -setup::TEXT_SELECTION_MARGIN);

	LogInfo("Down: ", startCoordinate.x, ", ", startCoordinate.y);
}

void TextSelectionAction::Deactivate()
{
    // Reset mouse state
    _pTab->EmulateLeftMouseButtonUp(0, 0);
}
