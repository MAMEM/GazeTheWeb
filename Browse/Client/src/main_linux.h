#include <stdlib.h>
#include <sys/stat.h>
#include "src/CEF/MainCefApp.h"
#include "src/CEF/OtherProcess/DefaultCefApp.h"
#include "src/CEF/ProcessTypeGetter.h"
#include "include/base/cef_logging.h"

// Forward declaration of common main
int CommonMain(const CefMainArgs& args, CefSettings settings, CefRefPtr<App> app, void* windows_sandbox_info, std::string userDirectory);

// Following taken partly out of CefSimple example of Chromium Embedded Framework!

// Entry point function for all processes.
int main(int argc, char* argv[])
{
	// Provide CEF with command-line arguments.
	CefMainArgs main_args(argc, argv);

	// ###############
	// ### PROCESS ###
	// ###############

	// Parse command-line arguments
	CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
	command_line->InitFromArgv(argc, argv);

	// Create an app of the correct type.
	CefRefPtr<CefApp> app;
	CefRefPtr<MainCefApp> mainProcessApp; // extra pointer to main process app implementation. Only filled on main process.
	ProcessType processType = ProcessTypeGetter::GetProcessType(commandLine);
	switch (processType)
	{
	case ProcessType::MAIN:

		// Main process
		mainProcessApp = new MainCefApp();
		app = mainProcessApp;
		break;

	case ProcessType::RENDER:

		// Render process
		app = new MainCefApp(); // TODO: different app implementation
		break;

	default:

		// Any other process
		app = new DefaultCefApp();
		break;
	}

    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
    // that share the same executable. This function checks the command-line and,
    // if this is a sub-process, executes the appropriate logic.
    int exit_code = CefExecuteProcess(main_args, app.get(), NULL);
    if (exit_code >= 0 || mainProcessApp.get() == nullptr)
    {
        // The sub-process has completed so return here.
        return exit_code;
    }

	// ################
	// ### SETTINGS ###
	// ################

    // Specify CEF global settings here.
    CefSettings settings;

    // Fetch directory for saving bookmarks etc.
    std::string userDirectory(getenv("HOME"));

    // Create .config folder, which should already exist.
    userDirectory.append("/.config");
    int dir_err = mkdir(userDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err)
    {
        // Folder could not be created or already existed.
    }

    // Create project folder.
    userDirectory.append("/GazeTheWeb");
    dir_err = mkdir(userDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err)
    {
        // Folder could not be created or already existed.
    }

    // Create .config folder, which should already exist.
    userDirectory.append("/Browse");
    dir_err = mkdir(userDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err)
    {
        // Folder could not be created or already existed.
    }

    // Append another slash for easier usage of that path.
    userDirectory.append("/");

    // Use common main now.
    return CommonMain(main_args, settings, mainProcessApp, NULL, userDirectory);
}
