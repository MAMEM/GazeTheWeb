//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Christopher Dreide (cdreide@uni-koblenz.de)
//============================================================================

// TODO: why small case name? both files and class

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

class voiceMonitorHandler
{
public:
	voiceMonitorHandler();
	static void setWindow(HWND hwnd);
	static HWND getWindow();

	// Sets the text corresponding to the provided printCategory and sends a package to the _hwnd window.
	static void setNewText(PrintCategory printCategory, std::wstring newText);

	virtual ~voiceMonitorHandler() {};

private:
	static HWND _hwnd;

	// Holds the current values for the monitor.
	static PrintStruct _printStruct;

	// Static text
	static std::wstring _currentMicrophone;
	static std::wstring _availableCommands;
	static std::wstring _connectionGoogle;
	static std::wstring _sentSeconds;
	static std::wstring _lastWords;
	static std::wstring _currentAction;
	static std::wstring _lineBreak;

	// Helper function to set the text corresponding to the provided printCategory.
	static void voiceMonitorHandler::addUpdate(PrintCategory printCategory, std::wstring newText);
	static LPCWSTR voiceMonitorHandler::evaluateLog();

	static std::mutex _mupdate;
};

