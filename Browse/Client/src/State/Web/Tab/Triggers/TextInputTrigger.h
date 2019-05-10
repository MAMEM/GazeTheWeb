//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Implementation of trigger generated from text input fields.

#ifndef TEXTINPUTTRIGGER_H_
#define TEXTINPUTTRIGGER_H_

#include "src/State/Web/Tab/Triggers/DOMTrigger.h"

class TextInputTrigger : public DOMTrigger<DOMTextInput>
{
public:

	// Constructor
	TextInputTrigger(TabInteractionInterface* pTab, std::vector<Trigger*>& rTriggerCollection, std::shared_ptr<DOMTextInput> spNode);

	// Destructor
	virtual ~TextInputTrigger();

	// Update
	//virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject);
	bool TextInputTrigger::Update(float tpf, const std::shared_ptr<const TabInput> spInput);

};

#endif // TEXTINPUTTRIGGER_H_