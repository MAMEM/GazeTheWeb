//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DOMNodeInteraction.h"
#include "src/CEF/Mediator.h"

void DOMTextInputInteraction::InputText(std::string text, bool submit)
{
	// Focus input node
	_pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "focusNode");

	// Clear text content and afterwards enter input
	_pTab->EmulateSelectAll();
	_pTab->EmulateKeyboardStrokes(text);

	// Emulate Enter key, when submitting
	if (submit)
	{
		_pTab->EmulateEnterKey();
	}

	_pTab->ExecuteCorrespondingJavascriptFunction(getBasePtr(), "setText", text);
}
