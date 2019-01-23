//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "PivotMenuAction.h"
#include "src/State/Web/Tab/Pipelines/LeftMouseButtonClickPipeline.h"
#include "src/State/Web/Tab/Pipelines/LeftMouseButtonDoubleClickPipeline.h"
#include "src/State/Web/Tab/Pipelines/TextSelectionPipeline.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"

PivotMenuAction::PivotMenuAction(TabInteractionInterface *pTab) : Action(pTab)
{
    // Add in- and output data slots
    AddVec2InputSlot("coordinate");

    // ### Menu overlay ###
	float sizeX, sizeY;

    // Pixel values
    sizeX = (float)_pTab->GetWebViewWidth() * 0.8f;
    sizeY = (float)_pTab->GetWebViewHeight() * _menuHeight;

    // Relative values
    sizeX /= _pTab->GetWindowWidth();
	sizeY /= _pTab->GetWindowHeight();

    // Add overlay (TODO: no id mapper since no idea when there could occur a problem about it)
    // Position is set at actviation when coordinate is known
    _menuFrameIndex = _pTab->AddFloatingFrameToOverlay(
        "bricks/actions/PivotMenu.beyegui",
        0,
        0,
        sizeX,
        sizeY,
        std::map<std::string, std::string>());

    // Left click button
    _pTab->RegisterButtonListenerInOverlay(
        "pivot_left_click",
        [&]() // down callback
        {
			// If coordinate set, do left mouse button click
			glm::vec2 coordinate;
			if (this->GetInputValue("coordinate", coordinate))
			{
				_pTab->PushBackPipeline(std::unique_ptr<LeftMouseButtonClickPipeline>(new LeftMouseButtonClickPipeline(_pTab, coordinate)));
			}
            _done= true;
        },
        [](){}); // up callback

	// Double left click button
	_pTab->RegisterButtonListenerInOverlay(
		"pivot_double_left_click",
		[&]() // down callback
		{
			// If coordinate set, do left mouse button click
			glm::vec2 coordinate;
			if (this->GetInputValue("coordinate", coordinate))
			{
				_pTab->PushBackPipeline(std::unique_ptr<LeftMouseButtonDoubleClickPipeline>(new LeftMouseButtonDoubleClickPipeline(_pTab, coordinate)));
			}
			_done = true;
		},
		[]() {}); // up callback


    // Right click button
    _pTab->RegisterButtonListenerInOverlay(
        "pivot_right_click",
        [&]() // down callback
        {
            // TODO
            _done= true;
        },
        [](){}); // up callback

    // ### Pivot overlay ###

    // Position is set at activation when coordinate is known
    _pivotFrameIndex = _pTab->AddFloatingFrameToOverlay(
        "bricks/actions/Pivot.beyegui",
        0,
        0,
        _pivotSize, // size in x direction
        _pivotSize, // size in y direction
        std::map<std::string, std::string>()); // no id used here
}

PivotMenuAction::~PivotMenuAction()
{
    // Unregister buttons
    _pTab->UnregisterButtonListenerInOverlay("pivot_left_click");
    _pTab->UnregisterButtonListenerInOverlay("pivot_right_click");
    _pTab->UnregisterButtonListenerInOverlay("pivot_double_left_click");
    _pTab->UnregisterButtonListenerInOverlay("pivot_selection");

    // Delete overlay frames
    _pTab->RemoveFloatingFrameFromOverlay(_menuFrameIndex);
    _pTab->RemoveFloatingFrameFromOverlay(_pivotFrameIndex);
}

bool PivotMenuAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
    return _done;
}

void PivotMenuAction::Draw() const
{
    // Nothing to do
}

void PivotMenuAction::Activate()
{
    // Use coordinate for positioning floating elements
    glm::vec2 coordinate;
    GetInputValue("coordinate", coordinate);

    // Position of menu
    float verticalPosition = (coordinate.y > (_pTab->GetWebViewHeight() / 2)) ? 0.1f : (0.5f + _menuHeight);
    float x = (float)_pTab->GetWebViewX() + ((float)_pTab->GetWebViewWidth() * 0.1f);
    float y = (float)_pTab->GetWebViewY() + ((float)_pTab->GetWebViewHeight() * verticalPosition);
    _pTab->SetPositionOfFloatingFrameInOverlay(_menuFrameIndex, x / _pTab->GetWindowWidth(), y / _pTab->GetWindowHeight());

    // Position of pivot
    glm::vec2 pivotPosition = coordinate;
    pivotPosition.x += _pTab->GetWebViewX();
    pivotPosition.y += _pTab->GetWebViewY();
    pivotPosition.x /= _pTab->GetWindowWidth();
    pivotPosition.y /= _pTab->GetWindowHeight();
    pivotPosition.x -= _pivotSize / 2.f;
    pivotPosition.y -= _pivotSize / 2.f;
    _pTab->SetPositionOfFloatingFrameInOverlay(_pivotFrameIndex, pivotPosition.x, pivotPosition.y);

    // Make overlays visible
    _pTab->SetVisibilityOfFloatingFrameInOverlay(_menuFrameIndex, true);
    _pTab->SetVisibilityOfFloatingFrameInOverlay(_pivotFrameIndex, true);
}

void PivotMenuAction::Deactivate()
{
    _pTab->SetVisibilityOfFloatingFrameInOverlay(_menuFrameIndex, false);
    _pTab->SetVisibilityOfFloatingFrameInOverlay(_pivotFrameIndex, false);
}

void PivotMenuAction::Abort()
{
    // Nothing to do
}
