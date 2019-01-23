//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action for delaying further pipeline execution.

#ifndef DELAYACTION_H_
#define DELAYACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"

class DelayAction : public Action
{
public:

    // Constructor
	DelayAction(TabInteractionInterface* pTab, float seconds);

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

protected:

	float _time;
};

#endif // DELAYACTION_H_
