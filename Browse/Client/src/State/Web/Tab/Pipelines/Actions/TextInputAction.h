//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action to input text into a text input field on a webpage.
// - Input: std::u16string text
// - Input: int submit (0 if not, else submit)
// - Input: float duration (how long text input took)
// - Output: none

#ifndef TEXTINPUTACTION_H_
#define TEXTINPUTACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/CEF/Data/DOMNodeInteraction.h"
#include "src/CEF/Data/DOMNode.h"

class TextInputAction : public Action
{
public:

    // Constructor
    TextInputAction(TabInteractionInterface* pTab, std::shared_ptr<const DOMTextInput> spNode, std::shared_ptr<DOMTextInputInteraction> spInteractionNode, bool isPasswordField);

    // Destructor
    virtual ~TextInputAction();

    // Update retuns whether finished with execution
    virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

    // Abort
    virtual void Abort();

private:

	// Members
	std::shared_ptr<const DOMTextInput> _spNode;
	std::shared_ptr<DOMTextInputInteraction> _spInteractionNode;
	bool _isPasswordField;

};

#endif // TEXTINPUTACTION_H_
