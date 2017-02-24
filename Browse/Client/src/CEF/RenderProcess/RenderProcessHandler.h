//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// This class is delegated by RenderCefApp to handle the Render Process.
// Handles the rendering of one tab aka CefBrowser. Has access to its V8
// context.

#ifndef CEF_RENDERPROCESSHANDLER_H_
#define CEF_RENDERPROCESSHANDLER_H_

#include "src/CEF/JSCode.h"
#include "include/wrapper/cef_message_router.h"
#include "include/cef_render_process_handler.h"

class RenderProcessHandler : public CefRenderProcessHandler {
public:

	// Constructor
	RenderProcessHandler();

    // Callback, called when IPC message from e.g. main process (see Handler) is received
    bool OnProcessMessageReceived(
        CefRefPtr<CefBrowser> browser,
        CefProcessId sourceProcess,
        CefRefPtr<CefProcessMessage> msg) OVERRIDE;

    // Callback, called when DOM node under cursor changes
    virtual void OnFocusedNodeChanged(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefDOMNode> node) OVERRIDE;

    // Callback, called when a browser's V8 context is created
    void OnContextCreated(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) OVERRIDE;

    // Callback, called when a browser's V8 context is released
    void OnContextReleased(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) OVERRIDE;

	// Fetching DOM node data from V8 context
	CefRefPtr<CefV8Value> FetchDOMObject(CefRefPtr<CefV8Context> context, int nodeType, int nodeID);
	CefRefPtr<CefProcessMessage> UnwrapDOMTextInput(CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Value> domObj, int nodeID);
	CefRefPtr<CefProcessMessage> UnwrapDOMLink(CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Value> domObj, int nodeID);

private:

    // Send logs to main process
    void IPCLog(CefRefPtr<CefBrowser> browser, std::string text, bool debugLog = false);
	void IPCLogDebug(CefRefPtr<CefBrowser> browser, std::string text) { IPCLog(browser, text, true); };

    // Message router instance
	CefRefPtr<CefMessageRouterRendererSide> _msgRouter;

    // JavaScript code as Strings
	const std::string _js_dom_update_sizes = GetJSCode(DOM_UPDATE_SIZES);
	const std::string _js_dom_fill_arrays = GetJSCode(DOM_FILL_ARRAYS);
	const std::string _js_favicon_create_img = GetJSCode(FAVICON_CREATE_IMG);
	const std::string _js_favicon_copy_img_bytes_to_v8array = GetJSCode(FAVICON_COPY_IMG_BYTES_TO_V8ARRAY);
	const std::string _js_mutation_observer_test = GetJSCode(MUTATION_OBSERVER_TEST);
	const std::string _js_dom_mutationobserver = GetJSCode(DOM_MUTATIONOBSERVER);
	std::string _js_dom_fixed_elements = GetJSCode(DOM_FIXED_ELEMENTS);

    // Include CEF'S default reference counting implementation
    IMPLEMENT_REFCOUNTING(RenderProcessHandler);
};

#endif // CEF_RENDERPROCESSHANDLER_H_
