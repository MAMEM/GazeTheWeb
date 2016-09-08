//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action to navigate links by coordinate. DOM tree is used to fix imprecise
// coordinates.

// Notes
// - Input: vec2 coordinate in pixels
// - Input: int visualize (0 if not, else visualize; default here is 1)
// - Output: none

#ifndef LINKNAVIGATIONACTION_H_
#define LINKNAVIGATIONACTION_H_

#include "Action.h"

class LinkNavigationAction : public Action
{
public:

    // Constructor
    LinkNavigationAction(TabInteractionInterface* pTab);

    // Update retuns whether finished with execution
    virtual bool Update(float tpf, TabInput tabInput);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

    // Abort
    virtual void Abort();
};

#endif // LINKNAVIGATIONACTION_H_
