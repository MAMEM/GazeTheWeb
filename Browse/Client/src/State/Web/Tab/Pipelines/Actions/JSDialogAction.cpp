//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "JSDialogAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "submodules/eyeGUI/include/eyeGUI.h"

JSDialogAction::JSDialogAction(TabInteractionInterface* pTab, std::string message, bool enableCancel) : Action(pTab)
{
	// Add output slot for clickedOk
	AddIntOutputSlot("clickedOk");

    // Create id, which is unique in overlay
    _overlayOkButtonId = "jsdialog_ok_button";
	_overlayCancelButtonId = "jsdialog_cancel_button";
	_overlayTextBlockId = "jsdialog_text_block";

    // Id mapper for brick to change ids from file to the used ones
    std::map<std::string, std::string> idMapper;
    idMapper.emplace("ok_button", _overlayOkButtonId);
	idMapper.emplace("cancel_button", _overlayCancelButtonId);
	idMapper.emplace("text_block", _overlayTextBlockId);

    // Calculate size of overlay
    float x, y, sizeX, sizeY;
    x = (float)_pTab->GetWebViewX() / (float)_pTab->GetWindowWidth();
    y = (float)_pTab->GetWebViewY() / (float)_pTab->GetWindowHeight();
    sizeX = (float)_pTab->GetWebViewWidth() / (float)_pTab->GetWindowWidth();
    sizeY = (float)_pTab->GetWebViewHeight() / (float)_pTab->GetWindowHeight();

    // Add overlay
    _overlayFrameIndex = _pTab->AddFloatingFrameToOverlay("bricks/actions/JSDialog.beyegui", x, y, sizeX, sizeY, idMapper);

    // Register listeners
    _pTab->RegisterButtonListenerInOverlay(
        _overlayOkButtonId,
        [&]() // down callback
        {
			SetOutputValue("clickedOk", 1);
            this->_done = true;
        },
        [](){}); // up callback
	_pTab->RegisterButtonListenerInOverlay(
		_overlayCancelButtonId,
		[&]() // down callback
		{
			SetOutputValue("clickedOk", 0);
			this->_done = true;
		},
		[]() {}); // up callback

	// Convert message to UTF-16
	std::u16string message16;
	eyegui_helper::convertUTF8ToUTF16(message, message16);

	// Set content of text block
	_pTab->SetContentOfTextBlock(_overlayTextBlockId, message16);

	// Set activity of cancel button
	if (!enableCancel) { _pTab->SetElementActivity(_overlayCancelButtonId, false, false); }
}

JSDialogAction::~JSDialogAction()
{
    // Delete overlay frame
    _pTab->RemoveFloatingFrameFromOverlay(_overlayFrameIndex);

    // Unregister buttons
    _pTab->UnregisterButtonListenerInOverlay(_overlayOkButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayCancelButtonId);
}

bool JSDialogAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
    return _done;
}

void JSDialogAction::Draw() const
{
	// Nothing to do
}

void JSDialogAction::Activate()
{
	_pTab->SetVisibilityOfFloatingFrameInOverlay(_overlayFrameIndex, true);

	// Play sound
	_pTab->PlaySound("sounds/GameAudio/TeleportCasual.ogg");
}

void JSDialogAction::Deactivate()
{
	_pTab->SetVisibilityOfFloatingFrameInOverlay(_overlayFrameIndex, false);
}

void JSDialogAction::Abort()
{
	// Nothing to do
}
