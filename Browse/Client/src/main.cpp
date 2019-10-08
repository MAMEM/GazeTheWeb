//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifdef _WIN32
// Windows
#include "main_windows.h"
#elif __linux__
// Linux
#include "main_linux.h"
#endif

#include "src/Master/Master.h"
#include "src/Utils/Logger.h"


// Execute function to have Master object on stack which might be faster than on heap
bool Execute(CefRefPtr<MainCefApp> app, std::string userDirectory, bool useVoice) // returns whether system should shut down
{
    // Initialize master
    Master master(app.get(), userDirectory, useVoice);

	// Give app poiner to master (only functions exposed through interface are accessible)
	app->SetMaster(&master);

    // Run master which communicates with CEF over mediator
    return master.Run();

    // Destructor of master is called implicity
}

// Common main for linux and windows
int CommonMain(const CefMainArgs& args, CefSettings settings, CefRefPtr<MainCefApp> app, void* windows_sandbox_info, std::string userDirectory, bool useVoice)
{

#ifdef CLIENT_DEPLOYMENT

	// Disable logging of CEF
	settings.log_severity = LOGSEVERITY_DISABLE;

#else // if no deployment, open console

	// Open Windows console for debugging purposes (even when deployed)
#ifdef _WIN32
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
#endif // _WIN32

#endif // CLIENT_DEPLOYMENT

	// Set path for CEF data: cache, user data and debug.log
	CefString(&settings.cache_path).FromASCII(std::string(userDirectory + "cache").c_str());
	CefString(&settings.user_data_path).FromASCII(std::string(userDirectory + "user_data").c_str());
	CefString(&settings.log_file).FromASCII(std::string(userDirectory + "debug.log").c_str());
	
	// Set output path of custom log file
	LogPath = userDirectory;

	// Say hello
	LogInfo("####################################################");
	LogInfo("Welcome to GazeTheWeb - Browse!");
	LogInfo("Version: ", CLIENT_VERSION);
	LogInfo("Personal files are saved in: ", userDirectory);

	// Turn on offscreen rendering.
	settings.windowless_rendering_enabled = true;
	settings.remote_debugging_port = 8088;

    // Initialize CEF
    LogInfo("Initializing CEF...");
    CefInitialize(args, settings, app.get(), windows_sandbox_info);
    LogInfo("..done.");

    // Execute our code
    bool shutdownOnExit = Execute(app, userDirectory, useVoice);


	// Shutdown CEF
    LogInfo("Shutdown CEF...");
    CefShutdown();
    LogInfo("..done.");

    // Return zero
    LogInfo("Successful termination of program.");
    LogInfo("####################################################");

	// Tell computer to shut down
	if (shutdownOnExit)
	{
		shutdown();
	}

	// Exit
	return 0;
}
