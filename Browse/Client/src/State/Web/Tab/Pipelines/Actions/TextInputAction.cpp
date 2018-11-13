//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "TextInputAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "submodules/eyeGUI/include/eyeGUI.h"
#include "submodules/eyeGUI/externals/levenshtein-sse/levenshtein-sse.hpp" // TODO: VERY BAD PRACTICE TO INCLUDE FROM EYEGUI

TextInputAction::TextInputAction(TabInteractionInterface* pTab, std::shared_ptr<const DOMTextInput> spNode, std::shared_ptr<DOMTextInputInteraction> spInteractionNode, bool isPasswordField) :
	Action(pTab),
	_spNode(spNode),
	_spInteractionNode(spInteractionNode),
	_isPasswordField(isPasswordField)
{
    // Add in- and output data slots
    AddString16InputSlot("text");
    AddIntInputSlot("submit");
	AddFloatInputSlot("duration");
}

TextInputAction::~TextInputAction()
{
    // Nothing to do
}

bool TextInputAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput)
{
	// Fetch input values
	std::u16string text;
	int submit = 0;
	GetInputValue("text", text);
	GetInputValue("submit", submit);

	// Convert u16string to string
	std::string text8;
	eyegui_helper::convertUTF16ToUTF8(text, text8);

	// Calculate distance between original text and inputted one
	std::string originalText = _spNode->GetText();
	int distance = (int)levenshteinSSE::levenshtein(
		originalText.begin(), originalText.end(),
		text8.begin(), text8.end());	

	// Tell tab about for social record
	if (!_isPasswordField) // only if no password field
	{
		float x = -1;
		float y = -1;
		if (!_spNode->GetRects().empty())
		{
			auto coord = _spNode->GetRects()[0].Center();
			x = coord.x;
			y = coord.y;
		}

		float duration = -1.f;
		GetInputValue("duration", duration);
		
		_pTab->NotifyTextInput(_spNode->GetHTMLClass(), _spNode->GetHTMLId(), text.length(), distance, x, y, duration);
	}

	// Input text
	_spInteractionNode->InputText(text8, submit > 0); // TODO: Call LSL Logging?

    // Action is done
    return true;
}

void TextInputAction::Draw() const
{
    // Nothing to do
}

void TextInputAction::Activate()
{
    // Nothing to do
}

void TextInputAction::Deactivate()
{
    // Nothing to do
}

void TextInputAction::Abort()
{
    // Nothing to do
}
