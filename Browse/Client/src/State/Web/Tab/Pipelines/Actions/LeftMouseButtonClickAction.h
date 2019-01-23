//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action to emulate left mouse button click.
// - Input: vec2 coordinate in WebViewPixel coordinates
// - Input: int visualize (0 if not, else visualize; default here is 1)
// - Output: none

#ifndef LEFTMOUSEBUTTONCLICKACTION_H_
#define LEFTMOUSEBUTTONCLICKACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"

class LeftMouseButtonClickAction : public Action
{
public:

    // Constructor
    LeftMouseButtonClickAction(TabInteractionInterface* pTab);

    // Update retuns whether finished with execution
    virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

    // Abort
    virtual void Abort();
};

#endif // LEFTMOUSEBUTTONCLICKACTION_H_
