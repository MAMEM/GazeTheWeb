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

enum class PrintCategory {
	CURRENTMICROPHONE, // underscore for space
	AVAILABLECOMMANDS,
	CONNECTIONGOOGLE,
	SENTSECONDS,
	LASTWORD,
	CURRENTACTION
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
	
	void setWindow(HWND hwnd);
	HWND getWindow();

	// Sets the text corresponding to the provided printCategory and sends a package to the _hwnd window.
	void setNewText(PrintCategory printCategory, std::wstring newText);

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

	// Helper function to set the text corresponding to the provided printCategory.
	void VoiceMonitorHandler::addUpdate(PrintCategory printCategory, std::wstring newText);
	LPCWSTR VoiceMonitorHandler::evaluateLog();
};
