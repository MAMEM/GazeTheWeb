//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Handles messages from JavaScript on Main Process coming from webpage on
// the Render Process.

#ifndef CEF_MESSAGEROUTER_H_
#define CEF_MESSAGEROUTER_H_

#include "src/Utils/Helper.h"
#include "include/wrapper/cef_message_router.h"
#include "include/cef_base.h"
#include <functional>

class Mediator; // Forward declaration

// Default handler for messages
class DefaultMsgHandler : public CefMessageRouterBrowserSide::Handler
{
public:

	// Constructor
	DefaultMsgHandler(Mediator* pMediator) { _pMediator = pMediator; }

	// Called when |cefQuery| was called in JavaScript
	virtual bool OnQuery(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		int64 query_id,
		const CefString& request,
		bool persistent,
		CefRefPtr<Callback> callback) OVERRIDE;

private:

	// Pointer to mediator (TODO: some extra interface?)
	Mediator* _pMediator;
};

// Callback handler
class CallbackMsgHandler : public CefMessageRouterBrowserSide::Handler
{
public:

	// Constructor
	CallbackMsgHandler(std::string prefix, std::function<void(std::string)> callbackFunction)
	{
		_prefix = prefix;
		_callbackFunction = callbackFunction;
	}

	// Called when |cefQuery| was called in JavaScript
	virtual bool OnQuery(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		int64 query_id,
		const CefString& request,
		bool persistent,
		CefRefPtr<Callback> callback) OVERRIDE;

private:

	// Members
	std::string _prefix;
	std::function<void(std::string)> _callbackFunction;
};

// Router that owns multiple handlers for messages
class MessageRouter : public CefBaseRefCounted
{
public:

	// Constructor
	MessageRouter(Mediator* pMediator);

	// Add new handler to process this callback
	void RegisterJavascriptCallback(std::string prefix, std::function<void(std::string)> callbackFunction)
	{
		CefMessageRouterBrowserSide::Handler* callbackHandler = new CallbackMsgHandler(prefix, callbackFunction);
		_router->AddHandler(callbackHandler, false);
	}

	// Redirecting onBeforeBrowse
	void OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) const
	{
		_router->OnBeforeBrowse(browser, frame);
	}

	// Redirecting OnBeforeClose
	void OnBeforeClose(CefRefPtr<CefBrowser> browser) const
	{ 
		_router->OnBeforeClose(browser); 
	}

	// Redirecting OnProcessMessageReceived
	bool OnProcessMessageReceived(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefProcessId source_process,
		CefRefPtr<CefProcessMessage> message) const
	{
		return _router->OnProcessMessageReceived(browser, frame, source_process, message);
	}
	// Redirect OnRenderProcessTerminated
	void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser) const
	{
		_router->OnRenderProcessTerminated(browser);
	}

private:

	// Wrapping CEF browser side message router
	CefRefPtr<CefMessageRouterBrowserSide> _router; // holds multiple handlers

	// Pointer to mediator. TODO: replace with some interface with less power and more control?
	Mediator* _pMediator;

	IMPLEMENT_REFCOUNTING(MessageRouter);
};

#endif // CEF_MESSAGEROUTER_H_