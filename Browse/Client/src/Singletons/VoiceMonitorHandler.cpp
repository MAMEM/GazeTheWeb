//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#include "VoiceMonitorHandler.h"

void VoiceMonitorHandler::setWindow(HWND hwnd) {
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
	instance()._printStruct.currentAction = L"";
};

HWND VoiceMonitorHandler::getWindow() {
	return instance()._hwnd;
};


void VoiceMonitorHandler::addUpdate(PrintCategory printCategory, std::wstring newText) {
	switch (printCategory)
	{
	case PrintCategory::CURRENTMICROPHONE:
	{
		instance()._printStruct.currentMicrophone = newText;
	}
		break;	
	case PrintCategory::AVAILABLECOMMANDS:
	{
		instance()._printStruct.availableCommands = newText;
	}
		break;
	case PrintCategory::CONNECTIONGOOGLE:
	{
		instance()._printStruct.connectionGoogle = newText;
	}
		break;
	case PrintCategory::SENTSECONDS:
	{
		instance()._printStruct.sentSeconds = newText;
	}
		break;
	case PrintCategory::LASTWORD:
	{
		if (instance()._printStruct.lastWords.size() > 2)
			instance()._printStruct.lastWords.pop_back();

		instance()._printStruct.lastWords.push_front(newText);
	}
		break;
	case PrintCategory::CURRENTACTION:
	{
		instance()._printStruct.currentAction = newText;
	}
		break;
	default:
		break;
	}
}

LPCWSTR VoiceMonitorHandler::evaluateLog() {

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

	strResult += instance()._currentAction;
	strResult += instance()._printStruct.currentAction;
	strResult += instance()._lineBreak;


	LPCWSTR result = strResult.c_str();

	return result;
}

void VoiceMonitorHandler::setNewText(PrintCategory printCategory, std::wstring newText) {

	// Prepare package to send to the window.
	addUpdate(printCategory, newText);

	// Code from here till the end maybe in some sort of "Update" function?
	LPCWSTR message = evaluateLog();

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
