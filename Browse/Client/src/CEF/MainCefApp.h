//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel M�ller (muellerd@uni-koblenz.de)
//============================================================================

#ifndef CEF_MAINCEFAPP_H_
#define CEF_MAINCEFAPP_H_

#include "include/cef_app.h"
#include "src/CEF/Mediator.h"
#include "src/CEF/RenderProcess/RenderProcessHandler.h"

class MainCefApp :	public CefApp,
					public CefBrowserProcessHandler,
					// expanding CefApp by our mediator interface
					public Mediator
{
public:

	MainCefApp();

    // Manipulate command line input
    virtual void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr< CefCommandLine > command_line) OVERRIDE;

    // CefBrowserProcessHandler methods
    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE { return this; }
    virtual void OnContextInitialized() OVERRIDE;

    // Called for IPC message navigation
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE {	return _renderProcessHandler; }

private:

    // Keep a reference to RenderProcessHandler
    CefRefPtr<RenderProcessHandler> _renderProcessHandler;

    // Include CEF'S default reference counting implementation
    IMPLEMENT_REFCOUNTING(MainCefApp);
};

#endif  // CEF_MAINCEFAPP_H_
