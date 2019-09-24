//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#include "voiceMonitorHandler.h"

HWND voiceMonitorHandler::_hwnd;
PrintStruct voiceMonitorHandler::_printStruct;
std::wstring voiceMonitorHandler::_currentMicrophone = L"Current used Microphone: ";
std::wstring voiceMonitorHandler::_availableCommands = L"AvailableCommands: ";
std::wstring voiceMonitorHandler::_connectionGoogle = L"Connection to Google: ";
std::wstring voiceMonitorHandler::_sentSeconds = L"Sent seconds: ";
std::wstring voiceMonitorHandler::_lastWords = L"Last Words: ";
std::wstring voiceMonitorHandler::_currentAction = L"Current Action: ";
std::wstring voiceMonitorHandler::_lineBreak = L"\r\n";
std::mutex voiceMonitorHandler::_mupdate;

voiceMonitorHandler::voiceMonitorHandler()
{
}

void voiceMonitorHandler::setWindow(HWND hwnd) {
	_hwnd = hwnd;

	_printStruct.currentMicrophone = L"";
	_printStruct.availableCommands = L" \r\n\
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
	_printStruct.connectionGoogle = L"off";
	_printStruct.sentSeconds = L"";
	_printStruct.lastWords.push_front(L""); // DATATYPE?
	_printStruct.currentAction = L"";
};

HWND voiceMonitorHandler::getWindow() {
	return  _hwnd;
};


void voiceMonitorHandler::addUpdate(PrintCategory printCategory, std::wstring newText) {
	switch (printCategory)
	{
	case PrintCategory::CURRENTMICROPHONE:
	{
		if (newText != _printStruct.currentMicrophone) // TODO: if not necessary
			_printStruct.currentMicrophone = newText;
	}
		break;	
	case PrintCategory::AVAILABLECOMMANDS:
	{
		if (newText != _printStruct.availableCommands)
			_printStruct.availableCommands = newText;
	}
		break;
	case PrintCategory::CONNECTIONGOOGLE:
	{
		if (newText != _printStruct.connectionGoogle)
			_printStruct.connectionGoogle = newText;
	}
		break;
	case PrintCategory::SENTSECONDS:
	{
		if (newText != _printStruct.sentSeconds)
			_printStruct.sentSeconds = newText;
	}
		break;
	case PrintCategory::LASTWORD:
	{
		if (newText != _printStruct.lastWords.back()) {
			if (_printStruct.lastWords.size() > 2)
				_printStruct.lastWords.pop_back();

			_printStruct.lastWords.push_front(newText);
		}
	}
		break;
	case PrintCategory::CURRENTACTION:
	{
		if (newText != _printStruct.currentAction)
			_printStruct.currentAction = newText;
	}
		break;
	default:
		break;
	}
}

LPCWSTR voiceMonitorHandler::evaluateLog() {
	//char* result = "";
	
	std::wstring strResult = L"";

	strResult += _currentMicrophone;
	strResult += _printStruct.currentMicrophone;
	strResult += _lineBreak;

	strResult += _availableCommands;
	strResult += _printStruct.availableCommands;
	strResult += _lineBreak;

	strResult += _connectionGoogle;
	strResult += _printStruct.connectionGoogle;
	strResult += _lineBreak;

	strResult += _sentSeconds;
	strResult += _printStruct.sentSeconds;
	strResult += _lineBreak;

	strResult += _lastWords;
	for (int i = 0; i < _printStruct.lastWords.size(); i++) {
		strResult += L"\"";
		strResult += _printStruct.lastWords[i];
		strResult += L"\"";
		if (i < _printStruct.lastWords.size() - 1)
			strResult += L", ";
	}
	strResult += _lineBreak;

	strResult += _currentAction;
	strResult += _printStruct.currentAction;
	strResult += _lineBreak;


	LPCWSTR result = strResult.c_str();

	return result;
}

void voiceMonitorHandler::setNewText(PrintCategory printCategory, std::wstring newText) {

	// Only prepare/send one package at the time
	std::lock_guard<std::mutex> lock(_mupdate); // TODO: think about it

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
