//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "SelectFieldTrigger.h"
#include "src/Singletons/LabStreamMailer.h"

SelectFieldTrigger::SelectFieldTrigger(TabInteractionInterface* pTab, std::vector<Trigger*>& rTriggerCollection, std::shared_ptr<DOMSelectField> spNode) : DOMTrigger<DOMSelectField>(pTab, rTriggerCollection, spNode, "bricks/triggers/SelectField.beyegui")
{
	// Nothing to do here
}

SelectFieldTrigger::~SelectFieldTrigger()
{
	// Nothing to do here
}

bool SelectFieldTrigger::Update(float tpf, TabInput& rTabInput)
{
	// Call super method
	bool triggered = DOMTrigger::Update(tpf, rTabInput);

	// When triggered, push back pipeline to input text
	if (triggered)
	{
		LabStreamMailer::instance().Send("Select field hit");

		// TODO: start correct pipeline
	}

	// Return whether triggered
	return triggered;
}