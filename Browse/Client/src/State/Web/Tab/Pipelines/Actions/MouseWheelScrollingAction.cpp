//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "MouseWheelScrollingAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"

MouseWheelScrollingAction::MouseWheelScrollingAction(TabInteractionInterface* pTab) : Action(pTab)
{
    // Add in- and output data slots
    AddVec2InputSlot("scrolling");
}

bool MouseWheelScrollingAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput)
{
    // Get coordinate from input slot
    glm::vec2 scrolling;
    GetInputValue("scrolling", scrolling);
    _pTab->EmulateMouseWheelScrolling(scrolling.x, scrolling.y);
    return true;
}

void MouseWheelScrollingAction::Draw() const
{
    // Nothing to do
}

void MouseWheelScrollingAction::Activate()
{
    // Nothing to do
}

void MouseWheelScrollingAction::Deactivate()
{
    // Nothing to do
}

void MouseWheelScrollingAction::Abort()
{
    // Nothing to do
}
