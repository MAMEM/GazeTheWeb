//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action for replying to JavaScript dialogs.
// - Input: int clickedOk (0 if user selected cancel, else ok)
// - Input: std::u16string userInput
// - Output: none

#ifndef REPLYJSDDIALOGACTION_H_
#define REPLYJSDDIALOGACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"

class ReplyJSDialogAction : public Action
{
public:

    // Constructor
	ReplyJSDialogAction(TabInteractionInterface* pTab);

    // Desctructor
    virtual ~ReplyJSDialogAction();

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
	bool _executed = false;
};

#endif // REPLYJSDDIALOGACTION_H_
