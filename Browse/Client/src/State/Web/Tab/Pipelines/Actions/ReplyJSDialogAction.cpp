//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "ReplyJSDialogAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "submodules/eyeGUI/include/eyeGUI.h"

ReplyJSDialogAction::ReplyJSDialogAction(TabInteractionInterface* pTab) : Action(pTab)
{
	// Add in- and output data slots
	AddIntInputSlot("clickedOk");
	AddString16InputSlot("userInput");
}

ReplyJSDialogAction::~ReplyJSDialogAction()
{
	// A little bit hacky but: if deactivated before finished,
	// the page may remain frozen since it expects a JavaScript callback.
	// Therefore execute some fallback here. This is called even if
	// never activated by pipeline. It has to be just created and deleted.
	if (!_executed) { _pTab->ReplyJSDialog(false, ""); }
}

bool ReplyJSDialogAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput)
{
	// Get values out of slots
	int clickedOk;
	std::u16string userInput;
	GetInputValue("clickedOk", clickedOk);
	GetInputValue("userInput", userInput);
	std::string userInput8;
	eyegui_helper::convertUTF16ToUTF8(userInput, userInput8);
	_pTab->ReplyJSDialog(clickedOk != 0, userInput8);
	_executed = true;
    return true;
}

void ReplyJSDialogAction::Draw() const
{
	// Nothing to do
}

void ReplyJSDialogAction::Activate()
{
	// Nothing to do
}

void ReplyJSDialogAction::Deactivate()
{
	// Nothing to do
}

void ReplyJSDialogAction::Abort()
{
	// Nothing to do
}
