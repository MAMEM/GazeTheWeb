#include <windows.h>
#include "src/CEF/MainCefApp.h"
#include "src/CEF/RenderProcess/RenderCefApp.h"
#include "src/CEF/OtherProcess/DefaultCefApp.h"
#include "src/CEF/ProcessTypeGetter.h"
#include "include/cef_sandbox_win.h"
#include <thread>
#include "Singletons/VoiceMonitorHandler.h"

#if defined(CEF_USE_SANDBOX)
#pragma comment(lib, "cef_sandbox.lib")
#endif

std::unique_ptr<std::thread> _tvoiceMonitor;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
	case WM_CREATE:
	{
		CreateWindow(TEXT("button"), TEXT("Toggle Recording"),
			WS_VISIBLE | WS_CHILD,
			20,		// x
			550,	// y
			200,	// width
			25,		// height
			hwnd, (HMENU)1, NULL, NULL);
		break;
	}
	case WM_COMMAND:
	{
		// Mute was pressed
		if (LOWORD(wParam) == 1) {
			VoiceMonitorHandler::instance().ToggleVoiceInput();
		}
		break;
	}
	case WM_ERASEBKGND:
		return 1;
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COPYDATA:
	{
		COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;
		if (pcds->dwData == 1)
		{
			LPCWSTR message = (LPCWSTR)(pcds->lpData);

			RECT rect;
			HDC wdc = GetDC(hwnd);
			BitBlt(wdc, 0, 0, 600, 500, 0, 0, 0, WHITENESS);
			//InvalidateRect(hwnd, NULL, true);
			GetClientRect(hwnd, &rect);
			SetTextColor(wdc, 0x00000000);
			SetBkMode(wdc, OPAQUE);
			rect.left = 1;
			rect.top = 1;
			DrawText(wdc, message, -1, &rect, DT_NOCLIP);
			ReleaseDC(hwnd, wdc);

		}
	}
	break;
	default:
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

// Forward declaration of common main
int CommonMain(const CefMainArgs& args, CefSettings settings, CefRefPtr<MainCefApp> app, void* windows_sandbox_info, std::string userDirectory, bool useVoice);

// Platform specific shutdown
void shutdown()
{
	system("shutdown -s");
}

// Following taken partly out of CefSimple example of Chromium Embedded Framework!

// Entry point function for all processes.
int APIENTRY wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	// Check commandline args
	LPTSTR check = L"--voice-input";
	bool useVoice = (lpCmdLine && (lstrcmpW(check, lpCmdLine) == 0));


	// Enable High-DPI support on Windows 7 or newer.
	// CefEnableHighDPISupport();

	// Provide CEF with command-line arguments.
	CefMainArgs main_args(hInstance);

	// ###############
	// ### SANDBOX ###
	// ###############

	// Sandbox information.
	void* sandbox_info = NULL;

#if defined(CEF_USE_SANDBOX)
	// Manage the life span of the sandbox information object. This is necessary
	// for sandbox support on Windows. See cef_sandbox_win.h for complete details.
	CefScopedSandboxInfo scoped_sandbox;
	sandbox_info = scoped_sandbox.sandbox_info();
#endif

	// ###############
	// ### PROCESS ###
	// ###############

	// Parse command-line arguments.
	CefRefPtr<CefCommandLine> commandLine = CefCommandLine::CreateCommandLine();
	commandLine->InitFromString(::GetCommandLineW());

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


		// If voice input show logging window
		// Register the window class.
		if (useVoice) {
			HWND hwnd;
			const wchar_t voiceMonitor[] = L"Voice Monitor";
			MSG msg = { };
			WNDCLASS wc = { };

			wc.lpfnWndProc = WndProc;
			wc.hInstance = hInstance;
			wc.lpszClassName = voiceMonitor;

			RegisterClass(&wc);

			// Create the window.

			hwnd = CreateWindowExW(
				0,                              // Optional window styles.
				voiceMonitor,                   // Window class
				L"Voice Monitor",				// Window text
				WS_OVERLAPPEDWINDOW,            // Window style

				// Size and position
				CW_USEDEFAULT,					// X
				CW_USEDEFAULT,					// Y
				600,							// Width
				800,							// Height

				NULL,       // Parent window    
				NULL,       // Menu
				hInstance,  // Instance handle
				NULL        // Additional application data
			);
			_tvoiceMonitor = std::make_unique<std::thread>([hInstance, nCmdShow, &hwnd, &msg] {

				ShowWindow(hwnd, nCmdShow);
				while (GetMessageW(&msg, NULL, 0, 0))
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
				return msg.wParam;
				});
			VoiceMonitorHandler::instance().SetWindow(hwnd);
		}
		break;

	case ProcessType::RENDER:

		// Render process
		app = new RenderCefApp();
		break;

	default:

		// Any other process
		app = new DefaultCefApp();
		break;
	}

	// Execute process. Returns for main process zero.
	int exit_code = CefExecuteProcess(main_args, app.get(), sandbox_info);
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

#if !defined(CEF_USE_SANDBOX)
	settings.no_sandbox = true;
#endif

	// Fetch directory for saving bookmarks etc.
	char *pValue;
	size_t len;
	// errno_t err = _dupenv_s(&pValue, &len, "APPDATA"); // not used err produces compiler warning
	_dupenv_s(&pValue, &len, "APPDATA");
	std::string userDirectory(pValue, len - 1);

    // Create project folder.
	userDirectory.append("\\GazeTheWeb");
	if (CreateDirectoryA(userDirectory.c_str(), NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError())
	{
        // Everything ok.
	}
	else
	{
        // Folder could not be created.
	}

    // Create application folder.
	userDirectory.append("\\BrowseData");
	if (CreateDirectoryA(userDirectory.c_str(), NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError())
	{
        // Everything ok.
	}
	else
	{
        // Folder could not be created.
	}

	// Create bmp folder.
	const char* bmpDir = ".\\bmp";
	if (CreateDirectoryA(bmpDir, NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError())
	{
		// Everything ok.
	}
	else
	{
		// Folder could not be created.
	}

    // Append another slash for easier usage of that path.
	userDirectory.append("\\");

	// Use common main now.
	return CommonMain(main_args, settings, mainProcessApp, sandbox_info, userDirectory, useVoice);
}
