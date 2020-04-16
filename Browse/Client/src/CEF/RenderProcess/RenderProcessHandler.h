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
		CefRefPtr<CefFrame> frame,
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

	// Helper for automatic transformation between IPC messages and V8 values
    CefRefPtr<CefV8Value> CefValueToCefV8Value(CefRefPtr<CefValue> val);

private:

    // Send logs to main process
    void IPCLog(CefRefPtr<CefBrowser> browser, std::string text, bool debugLog = false);
    void IPCLogDebug(CefRefPtr<CefBrowser> browser, std::string text) { IPCLog(browser, text, true); }

    // Message router instance
	CefRefPtr<CefMessageRouterRendererSide> _msgRouter;

    // JavaScript code as Strings
	const std::string _js_dom_nodes				= GetJSCode(DOM_NODES);
	const std::string _js_dom_nodes_interaction	= GetJSCode(DOM_NODES_INTERACTION);
	const std::string _js_dom_mutationobserver	= GetJSCode(DOM_MUTATIONOBSERVER);
	const std::string _js_dom_fixed_elements	= GetJSCode(DOM_FIXED_ELEMENTS);
	const std::string _js_helpers				= GetJSCode(HELPERS);
	const std::string _js_dom_attributes		= GetJSCode(DOM_ATTRIBUTES);
	const std::string _js_dom_nodes_helpers		= GetJSCode(DOM_NODES_HELPERS);

	std::vector<std::pair<std::string, std::string>> _js_dom_code = {
		std::make_pair(_js_helpers, "helpers.js"),
		std::make_pair(_js_dom_nodes, "dom_nodes.js"),
		std::make_pair(_js_dom_nodes_helpers, "dom_nodes_helper.js"),
		std::make_pair(_js_dom_nodes_interaction, "dom_nodes_interaction.js"),
		std::make_pair(_js_dom_fixed_elements, "dom_fixed_elements.js"),
		std::make_pair(_js_dom_mutationobserver, "dom_mutationobserver.js"),
		std::make_pair(_js_dom_attributes, "dom_attributes.js")

	};

    // Include CEF'S default reference counting implementation
    IMPLEMENT_REFCOUNTING(RenderProcessHandler);
};

#endif // CEF_RENDERPROCESSHANDLER_H_
