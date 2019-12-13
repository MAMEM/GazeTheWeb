//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#include "VoiceMonitorHandler.h"

void VoiceMonitorHandler::SetWindow(HWND hwnd) {
	instance()._hwnd = hwnd;

	instance()._printStruct.currentMicrophone = L"";
	instance()._printStruct.availableCommands = L" \r\n\
				SCROLL_UP, \r\n\
				SCROLL_DOWN, \r\n\
				TOP, \r\n\
				BOTTOM, \r\n\
				BOOKMARK, \r\n\
				BACK, \r\n\
				REFRESH, \r\n\
				FORWARD, \r\n\
				GO_TO, \r\n\
				CHECK, \r\n\
				VIDEO_INPUT, \r\n\
				INCREASE, \r\n\
				DECREASE, \r\n\
				PLAY, \r\n\
				STOP, \r\n\
				MUTE, \r\n\
				UNMUTE, \r\n\
				TEXT, \r\n\
				REMOVE, \r\n\
				SUBMIT, \r\n\
				QUIT \
		";
	instance()._printStruct.connectionGoogle = L"off";
	instance()._printStruct.sentSeconds = L"";
	instance()._printStruct.lastWords.push_front(L""); // DATATYPE?
	instance()._printStruct.currentMetaphoneAction = L"";
	instance()._printStruct.currentSoundexAction = L"";
	instance()._printStruct.currentLevenshteinAction = L"";
};

HWND VoiceMonitorHandler::GetWindow() {
	return instance()._hwnd;
};

void VoiceMonitorHandler::SetVoiceInput(std::shared_ptr<VoiceInput> spVoiceInputObject) {
	_spVoiceInputObject = spVoiceInputObject;
}

void VoiceMonitorHandler::ToggleVoiceInput() {

	if (_spVoiceInputObject->GetState() == VoiceInputState::Active)
		auto tDeactivating = std::make_unique<std::thread>([this] {
		_spVoiceInputObject->Deactivate();
	});
	else if (_spVoiceInputObject->GetState() == VoiceInputState::Inactive)
		auto tActivating = std::make_unique<std::thread>([this] {
		_spVoiceInputObject->Activate();
	});
}

void VoiceMonitorHandler::AddUpdate(PrintCategory printCategory, std::wstring newText) {
	switch (printCategory)
	{
	case PrintCategory::CURRENT_MICROPHONE:
	{
		instance()._printStruct.currentMicrophone = newText;
		break;
	}
	case PrintCategory::AVAILABLE_COMMANDS:
	{
		instance()._printStruct.availableCommands = newText;
		break;
	}
	case PrintCategory::CONNECTION_GOOGLE:
	{
		instance()._printStruct.connectionGoogle = newText;
		break;
	}
	case PrintCategory::SENT_SECONDS:
	{
		instance()._printStruct.sentSeconds = newText;
		break;
	}
	case PrintCategory::LAST_WORD:
	{
		if (instance()._printStruct.lastWords.size() > 2)
			instance()._printStruct.lastWords.pop_back();

		instance()._printStruct.lastWords.push_front(newText);
		break;
	}
	case PrintCategory::CURRENT_METAPHONE_ACTION:
	{
		instance()._printStruct.currentMetaphoneAction = newText;
	}
	break;
	case PrintCategory::CURRENT_SOUNDEX_ACTION:
	{
		instance()._printStruct.currentSoundexAction = newText;
	}
	break;
	case PrintCategory::CURRENT_LEVENSHTEIN_ACTION:
	{
		instance()._printStruct.currentLevenshteinAction = newText;
	}
	default:
		break;
	}
}

LPCWSTR VoiceMonitorHandler::EvaluateLog() {

	std::wstring strResult = L"";

	strResult += instance()._currentMicrophone;
	strResult += instance()._printStruct.currentMicrophone;
	strResult += instance()._lineBreak;

	strResult += instance()._availableCommands;
	strResult += instance()._printStruct.availableCommands;
	strResult += instance()._lineBreak;

	strResult += instance()._connectionGoogle;
	strResult += instance()._printStruct.connectionGoogle;
	strResult += instance()._lineBreak;

	strResult += instance()._sentSeconds;
	strResult += instance()._printStruct.sentSeconds;
	strResult += instance()._lineBreak;

	strResult += instance()._lastWords;
	for (int i = 0; i < _printStruct.lastWords.size(); i++) {
		strResult += L"\"";
		strResult += instance()._printStruct.lastWords[i];
		strResult += L"\"";
		if (i < instance()._printStruct.lastWords.size() - 1)
			strResult += L", ";
	}
	strResult += instance()._lineBreak;

	strResult += instance()._currentMetaphoneAction;
	strResult += instance()._printStruct.currentMetaphoneAction;
	strResult += instance()._lineBreak;

	strResult += instance()._currentSoundexAction;
	strResult += instance()._printStruct.currentSoundexAction;
	strResult += instance()._lineBreak;

	strResult += instance()._currentLevenshteinAction;
	strResult += instance()._printStruct.currentLevenshteinAction;
	strResult += instance()._lineBreak;

	LPCWSTR result = strResult.c_str();

	return result;
}

void VoiceMonitorHandler::SetNewText(PrintCategory printCategory, std::wstring newText) {

	// Prepare package to send to the window.
	AddUpdate(printCategory, newText);

	// Code from here till the end maybe in some sort of "Update" function?
	LPCWSTR message = EvaluateLog();

	// Prepare package to send to the window.
	COPYDATASTRUCT cds;
	cds.dwData = 1;
	cds.cbData = sizeof(WCHAR) * (wcslen(message) + 1);
	cds.lpData = (WCHAR*)message;

	// Finally send the message to the window.
	SendMessageW(
		_hwnd,
		WM_COPYDATA,
		(WPARAM)_hwnd,
		(LPARAM)(LPVOID)&cds
	);
};
