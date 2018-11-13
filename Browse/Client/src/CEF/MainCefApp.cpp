//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "MainCefApp.h"
#include "src/Setup.h"
#include "src/CEF/Handler.h"
#include "src/CEF/Renderer.h"
#include "src/CEF/DevToolsHandler.h"
#include "src/Utils/Logger.h"
#include "src/Arguments.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include <string>

void MainCefApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr< CefCommandLine > command_line)
{	
	// Parse custom arguments
	std::vector<CefString> arguments;
	command_line->GetArgv(arguments);
	for (const auto& rString : arguments)
	{
		if (rString.compare("--localization=english") == 0)
		{
			Argument::localization = Argument::Localization::English;
		}
		else if (rString.compare("--localization=greek") == 0)
		{
			Argument::localization = Argument::Localization::Greek;
		}
		else if (rString.compare("--localization=hebrew") == 0)
		{
			Argument::localization = Argument::Localization::Hebrew;
		} // no else!
	}

	// Enable WebGL part 2 (other is in CefMediator.cpp) (or not disable is better description)
	if (!setup::ENABLE_WEBGL)
	{
		// Setup for offscreen rendering
		command_line->AppendSwitch("disable-gpu");
		command_line->AppendSwitch("disable-gpu-compositing");
		command_line->AppendSwitch("enable-begin-frame-scheduling"); // breaks WebGL, but better for performance
	}

	// Enable media input
	command_line->AppendSwitch("enable-media-stream");
	command_line->AppendSwitch("enable-speech-input");

#ifndef CLIENT_DEPLOYMENT
	command_line->AppendSwitch("enable-logging"); // get LOG(..) writes in console
#endif

	// EXPERIMENTAL: slow loading?
	// see end of https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage#markdown-header-proxy-resolution
	command_line->AppendSwitch("no-proxy-server");

	// EXPERIMENTAL
	// Javascript debugging?
	command_line->AppendArgument("remote-debugging-port=2012");
}

void MainCefApp::OnContextInitialized()
{
    CEF_REQUIRE_UI_THREAD();

	// TODO: handlers are members in Mediator but created here. Looks strange

    // Create Renderer
    CefRefPtr<Renderer> renderer(new Renderer(this));

    // Create Handler
	_handler = new Handler(this, renderer);

	// Create handler for dev tools
	_devToolsHandler = new DevToolsHandler();
}