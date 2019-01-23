//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "SelectFieldAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "submodules/eyeGUI/include/eyeGUI.h"

SelectFieldAction::SelectFieldAction(TabInteractionInterface *pTab, std::shared_ptr<DOMSelectFieldInteraction> spInteractionNode) :
	Action(pTab),
	_spInteractionNode(spInteractionNode)
{
	// Add in- and output data slots
	AddIntInputSlot("option");
}

SelectFieldAction::~SelectFieldAction()
{
    // Nothing to do
}

bool SelectFieldAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
    // Set option
	int option = 0;
	GetInputValue("option", option);
	_spInteractionNode->SetSelectionIndex(option);

	// Action is done
	return true;
}

void SelectFieldAction::Draw() const
{
    // Nothing to do
}

void SelectFieldAction::Activate()
{
    // Nothing to do
}

void SelectFieldAction::Deactivate()
{
    // Nothing to do
}

void SelectFieldAction::Abort()
{
    // Nothing to do
}
