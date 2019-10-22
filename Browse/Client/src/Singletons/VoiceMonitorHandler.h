//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

#pragma once
#include <windows.h>
#include <string>
#include <cstring> 
#include <deque>
#include <codecvt>
#include <mutex>
#include "src/Input/VoiceInput.h"

enum class PrintCategory {
	CURRENT_MICROPHONE,
	AVAILABLE_COMMANDS,
	CONNECTION_GOOGLE,
	SENT_SECONDS,
	LAST_WORD,
	CURRENT_ACTION
};

struct PrintStruct {

	std::wstring currentMicrophone;
	std::wstring availableCommands;
	std::wstring connectionGoogle;
	std::wstring sentSeconds;
	std::deque<std::wstring> lastWords;
	std::wstring currentAction;
};

class VoiceMonitorHandler
{
public:

	// Get instance
	static VoiceMonitorHandler& instance()
	{
		static VoiceMonitorHandler _instance;
		return _instance;
	}

	// Destructor
	~VoiceMonitorHandler() {};
	
	void SetWindow(HWND hwnd);
	HWND GetWindow();

	// Handle the Voice Input
	void SetVoiceInput(std::shared_ptr<VoiceInput> spVoiceInputObject);
	void ToggleVoiceInput();

	// Sets the text corresponding to the provided printCategory and sends a package to the _hwnd window.
	void SetNewText(PrintCategory printCategory, std::wstring newText);

private:

	// Window id
	HWND _hwnd;

	// Holds the current values for the monitor.
	PrintStruct _printStruct;

	// Static text
	std::wstring _currentMicrophone = L"Current used Microphone: ";
	std::wstring _availableCommands = L"AvailableCommands: ";
	std::wstring _connectionGoogle = L"Connection to Google: ";
	std::wstring _sentSeconds = L"Sent seconds: ";
	std::wstring _lastWords = L"Last Words: ";
	std::wstring _currentAction = L"Current Action: ";
	std::wstring _lineBreak = L"\r\n";

	// VoiceInput Object
	std::shared_ptr<VoiceInput> _spVoiceInputObject;

	// Helper function to set the text corresponding to the provided printCategory.
	void VoiceMonitorHandler::AddUpdate(PrintCategory printCategory, std::wstring newText);
	LPCWSTR VoiceMonitorHandler::EvaluateLog();
};
