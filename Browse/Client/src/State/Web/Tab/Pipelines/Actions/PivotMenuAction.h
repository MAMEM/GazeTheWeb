//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action for displaying pivot menu.
// - Input: vec2 WebViewPixel coordinate
// - Output: none

#ifndef PIVOTMENUACTION_H_
#define PIVOTMENUACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/Utils/LerpValue.h"

class PivotMenuAction : public Action
{
public:

    // Constructor
    PivotMenuAction(TabInteractionInterface* pTab);

    // Destructor
    ~PivotMenuAction();

    // Update retuns whether finished with execution
	virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

    // Abort
    virtual void Abort();

protected:

    // Index of floating frame for menu in Tab's overlay
    int _menuFrameIndex = -1;

    // Index of floating frame for pivot visualization in Tab's overlay
    int _pivotFrameIndex = -1;

private:

    // Size of pivot in overlay
    const float _pivotSize = 0.2f;

    // Height of menu in overlay
    const float _menuHeight = 0.2f;

    // Check whether done with menu
    bool _done = false;

};

#endif // PIVOTMENUACTION_H_
