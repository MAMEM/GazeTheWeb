//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef CEF_HANDLER_H_
#define CEF_HANDLER_H_

#include "src/CEF/Renderer.h"
#include "src/CEF/JSCode.h"
#include "include/cef_client.h"
#include "src/CEF/MessageRouter.h"
#include "src/CEF/ImageDownload.h"
#include "src/CEF/RequestHandler.h"
#include <list>
#include <set>

// Forward declaration
class Mediator;
class BrowserMsgRouter;

class Handler : public CefClient,
                public CefDisplayHandler,
                public CefLifeSpanHandler,
                public CefLoadHandler,
				public CefJSDialogHandler,
				public HandlerImageInterface,
				public CefKeyboardHandler
{
public:

    Handler(Mediator* pMediator, CefRefPtr<Renderer> renderer);
    ~Handler();

	// TODO: Not called?
	bool OnKeyEvent(CefRefPtr<CefBrowser> browser,
		const CefKeyEvent& event,
		CefEventHandle os_event) OVERRIDE; 

    // Request that all existing browser windows close
    void CloseAllBrowsers(bool forceClose);
    bool IsClosing() const { return _isClosing; }

    // CefClient methods
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE { return this; }
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE { return this; }
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }
	virtual CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() OVERRIDE { return this; }
    virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE { return _renderer; }
	
	// Request handler
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE
	{
		return _requestHandler;
	}

    // Life span handling of CefBrowsers
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
	virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		const CefString& target_url,
		const CefString& target_frame_name,
		WindowOpenDisposition target_disposition,
		bool user_gesture,
		const CefPopupFeatures& popupFeatures,
		CefWindowInfo& windowInfo,
		CefRefPtr<CefClient>& client,
		CefBrowserSettings& settings,
		CefRefPtr<CefDictionaryValue>& extra_info,
		bool* no_javascript_access) OVERRIDE;

    // Loading state callbacks
    virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) OVERRIDE;
    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) OVERRIDE;
    virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
        bool isLoading,
        bool canGoBack,
        bool canGoForward) OVERRIDE;
    virtual void OnLoadError(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        ErrorCode errorCode,
        const CefString& errorText,
        const CefString& failedUrl) OVERRIDE;

    // Only called by CefMediator
    void CloseBrowser(CefRefPtr<CefBrowser> browser);

    // Called when message from another process is received (e.g. IPC response from renderer process)
	virtual bool OnProcessMessageReceived(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefProcessId source_process,
		CefRefPtr<CefProcessMessage> message) OVERRIDE;

	// Called when JavaScript dialog is received
	virtual bool OnJSDialog(
		CefRefPtr<CefBrowser> browser,
		const CefString& origin_url,
		JSDialogType dialog_type,
		const CefString& message_text,
		const CefString& default_prompt_text,
		CefRefPtr<CefJSDialogCallback> callback,
		bool& suppress_message) OVERRIDE;

	// Called when the user wants to leave a page
	virtual bool OnBeforeUnloadDialog(
		CefRefPtr<CefBrowser> browser,
		const CefString& message_text,
		bool is_reload,
		CefRefPtr<CefJSDialogCallback> callback) OVERRIDE;

    // Load URL in specific browser
    void LoadPage(CefRefPtr<CefBrowser> browser, std::string url);

    // Page navigation
    void Reload(CefRefPtr<CefBrowser> browser);
    void GoBack(CefRefPtr<CefBrowser> browser);
    void GoForward(CefRefPtr<CefBrowser> browser);

	// Reply JavaScript dialog callback
	void ReplyJSDialog(CefRefPtr<CefBrowser> browser, bool clicked_ok, std::string user_input);

    // Called by CefMediator, when window resize happens
    void ResizeBrowsers();

    // Emulation of mouse buttons in specific browser
    void EmulateMouseCursor(CefRefPtr<CefBrowser> browser, double x, double y, bool leftButtonPressed);
    void EmulateLeftMouseButtonClick(CefRefPtr<CefBrowser> browser, double x, double y);
    void EmulateMouseWheelScrolling(CefRefPtr<CefBrowser> browser, double deltaX, double deltaY, int x, int y);
	void EmulateKeyboardKey(CefRefPtr<CefBrowser> browser, int key, int scancode, int mods, bool send_char_keyevent=true);
	void EmulateKeyboardStrokes(CefRefPtr<CefBrowser> browser, std::string input);
	void EmulateEnterKey(CefRefPtr<CefBrowser> browser);
	void EmulateSelectAll(CefRefPtr<CefBrowser> browser);
    
    void ResetMainFramesScrolling(CefRefPtr<CefBrowser> browser);

    // Bool value indicates need to reload DOM node data, true when called from outside of Tab due to changes
    void SetZoomLevel(CefRefPtr<CefBrowser> browser, bool definitelyChanged = true);

    // Write page resolution to V8 variables, read them and update Tab
	void UpdatePageResolution(CefRefPtr<CefBrowser> browser);				// TODO: <--- Needs refactoring!

    // EXPERIMENTAL: Request coordinates of fixed elements (like bars on top of pages)
    void GetFixedElements(CefRefPtr<CefBrowser> browser);

	// CefDisplayHandler callbacks
	void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
		const std::vector<CefString>& icon_urls) OVERRIDE;
	void OnTitleChange(CefRefPtr<CefBrowser> browser,
		const CefString& title) OVERRIDE;

	void RegisterJavascriptCallback(std::string prefix, std::function<void(std::string)> callbackFunction)
	{
		_msgRouter->RegisterJavascriptCallback(prefix, callbackFunction);
	}

	// Send log data to LoggingMediator instance in each browser context
	void SendToJSLoggingMediator(std::string message);

	// Check if favicon was already loaded, if not download it
	bool StartFaviconImageDownload(CefRefPtr<CefBrowser> browser, CefString img_url);
	// HandlerImageDownload interface methods
	bool ForwardFaviconBytes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefImage> img);

	// Decide whether to block ads
	void BlockAds(bool blockAds) { _requestHandler->BlockAds(blockAds); }

private:

    /* METHODS */

    // Log messages from renderer process on receiving logging relevant IPC messages
    void IPCLogRenderer(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg);

    /* MEMBERS */

    // List of existing browser windows. Only accessed on the CEF UI thread
    typedef std::list<CefRefPtr<CefBrowser> > BrowserList;
    BrowserList _browserList;
    // Handler's current closing state
    bool _isClosing;

    // Provide pointer to CefMediator in order to communicate with Tabs etc.
    Mediator* _pMediator;

    // Renderer, whose methods are called when rendering relevant actions take place
    CefRefPtr<Renderer> _renderer;

	// Message router for JavaScript induced C++ callbacks
	CefRefPtr<MessageRouter> _msgRouter;

	// Used for adblocking
	CefRefPtr<RequestHandler> _requestHandler;

    // JavaScript code as Strings
    const std::string _js_remove_css_scrollbar = GetJSCode(REMOVE_CSS_SCROLLBAR);

	// Set for parsing strings (as char by accessing it with []) to numbers
	std::set<char> digits = { '0', '1', '2', '3', '4', '5', '6' ,'7', '8', '9' };

	// Map of browser identifier to JavaScript dialog callbacks that can be answered (may be never answered or to late TODO: problem?)
	std::map<int, CefRefPtr<CefJSDialogCallback> > _jsDialogCallbacks;

    // Include CEF'S default reference counting implementation
    IMPLEMENT_REFCOUNTING(Handler);
};

#endif  // CEF_HANDLER_H_
