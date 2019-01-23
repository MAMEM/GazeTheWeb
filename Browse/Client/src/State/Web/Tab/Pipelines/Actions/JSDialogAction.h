//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action for displaying JavaScript dialogs.
// - Input: none
// - Output: int clickedOk (0 if user selected cancel, else ok)

#ifndef JSDDIALOGACTION_H_
#define JSDDIALOGACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"

class JSDialogAction : public Action
{
public:

    // Constructor, takes message and whether the user can cancel the dialog (TODO: move parameters to input values)
	JSDialogAction(TabInteractionInterface* pTab, std::string message, bool enableCancel);

    // Desctructor
    virtual ~JSDialogAction();

    // Update retuns whether finished with execution
    virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

    // Abort
    virtual void Abort();

protected:

    // Index of floating frame showing dialog
    int _overlayFrameIndex = -1;

    // Id of ok button in overlay
    std::string _overlayOkButtonId;

	// Id of cancel button in overlay
	std::string _overlayCancelButtonId;

	// Id of text block in overlay
	std::string _overlayTextBlockId;

	// Bool which indicates whether done or not
	bool _done = false;
};

#endif // JSDDIALOGACTION_H_
