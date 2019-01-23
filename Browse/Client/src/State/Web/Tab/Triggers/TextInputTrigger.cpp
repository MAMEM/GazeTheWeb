//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "TextInputTrigger.h"
#include "src/State/Web/Tab/Pipelines/TextInputPipeline.h"
#include "src/Singletons/LabStreamMailer.h"

TextInputTrigger::TextInputTrigger(TabInteractionInterface* pTab, std::vector<Trigger*>& rTriggerCollection, std::shared_ptr<DOMTextInput> spNode) : DOMTrigger<DOMTextInput>(pTab, rTriggerCollection, spNode, "bricks/triggers/TextInput.beyegui", "text_input")
{
	// Nothing to do here
}

TextInputTrigger::~TextInputTrigger()
{
	// Nothing to do here
}

bool TextInputTrigger::Update(float tpf, const std::shared_ptr<const TabInput> spInput)
//bool TextInputTrigger::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
	// Call super method
	bool triggered = DOMTrigger::Update(tpf, spInput);

	// When triggered, push back pipeline to input text
	if (triggered)
	{
		//if (spVoiceInputObject->GetVoiceMode == VoiceMode::FREE)
		//	spVoiceInputObject->SetVoiceMode(VoiceMode::COMMAND);

		LabStreamMailer::instance().Send("Text input started");

		_pTab->PushBackPipeline(
			std::move(
				std::unique_ptr<TextInputPipeline>(
					new TextInputPipeline(
						_pTab,
						_spNode,
						_spNode))));
	}

	// Return whether triggered
	return triggered;
}