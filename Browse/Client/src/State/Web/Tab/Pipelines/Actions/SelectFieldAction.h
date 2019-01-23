//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action to choose option of select field on a webpage.
// - Input: int option
// - Output: none

#ifndef SELECTFIELDACTION_H_
#define SELECTFIELDACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/CEF/Data/DOMNodeInteraction.h"

class SelectFieldAction : public Action
{
public:

    // Constructor
	SelectFieldAction(TabInteractionInterface* pTab, std::shared_ptr<DOMSelectFieldInteraction> spInteractionNode);

    // Destructor
    virtual ~SelectFieldAction();

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

private:

	// Members
	std::shared_ptr<DOMSelectFieldInteraction> _spInteractionNode;
};

#endif // SELECTFIELDACTION_H_
