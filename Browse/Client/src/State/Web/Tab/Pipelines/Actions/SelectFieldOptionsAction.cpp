//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "SelectFieldOptionsAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/Global.h"
#include "submodules/eyeGUI/include/eyeGUI.h"

SelectFieldOptionsAction::SelectFieldOptionsAction(TabInteractionInterface *pTab, std::shared_ptr<const DOMSelectField> spNode) : Action(pTab)
{
	// Add in- and output data slots
	AddIntOutputSlot("option");

	// Extract options of select field
	const auto options = spNode->GetOptions();
	_optionCount = options.size();

	// TODO: Hacky way to limit option count. Otherwise rendering / selection gets too hard
	_optionCount = glm::min(_optionCount, 100);

	// Calculate size of overlay (TODO: maybe move this into tab so it is easy to make WebView filling overlay)
	float x, y, sizeX, sizeY;
	x = (float)_pTab->GetWebViewX() / (float)_pTab->GetWindowWidth();
	y = (float)_pTab->GetWebViewY() / (float)_pTab->GetWindowHeight();
	sizeX = (float)_pTab->GetWebViewWidth() / (float)_pTab->GetWindowWidth();
	sizeY = (float)_pTab->GetWebViewHeight() / (float)_pTab->GetWindowHeight();

	// Create flow as overlay in layout
	_overlayFrameIndex = _pTab->AddFloatingFrameToOverlay("bricks/actions/SelectOptionsFlow.beyegui", x, y, sizeX, sizeY);

	// Prepare adding of select options to layout
	std::map<std::string, std::string> idMapper;

	// Set space of flow TODO: breaks when action is multiple times created. move to activate or give extra qualifier for ids
	float space = ((float)(_optionCount + 1) / (float)SELECT_FIELD_OPTIONS_ON_SCREEN); // size + 1 for title
	space = space < 1.f ? 1.f : space;
	_pTab->SetSpaceOfFlow("select_options_flow", space);

	// Go over available options
	for (int i = 0; i < _optionCount; ++i)
	{
		// Option id
		std::string optionId = "option_" + std::to_string(i);

		// Select id
		std::string selectId = "select_" + std::to_string(i);

		// Id mapper
		idMapper.clear();
		idMapper.emplace("option", optionId);
		idMapper.emplace("select", selectId);

		// Add brick to stack
		_pTab->AddBrickToStack(
			"select_options_stack",
			"bricks/actions/SelectOption.beyegui",
			idMapper);

		// Set content of option
		std::u16string option16;
		eyegui_helper::convertUTF8ToUTF16(options.at(i), option16);
		_pTab->SetContentOfTextBlock(optionId, option16);

		// Register button listener for select buttons
		_pTab->RegisterButtonListenerInOverlay(
			selectId,
			[&, i]() // down callback. Providing i as copy, not reference!
		{
			SetOutputValue("option", i);
			_finished = true;
		},
		[]() {}); // up callback
	}
}

SelectFieldOptionsAction::~SelectFieldOptionsAction()
{
	// Delete overlay frame
	_pTab->RemoveFloatingFrameFromOverlay(_overlayFrameIndex);

	// Unregister listener of buttons in overlay
	for (int i = 0; i < _optionCount; ++i)
	{
		_pTab->UnregisterButtonListenerInOverlay("select_" + std::to_string(i));
	}
}

bool SelectFieldOptionsAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput)
{
    return _finished;
}

void SelectFieldOptionsAction::Draw() const
{
    // Nothing to do
}

void SelectFieldOptionsAction::Activate()
{
	// Set visibility of floating frame
	_pTab->SetVisibilityOfFloatingFrameInOverlay(_overlayFrameIndex, true);
}

void SelectFieldOptionsAction::Deactivate()
{
	// Set visibility of floating frame
	_pTab->SetVisibilityOfFloatingFrameInOverlay(_overlayFrameIndex, false);
}

void SelectFieldOptionsAction::Abort()
{
    // Nothing to do
}
