//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action for testing.

#ifndef TESTACTION_H_
#define TESTACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/Utils/LerpValue.h"

class TestAction : public Action
{
public:

    // Constructor
    TestAction(TabInteractionInterface* pTab);

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

protected:

    LerpValue _dim;
};

#endif // TESTACTION_H_
