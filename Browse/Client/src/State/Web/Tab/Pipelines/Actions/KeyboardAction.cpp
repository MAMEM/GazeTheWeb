//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "KeyboardAction.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "submodules/eyeGUI/include/eyeGUI.h"

#include "src/Singletons/JSMailer.h"

KeyboardAction::KeyboardAction(TabInteractionInterface *pTab) : Action(pTab)
{
    // Add in- and output data slots
	AddString16InputSlot("text");
    AddString16OutputSlot("text");
    AddIntOutputSlot("submit");
	AddFloatOutputSlot("duration", 0.f);

	// TODO: forget about ids and move all of this into activation

    // Create id, which is unique in overlay
    _overlayKeyboardId = "text_input_action_keyboard";
    _overlayCompleteButtonId = "text_input_action_complete_button";
    _overlaySubmitButtonId = "text_input_action_submit_button";
    _overlayDeleteCharacterButtonId = "text_input_action_delete_character_button";
	_overlayPasteButtonId = "text_input_action_paste_button";
    _overlaySpaceButtonId = "text_input_action_space_button";
	_overlayTextEditId = "text_input_action_text_edit";
    _overlayWordSuggestId = "text_input_action_word_suggest";
	_overlayShiftButtonId = "text_input_action_shift_button";
	_overlayNewLineButtonId = "text_input_action_new_line_button";
	_overlayNextWordButtonId = "text_input_action_next_word_button";
	_overlayPreviousWordButtonId = "text_input_action_previous_word_button";
	_overlayNextLetterButtonId = "text_input_action_next_letter_button";
	_overlayPreviousLetterButtonId = "text_input_action_previous_letter_button";
	_overlayDeleteAllButtonId = "text_input_action_delete_all_button";
	_overlayLayoutId = "text_input_action_layout_button";
	_overlayEnglishLayoutId = "text_input_action_us_english_button";
	_overlayGermanLayoutId = "text_input_action_germany_german_button";
	_overlayHebrewLayoutId = "text_input_action_israel_hebrew_button";
	_overlayGreekLayoutId = "text_input_action_greece_greek_button";
	_overlayExtraKeyId = "text_input_action_extra_keys_button";

    // Id mapper for brick to change ids from file to the used ones
    std::map<std::string, std::string> idMapper;
    idMapper.emplace("keyboard", _overlayKeyboardId);
    idMapper.emplace("complete", _overlayCompleteButtonId);
    idMapper.emplace("submit", _overlaySubmitButtonId);
    idMapper.emplace("delete_character", _overlayDeleteCharacterButtonId);
	idMapper.emplace("paste", _overlayPasteButtonId);
    idMapper.emplace("space", _overlaySpaceButtonId);
    idMapper.emplace("text_edit", _overlayTextEditId);
    idMapper.emplace("word_suggest", _overlayWordSuggestId);
	idMapper.emplace("shift", _overlayShiftButtonId);
	idMapper.emplace("new_line", _overlayNewLineButtonId);
	idMapper.emplace("next_word", _overlayNextWordButtonId);
	idMapper.emplace("previous_word", _overlayPreviousWordButtonId);
	idMapper.emplace("next_letter", _overlayNextLetterButtonId);
	idMapper.emplace("previous_letter", _overlayPreviousLetterButtonId);
	idMapper.emplace("delete_all", _overlayDeleteAllButtonId);
	idMapper.emplace("layout", _overlayLayoutId);
	idMapper.emplace("layout_us_english", _overlayEnglishLayoutId);
	idMapper.emplace("layout_germany_german", _overlayGermanLayoutId);
	idMapper.emplace("layout_israel_hebrew", _overlayHebrewLayoutId);
	idMapper.emplace("layout_greece_greek", _overlayGreekLayoutId);
	idMapper.emplace("extra_keys", _overlayExtraKeyId);

    // Calculate size of overlay
    float x, y, sizeX, sizeY;
    x = (float)_pTab->GetWebViewX() / (float)_pTab->GetWindowWidth();
	y = (float)_pTab->GetWebViewY() / (float)_pTab->GetWindowHeight();
    sizeX = (float)_pTab->GetWebViewWidth() / (float)_pTab->GetWindowWidth();
    sizeY = (float)_pTab->GetWebViewHeight() / (float)_pTab->GetWindowHeight();

    // Add overlay
    _overlayFrameIndex = _pTab->AddFloatingFrameToOverlay("bricks/actions/Keyboard.beyegui", x, y, sizeX, sizeY, idMapper);

    // ### Register listeners

	// Keyboard
    _pTab->RegisterKeyboardListenerInOverlay(
        _overlayKeyboardId,
		[&](std::string value) // select callback
		{
			// ######################################################
			// ### TODO CERTH #######################################
			// ######################################################
			// This lambda function is called when ANY key on keyboard is selected by user.
			// In this example, the timer for classification is reset.

			// Start classification here (send something to LSL)
			_classificationTime = CLASSIFICATION_DURATION;

			// Tell member it was a key from keyboard
			_classifyingButton = false;

			// ######################################################

			// Send marker about key selection into lab streaming layer
			LabStreamMailer::instance().Send("GAZE_SELECTED_KEY_" + value);
		},
        [&](std::u16string value) // press callback
        {
			// Add content from keyboard
			_pTab->AddContentAtCursorInTextEdit(_overlayTextEditId, value);

			// Do logging about it
			JSMailer::instance().Send("keystroke");

			// Refresh suggestions
            _pTab->DisplaySuggestionsInWordSuggest(
				_overlayWordSuggestId,
				_pTab->GetActiveEntityContentInTextEdit(_overlayTextEditId));

			// Make letters in keyboard small again
			_pTab->ButtonUp(_overlayShiftButtonId);
        });

	// Complete button
    _pTab->RegisterButtonListenerInOverlay(
        _overlayCompleteButtonId,
        [&]() // down callback
        {
            this->_complete = true;
        },
		[](){}); // up callback

	// Submit button
    _pTab->RegisterButtonListenerInOverlay(
        _overlaySubmitButtonId,
        [&]() // down callback
        {
            this->_submit = true;
            this->_complete = true;
        },
		[](){}); // up callback

	// Delete character button
    _pTab->RegisterButtonListenerInOverlay(
        _overlayDeleteCharacterButtonId,
        [&]() // down callback
        {
			// Delete an letter from content
			_pTab->DeleteContentAtCursorInTextEdit(_overlayTextEditId, -1);

			// Refresh suggestions
			_pTab->DisplaySuggestionsInWordSuggest(_overlayWordSuggestId, _pTab->GetActiveEntityContentInTextEdit(_overlayTextEditId));
        },
		[](){}); // up callback

	// Paste button
	_pTab->RegisterButtonListenerInOverlay(
		_overlayPasteButtonId,
		[&]() // down callback
	{
		// Get text from global clipboard and convert it to UTF-16
		std::string clipboard = _pTab->GetClipboardText();
		std::u16string clipboard16;
		eyegui_helper::convertUTF8ToUTF16(clipboard, clipboard16);

		// Add text from clipboard to collected text
		_pTab->AddContentAtCursorInTextEdit(_overlayTextEditId, clipboard16);

		// Refresh suggestions
		_pTab->DisplaySuggestionsInWordSuggest(_overlayWordSuggestId, _pTab->GetActiveEntityContentInTextEdit(_overlayTextEditId));
	},
		[]() {}); // up callback

	// Space button
    _pTab->RegisterButtonListenerInOverlay(
        _overlaySpaceButtonId,
        [&]() // down callback
        {
			// Add space to content
			_pTab->AddContentAtCursorInTextEdit(_overlayTextEditId, u" ");

			// Clear suggestions
			_pTab->DisplaySuggestionsInWordSuggest(_overlayWordSuggestId, u"");
        },
		[](){}, // up callback
		[&]() // select callback
		{
			// ######################################################
			// ### TODO CERTH #######################################
			// ######################################################
			// This lambda function is called when the space bar button is selected by user.
			// In this example, the timer for classification is reset.

			// TODO: Deactivate input for all other buttons and the keyboard which interaction can be classified while classification.
			// Otherwise one can select something else for classification while the previous is still pending.

			// Start classification here (send something to LSL)
			_classificationTime = CLASSIFICATION_DURATION;

			// Tell member it was a button
			_classifyingButton = true;

			// ######################################################

			// Send marker about button selection into lab streaming layer
			LabStreamMailer::instance().Send("GAZE_SELECTED_BUTTON_SPACE");
		});

	// Word suggestion
    _pTab->RegisterWordSuggestListenerInOverlay(
        _overlayWordSuggestId,
        [&](std::u16string value)
        {
			// Fill chosen suggestion into text edit
			_pTab->SetActiveEntityContentInTextEdit(_overlayTextEditId, value);
        });

	// Shift button (switch)
	_pTab->RegisterButtonListenerInOverlay(
		_overlayShiftButtonId,
		[&]() // down callback
		{
			this->_pTab->SetCaseOfKeyboardLetters(_overlayKeyboardId, true);
		},
		[&]() // up callback
		{
			this->_pTab->SetCaseOfKeyboardLetters(_overlayKeyboardId, false);
		});

	// New line button
	_pTab->RegisterButtonListenerInOverlay(
		_overlayNewLineButtonId,
		[&]() // down callback
	{
		// Add new line to content
		_pTab->AddContentAtCursorInTextEdit(_overlayTextEditId, u"\n");
	},
	[]() {}); // up callback

	// Next word button
	_pTab->RegisterButtonListenerInOverlay(
		_overlayNextWordButtonId,
		[&]() // down callback
	{
		// Move one word righward
		_pTab->MoveCursorOverWordsInTextEdit(_overlayTextEditId, 1);
	},
	[]() {}); // up callback

	 // Previous word button
	_pTab->RegisterButtonListenerInOverlay(
		_overlayPreviousWordButtonId,
		[&]() // down callback
	{
		// Move one word leftward
		_pTab->MoveCursorOverWordsInTextEdit(_overlayTextEditId, -1);
	},
	[]() {}); // up callback

	// Next letter button
	_pTab->RegisterButtonListenerInOverlay(
		_overlayNextLetterButtonId,
		[&]() // down callback
	{
		// Move one letter righward
		_pTab->MoveCursorOverLettersInTextEdit(_overlayTextEditId, 1);
	},
	[]() {}); // up callback

	// Previous word button
	_pTab->RegisterButtonListenerInOverlay(
		_overlayPreviousLetterButtonId,
		[&]() // down callback
	{
		// Move one letter leftward
		_pTab->MoveCursorOverLettersInTextEdit(_overlayTextEditId, -1);
	},
	[]() {}); // up callback

	// Delete all button
	_pTab->RegisterButtonListenerInOverlay(
		_overlayDeleteAllButtonId,
		[&]() // down callback
	{
		// Delete content
		_pTab->DeleteContentInTextEdit(_overlayTextEditId);
	},
	[]() {}); // up callback

	// Localization layout
	_pTab->RegisterButtonListenerInOverlay(
		_overlayLayoutId,
		[&]() // down callback
	{
		// Deactivate keyboard and suggestions
		_pTab->SetElementActivity(_overlayKeyboardId, false, true);
		_pTab->SetElementActivity(_overlayWordSuggestId, false, true);
	},
		[&]() // up callback
	{
		// Activate keyboard and suggestions
		_pTab->SetElementActivity(_overlayKeyboardId, true, true);
		_pTab->SetElementActivity(_overlayWordSuggestId, true, true);
	}); 

	// English layout
	_pTab->RegisterButtonListenerInOverlay(
		_overlayEnglishLayoutId,
		[&]() // down callback
	{
		// Set keyboard layout
		_pTab->SetKeyboardLayout(eyegui::KeyboardLayout::US_ENGLISH);

		// Hide drop menu
		_pTab->ButtonUp(_overlayLayoutId);

		// Go to standard mode of keyboard
		_pTab->ButtonUp(_overlayExtraKeyId);
	},
	[](){}); // up callback

	// German layout
	_pTab->RegisterButtonListenerInOverlay(
		_overlayGermanLayoutId,
		[&]() // down callback
	{
		// Set keyboard layout
		_pTab->SetKeyboardLayout(eyegui::KeyboardLayout::GERMANY_GERMAN);

		// Hide drop menu
		_pTab->ButtonUp(_overlayLayoutId);

		// Go to standard mode of keyboard
		_pTab->ButtonUp(_overlayExtraKeyId);
	},
	[]() {}); // up callback

	// Hebrew layout
	_pTab->RegisterButtonListenerInOverlay(
		_overlayHebrewLayoutId,
		[&]() // down callback
	{
		// Set keyboard layout
		_pTab->SetKeyboardLayout(eyegui::KeyboardLayout::ISRAEL_HEBREW);

		// Hide drop menu
		_pTab->ButtonUp(_overlayLayoutId);

		// Go to standard mode of keyboard
		_pTab->ButtonUp(_overlayExtraKeyId);
	},
	[]() {}); // up callback

	// Greek layout
	_pTab->RegisterButtonListenerInOverlay(
		_overlayGreekLayoutId,
		[&]() // down callback
	{
		// Set keyboard layout
		_pTab->SetKeyboardLayout(eyegui::KeyboardLayout::GREECE_GREEK);

		// Hide drop menu
		_pTab->ButtonUp(_overlayLayoutId);

		// Go to standard mode of keyboard
		_pTab->ButtonUp(_overlayExtraKeyId);
	},
	[]() {}); // up callback

	// Extra keys
	_pTab->RegisterButtonListenerInOverlay(
		_overlayExtraKeyId,
		[&]() // down callback
	{
		// Set keymap for extra keys
		_pTab->SetKeymapOfKeyboard(_overlayKeyboardId, 1);
	},
		[&]() // up callback
	{
		// Set keymap for standard keys
		_pTab->SetKeymapOfKeyboard(_overlayKeyboardId, 0);
	});

	// Create callback for lab streaming layer to send classificatoin
	_spLabStreamCallback = std::shared_ptr<LabStreamCallback>(new LabStreamCallback(
		[this](std::vector<std::string> messages)
	{
		for (const std::string& rMessage : messages)
		{
			// Check for the message content
			LogInfo("ERRP Message: ", rMessage);
			if (!strcmp(rMessage.c_str(), "-1"))
			{
				// Just delete last character
				_pTab->DeleteContentAtCursorInTextEdit(_overlayTextEditId, -1);
			}
		}
	}
	));

	// Register that callback
	LabStreamMailer::instance().RegisterCallback(_spLabStreamCallback);
}

KeyboardAction::~KeyboardAction()
{
    // Delete overlay frame
    _pTab->RemoveFloatingFrameFromOverlay(_overlayFrameIndex);

    // Unregister listener of elements in overlay
    _pTab->UnregisterKeyboardListenerInOverlay(_overlayKeyboardId);
    _pTab->UnregisterButtonListenerInOverlay(_overlayCompleteButtonId);
    _pTab->UnregisterButtonListenerInOverlay(_overlaySubmitButtonId);
    _pTab->UnregisterButtonListenerInOverlay(_overlayDeleteCharacterButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayPasteButtonId);
    _pTab->UnregisterButtonListenerInOverlay(_overlaySpaceButtonId);
    _pTab->UnregisterWordSuggestListenerInOverlay(_overlayWordSuggestId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayShiftButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayNewLineButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayNextWordButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayPreviousWordButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayNextLetterButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayPreviousLetterButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayDeleteAllButtonId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayLayoutId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayEnglishLayoutId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayGermanLayoutId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayHebrewLayoutId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayGreekLayoutId);
	_pTab->UnregisterButtonListenerInOverlay(_overlayExtraKeyId);
}

bool KeyboardAction::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject)
{
	// Update duration
	float duration = 0.f;
	GetOutputValue("duration", duration);
	SetOutputValue("duration", duration + tpf);

	// Voice!
	if ((spVoiceInput->command == VoiceCommand::PARAMETER_ONLY || spVoiceInput->command == VoiceCommand::TEXT) && !spVoiceInput->parameter.empty()) {
		std::u16string paramU16;
		eyegui_helper::convertUTF8ToUTF16(spVoiceInput->parameter, paramU16);

		SetOutputValue("text", paramU16);

		// Submit text directly if wished
		SetOutputValue("submit", _submit);
		JSMailer::instance().Send("submit");

		// Action is now finished
		return true;
	}



	// When classification timer is set, it is decremented at each update.
	// When timer is zero, selection is ALWAYS accepted. Please change as required.

	// Check classification
	if (_classificationTime > 0)
	{
		_classificationTime -= tpf; // decrement timer
		_classificationTime = glm::max(0.f, _classificationTime); // lower limit of timer

		// When timer is complete, accept selection
		if (_classificationTime <= 0)
		{
			if (_classifyingButton)
			{
				_pTab->ClassifyButton(_overlaySpaceButtonId, true); // true for accept (TODO: encode id of button-to-classify in LabStream message or store it in member)
			}
			else // key from keyboard
			{
				_pTab->ClassifyKey(_overlayKeyboardId, true); // true for accept
			}
		}
	}

	// ######################################################

	// Decide whether action is complete
    if (_complete)
    {
		// Fill collected input to output
		SetOutputValue("text", _pTab->GetContentOfTextEdit(_overlayTextEditId));			
		
		// Submit text directly if wished
		SetOutputValue("submit", _submit);
		JSMailer::instance().Send("submit");

        // Action is now finished
        return true;
    }
    else
    {
        return false;
    }
}

void KeyboardAction::Draw() const
{
    // Nothing to do
}

void KeyboardAction::Activate()
{
	// Set visibility of floating frame
    _pTab->SetVisibilityOfFloatingFrameInOverlay(_overlayFrameIndex, true);

	// Get text from input
	std::u16string text;
	GetInputValue<std::u16string>("text", text);

	// Put text into preview
	_pTab->AddContentAtCursorInTextEdit(_overlayTextEditId, text); // TODO: set content would be better here
}

void KeyboardAction::Deactivate()
{
	// Set visibility of floating frame
    _pTab->SetVisibilityOfFloatingFrameInOverlay(_overlayFrameIndex, false);
}

void KeyboardAction::Abort()
{
    // Nothing to do
}