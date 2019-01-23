//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action to present user options of a select field on a webpage
// - Input: none
// - Output: int option

#ifndef SELECTFIELDOPTIONSACTION_H_
#define SELECTFIELDOPTIONSACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/CEF/Data/DOMNode.h"

class SelectFieldOptionsAction : public Action
{
public:

    // Constructor
	SelectFieldOptionsAction(TabInteractionInterface* pTab, std::shared_ptr<const DOMSelectField> spNode);

    // Destructor
    virtual ~SelectFieldOptionsAction();

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

	// Store count of options to delete ids later on
	int _optionCount = 0;

	// Index of floating frame in Tab's overlay
	int _overlayFrameIndex = -1;

	// Bool whether finished
	bool _finished = false;
};

#endif // SELECTFIELDOPTIONSACTION_H_
