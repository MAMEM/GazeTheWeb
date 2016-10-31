//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "PivotMenuAction.h"
#include "src/State/Web/Tab/Pipelines/LeftMouseButtonClickPipeline.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"

PivotMenuAction::PivotMenuAction(TabInteractionInterface *pTab) : Action(pTab)
{
    // Add in- and output data slots
    AddVec2InputSlot("coordinate");

    // Calculate size of overlay (TODO: what to do at resize? everything ok? simpler function call would be cool)
    int webViewX, webViewY, webViewWidth, webViewHeight;
    _pTab->CalculateWebViewPositionAndSize(webViewX, webViewY, webViewWidth, webViewHeight);
    int windowWidth, windowHeight;
    _pTab->GetWindowSize(windowWidth, windowHeight);
    float sizeX, sizeY;

    // ### Menu overlay ###

    // Pixel values
    sizeX = (float)webViewWidth * 0.8f;
    sizeY = (float)webViewHeight * _menuHeight;

    // Relative values
    sizeX /= windowWidth;
    sizeY /= windowHeight;

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

    // Right click button
    _pTab->RegisterButtonListenerInOverlay(
        "pivot_right_click",
        [&]() // down callback
        {
            _done= true;
        },
        [](){}); // up callback

    // Double left click button
    _pTab->RegisterButtonListenerInOverlay(
        "pivot_double_left_click",
        [&]() // down callback
        {
            _done= true;
        },
        [](){}); // up callback

    // Selection button
    _pTab->RegisterButtonListenerInOverlay(
        "pivot_selection",
        [&]() // down callback
        {
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

bool PivotMenuAction::Update(float tpf, TabInput tabInput)
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

    // Fetch web view values
    int webViewX, webViewY, webViewWidth, webViewHeight;
    _pTab->CalculateWebViewPositionAndSize(webViewX, webViewY, webViewWidth, webViewHeight);
    int windowWidth, windowHeight;
    _pTab->GetWindowSize(windowWidth, windowHeight);

    // Position of menu
    float verticalPosition = (coordinate.y > (webViewHeight / 2)) ? 0.1f : (0.5f + _menuHeight);
    float x = (float)webViewX + ((float)webViewWidth * 0.1f);
    float y = (float)webViewY + ((float)webViewHeight * verticalPosition);
    _pTab->SetPositionOfFloatingFrameInOverlay(_menuFrameIndex, x / windowWidth, y / windowHeight);

    // Position of pivot
    glm::vec2 pivotPosition = coordinate;
    pivotPosition.x += webViewX;
    pivotPosition.y += webViewY;
    pivotPosition.x /= windowWidth;
    pivotPosition.y /= windowHeight;
    pivotPosition.x -= _pivotSize / 2.f;
    pivotPosition.y -= _pivotSize / 2.f;
    _pTab->SetPositionOfFloatingFrameInOverlay(_pivotFrameIndex, pivotPosition.x, pivotPosition.y);

    // Make overlays visible
    _pTab->SetVisibilyOfFloatingFrameInOverlay(_menuFrameIndex, true);
    _pTab->SetVisibilyOfFloatingFrameInOverlay(_pivotFrameIndex, true);
}

void PivotMenuAction::Deactivate()
{
    _pTab->SetVisibilyOfFloatingFrameInOverlay(_menuFrameIndex, false);
    _pTab->SetVisibilyOfFloatingFrameInOverlay(_pivotFrameIndex, false);
}

void PivotMenuAction::Abort()
{
    // Nothing to do
}
