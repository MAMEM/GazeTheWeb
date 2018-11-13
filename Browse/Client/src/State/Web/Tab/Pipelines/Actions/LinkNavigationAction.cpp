//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "LinkNavigationAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/State/Web/Tab/Interface/TabActionInterface.h"
#include "src/CEF/Data/DOMNode.h"
#include "src/Setup.h"

LinkNavigationAction::LinkNavigationAction(TabInteractionInterface* pTab) : Action(pTab)
{
    // Add in- and output data slots
    AddVec2InputSlot("coordinate");
	AddIntInputSlot("visualize", 1);
}

bool LinkNavigationAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput)
{
    // Get coordinate from input slot (WebViewPixel space)
    glm::vec2 coordinate;
    GetInputValue("coordinate", coordinate);

	// Get whether should be visualized
    int visualize = 0;
	GetInputValue("visualize", visualize);

    // Decide what to click
	double CEFPixelX = coordinate.x;
	double CEFPixelY = coordinate.y;

    double scrollingX, scrollingY;
    _pTab->GetScrollingOffset(scrollingX, scrollingY);
	
	// Call function to find nearest neighbor
	
	float distance = 0.f;
    glm::vec2 pagePixelCoordinate = glm::vec2(CEFPixelX + scrollingX, CEFPixelY + scrollingY);
    std::weak_ptr<const DOMNode> wpNearestLink = _pTab->GetNearestLink(pagePixelCoordinate, distance);

    // Determine where to click instead, if not too far away or coordinate already valid
    if(distance < setup::LINK_CORRECTION_MAX_PIXEL_DISTANCE && distance > 0)
    {
        // Try to get value from weak pointer
        if (auto sp = wpNearestLink.lock())
        {
            // Just get coordinate of first rect
            if(!sp->GetRects().empty())
            {
				// Get coordinate in CEFPixel space
                glm::vec2 linkCoordinate = sp->GetRects().front().Center();
				CEFPixelX = linkCoordinate.x - scrollingX;
				CEFPixelY = linkCoordinate.y - scrollingY;
            }
        }
    }

    // Emulate left mouse button click (TODO: assumption that this was actively triggered by a user)
    _pTab->EmulateLeftMouseButtonClick(CEFPixelX, CEFPixelY, visualize > 0, false, true); // coordinate already in CEFPixel space

	// Play click sound
	_pTab->PlaySound("sounds/GameAudio/ClickBasic.ogg");

	// Return success
    return true;
}

void LinkNavigationAction::Draw() const
{
    // Nothing to do
}

void LinkNavigationAction::Activate()
{
    // Nothing to do
}

void LinkNavigationAction::Deactivate()
{
    // Nothing to do
}

void LinkNavigationAction::Abort()
{
    // Nothing to do
}
