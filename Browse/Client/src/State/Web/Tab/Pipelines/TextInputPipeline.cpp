//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "TextInputPipeline.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/State/Web/Tab/Interface/TabActionInterface.h"
#include "src/State/Web/Tab/Pipelines/Actions/KeyboardAction.h"
#include "src/State/Web/Tab/Pipelines/Actions/TextInputAction.h"
#include "submodules/eyeGUI/include/eyeGUI.h"

TextInputPipeline::TextInputPipeline(
	TabInteractionInterface* pTab,
	std::shared_ptr<const DOMTextInput> spNode,
	std::shared_ptr<DOMTextInputInteraction> spInteractionNode) : Pipeline(pTab)
{
	// Get current text from node
	std::string text = spNode->GetText();
	std::u16string text16;
	eyegui_helper::convertUTF8ToUTF16(text, text16);

    // Then, do input via keyboard
	std::shared_ptr<KeyboardAction> spKeyboardAction = std::make_shared<KeyboardAction>(_pTab);
	spKeyboardAction->SetInputValue<std::u16string>("text", text16);
	_actions.push_back(spKeyboardAction);

    // At last, fill input into text field
	std::shared_ptr<TextInputAction> spTextInputAction = std::make_shared<TextInputAction>(_pTab, spNode, spInteractionNode, spNode->IsPasswordField());
	_actions.push_back(spTextInputAction);

    // Connect those actions
    std::unique_ptr<ActionConnector> upConnector =
        std::unique_ptr<ActionConnector>(new ActionConnector(spKeyboardAction, spTextInputAction));
    upConnector->ConnectString16("text", "text");
    upConnector->ConnectInt("submit", "submit");
	upConnector->ConnectFloat("duration", "duration");
    _connectors.push_back(std::move(upConnector));
}
