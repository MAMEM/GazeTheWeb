//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "VideoModeTrigger.h"
#include "src/State/Web/Tab/Pipelines/TextInputPipeline.h"
#include "src/Singletons/LabStreamMailer.h"

VideoModeTrigger::VideoModeTrigger(TabInteractionInterface* pTab, std::vector<Trigger*>& rTriggerCollection, std::shared_ptr<DOMVideo> spNode) : DOMTrigger<DOMVideo>(pTab, rTriggerCollection, spNode, "bricks/triggers/VideoMode.beyegui", "video")
{
	// Nothing to do here
}

VideoModeTrigger::~VideoModeTrigger()
{
	// Nothing to do here
}

bool VideoModeTrigger::Update(float tpf, const std::shared_ptr<const TabInput> spInput)
{
	// Call super method
	bool triggered = DOMTrigger::Update(tpf, spInput);

	// When triggered, do something
	if (triggered)
	{
		_spNode->SetPlaying(true); // TODO testing
	}

	// Return whether triggered
	return triggered;
}