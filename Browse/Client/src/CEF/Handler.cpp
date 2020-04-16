//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/CEF/Handler.h"
#include "src/CEF/Mediator.h"
#include "src/Utils/Logger.h"
#include "src/Singletons/JSMailer.h"
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include <sstream>
#include <string>
#include <cmath>
#include "src/CEF/Data/DOMNode.h"
// For keyboard emulation
#include "submodules/glfw/include/GLFW/glfw3.h"
#include "submodules/eyeGUI/include/eyeGUI.h"

Handler::Handler(Mediator* pMediator, CefRefPtr<Renderer> renderer) : _isClosing(false)
{
  _pMediator = pMediator;
  _renderer = renderer;
  _msgRouter = new MessageRouter(pMediator);
  _requestHandler= new RequestHandler();

  // TODO: delete all this or do a nice rewrite
  // Tell JSMailer singleton about the method to call
  JSMailer::instance().SetHandler(this);
}

Handler::~Handler()
{
	// Nothing to do
}

void Handler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  // Add to the list of existing browsers.
  _browserList.push_back(browser);
  LogDebug("Handler: New CefBrowser with id = ", browser->GetIdentifier(), " created.");
}

bool Handler::DoClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  // Remove browser from browser list
  BrowserList::const_iterator it = _browserList.begin();
  int browserID = browser->GetIdentifier();
  for (; it != _browserList.end(); ++it)
  {
      if (it->get()->GetIdentifier() == browserID)
      {
          _browserList.remove(browser);
          break;
      }
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void Handler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  _msgRouter->OnBeforeClose(browser);

   //Remove from the list of existing browsers.
  BrowserList::iterator bit = _browserList.begin();
  for (; bit != _browserList.end(); ++bit)
  {
    if ((*bit)->IsSame(browser))
    {
        _browserList.erase(bit);
        break;
    }
  }

  //if (_browserList.empty())
  //{
  //  // All browser windows have closed. Quit the application's message loop
  //  LogDebug << "Quitting CEF message loop after all browser have shut down.";
  //  CefQuitMessageLoop();

  //}
}

void Handler::OnLoadError(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    ErrorCode errorCode,
    const CefString& errorText,
    const CefString& failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
        "<h2>Failed to load URL " << std::string(failedUrl) <<
        " with error " << std::string(errorText) << " (" << errorCode <<
        ").</h2></body></html>";
    frame->LoadString(ss.str(), failedUrl);
}

void Handler::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type)
{
	// Set loading status of each in Tab to loading in order to display loading icon and status
	_pMediator->SetLoadingStatus(browser, true, frame->IsMain());

    if (frame->IsMain())
    {
		_pMediator->ResetFavicon(browser);
		_pMediator->ClearDOMNodes(browser);
		//LogDebug("Handler: Started loading frame id = ", frame->GetIdentifier(), " (main = ", frame->IsMain(), "), browserID = ", browser->GetIdentifier());

        // Set zoom level according to Tab's settings
        SetZoomLevel(browser, false);

        // Inject Javascript to hide scrollbar
        frame->ExecuteJavaScript(_js_remove_css_scrollbar, frame->GetURL(), 0);

    }
	else
	{
		// EXPERIMENTAL
		//frame->ExecuteJavaScript(
		//	"window.addEventListener('message', (event) => {"
		//	"console.log('Window object',window,'received message from',event.source);"
		//	"event.source.postMessage('Answering event source...', event);"
		//	"});", "", 0);
	}

}

void Handler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
	// Set loading status of each frame in Tab to load finished in order to display loading icon and status
	_pMediator->SetLoadingStatus(browser, false, frame->IsMain());

    if (frame->IsMain())
    {
		// Calculate page loading time via Javascript
		frame->ExecuteJavaScript(
			"PrintPerformanceInformation();", "", 0);

        // Set zoom level according to Tab's settings
        SetZoomLevel(browser, false);

		// Inject Javascript to hide scrollbar
		frame->ExecuteJavaScript(_js_remove_css_scrollbar, frame->GetURL(), 0);
    }

}

void Handler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
    bool isLoading,
    bool canGoBack,
    bool canGoForward)
{
	// Fetch main frame's url and set it in Tab, if it changed
	_pMediator->SetURL(browser);

    _pMediator->SetCanGoBack(browser, canGoBack);
    _pMediator->SetCanGoForward(browser, canGoForward);

    // Finished loading whole page
    if (!isLoading)
    {
        // Set zoom level again at load end, in case it was written over again
        SetZoomLevel(browser, false);

        // Write page resolution to V8 variables, read them and update Tab
        UpdatePageResolution(browser);

    }
}

void Handler::CloseAllBrowsers(bool forceClose)
{
   LogDebug("Handler: Closing all browsers.");

    if (!CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        CefPostTask(TID_UI,
            base::Bind(&Handler::CloseAllBrowsers, this, forceClose));
        return;
    }

    if (_browserList.empty())
        return;

    BrowserList::const_iterator it = _browserList.begin();
    for (; it != _browserList.end(); ++it)
    {
        CloseBrowser(it->get());
    }

}

void Handler::CloseBrowser(CefRefPtr<CefBrowser> browser)
{
    int browserID = browser->GetIdentifier();

    browser->StopLoad();

    browser->GetHost()->CloseBrowser(false);

    LogDebug("Handler: Browser successfully closed (id = ", browserID, ").");
}

bool Handler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> msg)
{
    const std::string& msgName = msg->GetName().ToString();

	if (msgName == "EmulateSelectAll")
	{
		EmulateSelectAll(browser);
		return true;
	}
	if (msgName == "EmulateEnterKey")
	{
		if (msg->GetArgumentList()->GetSize() > 1)
		{
			// Emulate left mouse click where 'Enter' should be pressed
			double x = msg->GetArgumentList()->GetDouble(0);
			double y = msg->GetArgumentList()->GetDouble(1);

			LogDebug("Handler: Emulating a click (", x, ",", y, ") and 'Enter' being pressed.");

			EmulateLeftMouseButtonClick(browser, x, y);
		}
		EmulateEnterKey(browser);
		return true;
	}
	if (msgName == "EmulateKeyboardStrokes")
	{
		const auto input = msg->GetArgumentList()->GetString(0).ToString();
		EmulateKeyboardStrokes(browser, input);
		return true;
	}
	if (msgName == "EmulateMouseClick")
	{
		double x = msg->GetArgumentList()->GetDouble(0);
		double y = msg->GetArgumentList()->GetDouble(1);

		LogDebug("Handler: Emulating a click (", x, ",", y, ").");

		EmulateLeftMouseButtonClick(browser, x, y);
		return true;
	}

    if (msgName == "ReceiveFavIconBytes")
 		LogDebug("Handler: Received depricated ", msgName, " IPC message!");
 

    if (msgName == "ReceiveFixedElements")
    {
        _pMediator->ReceiveFixedElements(browser, msg);
		return true;
    }
    if (msgName == "IPCLog")
    {
        IPCLogRenderer(browser, msg);
		return true;
    }

	if (msgName == "OnContextCreated")
	{
		// _pMediator->ClearDOMNodes(browser);
		return true;
	}

	// NEW DOMNODE STRUCTURE, TODO: Might be (partially) moved to DOMExtraction?
	if (msgName.substr(0, 12) == "ExtractedDOM")
	{
		
		const std::string type = msgName.substr(12, msgName.size() - 16); // NOTE: -4 + (-12) -> remove "Data" at the end
		const int& id = msg->GetArgumentList()->GetInt(0);

		if (type == "TextInput")
		{
			const auto& wpNode = _pMediator->GetDOMTextInput(browser, id);
			if (const auto& node = wpNode.lock())
			{
				node->Initialize(msg);
			}
			return true;
		}
		if (type == "Link")
		{
			const auto& wpNode = _pMediator->GetDOMLink(browser, id);
			if (const auto& node = wpNode.lock())
			{
				node->Initialize(msg);
			}
			return true;
		}
		if (type == "SelectField")
		{
			const auto& wpNode = _pMediator->GetDOMSelectField(browser, id);
			if (const auto& node = wpNode.lock())
			{
				node->Initialize(msg);
			}
			return true;
		}
		if (type == "OverflowElement")
		{
			const auto& wpNode = _pMediator->GetDOMOverflowElement(browser, id);
			if (const auto& node = wpNode.lock())
			{
				node->Initialize(msg);
			}
			return true;
		}
		// TODO: Refactor this.
		if (type == "Checkbox")
		{
			const auto& wpNode = _pMediator->GetDOMCheckbox(browser, id);
			if (const auto& node = wpNode.lock())
			{
				node->Initialize(msg);
			}
			return true;
		}
	}

    return _msgRouter->OnProcessMessageReceived(browser, source_process, msg);
}

bool Handler::OnJSDialog(
	CefRefPtr<CefBrowser> browser,
	const CefString& origin_url,
	JSDialogType dialog_type,
	const CefString& message_text,
	const CefString& default_prompt_text,
	CefRefPtr<CefJSDialogCallback> callback,
	bool& suppress_message)
{
	/*
	// HOW TO ANSWER DIALOG CALLBACK
	bool clicked_ok;
	std::string users_answer;
	callback->Continue(clicked_ok, users_answer);

	//return true;
	*/

	// Remember that callback
	_jsDialogCallbacks[browser->GetIdentifier()] = callback;

	// Decide type of dialog
	JavaScriptDialogType type = JavaScriptDialogType::ALERT;
	if (dialog_type == JSDialogType::JSDIALOGTYPE_CONFIRM)
	{
		type = JavaScriptDialogType::CONFIRM;
	}
	else if (dialog_type == JSDialogType::JSDIALOGTYPE_PROMPT)
	{
		type = JavaScriptDialogType::PROMPT;
	}

	// Tell Tab about it so it can react and execute callback later
	_pMediator->RequestJSDialog(browser, type, message_text);

	// Dialog handled!
	return true;
}

bool Handler::OnBeforeUnloadDialog(
	CefRefPtr<CefBrowser> browser,
	const CefString& message_text,
	bool is_reload,
	CefRefPtr<CefJSDialogCallback> callback)
{
	// Remember that callback
	_jsDialogCallbacks[browser->GetIdentifier()] = callback;

	// Tell Tab about it so it can react and execute callback later
	_pMediator->RequestJSDialog(browser, JavaScriptDialogType::LEAVE_PAGE, message_text);

	// Dialog handled!
	return true;
}

void Handler::ResizeBrowsers()
{
    CEF_REQUIRE_UI_THREAD();

    BrowserList::iterator bit = _browserList.begin();
    for (; bit != _browserList.end(); ++bit)
    {
        LogDebug("Handler: Browser (id = ", bit->get()->GetIdentifier(), " was resized.");

        bit->get()->GetHost()->WasResized();

        // Resize may cause change in page size
        UpdatePageResolution(bit->get());

		// TODO / NOTE: 
		// It might be better to perform that Rect Update not simultaneously on every browser.
		// Instead save it and execute it when you switch to the target tab

		bit->get()->GetMainFrame()->ExecuteJavaScript("console.log('Browser resized, updating DOM Rects!');" \
			"UpdateDOMRects();", "", 0);

        // EXPERIMENTAL
        //GetFixedElements(bit->get());

    }
}

void Handler::LoadPage(CefRefPtr<CefBrowser> browser, std::string url)
{
    LogDebug("Handler: Loading page ", url);
    browser->GetMainFrame()->LoadURL(url);
}

void Handler::EmulateMouseCursor(CefRefPtr<CefBrowser> browser, double x, double y, bool leftButtonPressed)
{
    CefMouseEvent event;
    event.x = x;
    event.y = y;
	if (leftButtonPressed) { event.modifiers = EVENTFLAG_LEFT_MOUSE_BUTTON; }

    browser->GetHost()->SendMouseMoveEvent(event, false);
}

void Handler::EmulateLeftMouseButtonClick(CefRefPtr<CefBrowser> browser, double x, double y)
{
    CefMouseEvent event;
    event.x = x;
    event.y = y;
    LogDebug("Handler: Emulating left mouse button click on position (", x, ", ", y, "), browserID = ", browser->GetIdentifier());
    browser->GetHost()->SendMouseClickEvent(event, MBT_LEFT, false, 1);	// press
    browser->GetHost()->SendMouseClickEvent(event, MBT_LEFT, true, 1);	// release

}

void Handler::EmulateMouseWheelScrolling(CefRefPtr<CefBrowser> browser, double deltaX, double deltaY)
{
    CefMouseEvent event;
    event.x = 0;	// TODO: Mouse position? Possible case: Scroll inside of a sub area inside the page
    event.y = 0;
    //DLOG(INFO) << "Emulating mouse wheel, browserID=" << browser->GetIdentifier();
    browser->GetHost()->SendMouseWheelEvent(event, deltaX, deltaY);
}

void Handler::Reload(CefRefPtr<CefBrowser> browser)
{
    LogDebug("Handler: Reloading browser (id = ", browser->GetIdentifier(), ") on page ", browser->GetMainFrame()->GetURL().ToString());
    browser->Reload();
}

void Handler::GoBack(CefRefPtr<CefBrowser> browser)
{
    browser->GoBack();
}

void Handler::GoForward(CefRefPtr<CefBrowser> browser)
{
    browser->GoForward();
}

void Handler::ReplyJSDialog(CefRefPtr<CefBrowser> browser, bool clicked_ok, std::string user_input)
{
	auto callback = _jsDialogCallbacks.find(browser->GetIdentifier());

	if (callback != _jsDialogCallbacks.end())
	{
		LogInfo(clicked_ok);
		callback->second->Continue(clicked_ok, user_input);
	}
}

void Handler::ResetMainFramesScrolling(CefRefPtr<CefBrowser> browser)
{
    const std::string resetScrolling = "window.scrollTo(0, 0);";
    browser->GetMainFrame()->ExecuteJavaScript(resetScrolling, browser->GetMainFrame()->GetURL(), 0);
}

void Handler::SetZoomLevel(CefRefPtr<CefBrowser> browser, bool definitelyChanged)
{

    if (double zoomLevel = _pMediator->GetZoomLevel(browser))
    {
        LogDebug("Handler: Setting zoom level = ", zoomLevel, " (browserID = ", browser->GetIdentifier(), ").");
		const std::string setZoomLevel = "if(document.body !== null && document.body !== undefined) document.body.style.zoom="\
			+ std::to_string(zoomLevel) + ";this.blur(); "; // TODO(Refactoring): console.log('Setting zoom level, updating DOMRects!'); UpdateDOMRects(); ";
        browser->GetMainFrame()->ExecuteJavaScript(setZoomLevel, "", 0); 

        if (definitelyChanged)
        {
            UpdatePageResolution(browser);				// TODO: Does a JS event exist for this?

        }
    }
}

void Handler::UpdatePageResolution(CefRefPtr<CefBrowser> browser)
{
	// Execute Javascript function which will send page resolution to MessageRouter
	browser->GetMainFrame()->ExecuteJavaScript(
		"if(typeof(CefGetPageResolution) === 'function'){ CefGetPageResolution(); }", browser->GetMainFrame()->GetURL(), 0);
}

void Handler::IPCLogRenderer(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg)
{
    CefRefPtr<CefListValue> args = msg->GetArgumentList();
    const std::string text = args->GetString(1);
    const bool debugLog = args->GetBool(0);
    const int browserID = browser->GetIdentifier();

    if (debugLog)
    {
        LogDebug("Renderer: ", text, " (browserID = ", browserID, ")");
    }
	else
	{
		LogInfo("Renderer: ", text, " (browserID = ", browserID, ")");
	}
}

void Handler::OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
    const std::vector<CefString>& icon_urls)
{
	for (auto url : icon_urls)
		StartFaviconImageDownload(browser, url);
}

void Handler::OnTitleChange(CefRefPtr<CefBrowser> browser,
	const CefString& title)
{
	_pMediator->OnTabTitleChange(browser, title.ToString());
}

bool Handler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	const CefString& target_url,
	const CefString& target_frame_name,
	WindowOpenDisposition target_disposition,
	bool user_gesture,
	const CefPopupFeatures& popupFeatures,
	CefWindowInfo& windowInfo,
	CefRefPtr<CefClient>& client,
	CefBrowserSettings& settings,
	bool* no_javascript_access)
{
	LogInfo("Handler: Suppressed popup from opening a new window");
	
	_pMediator->OpenPopupTab(browser, target_url, !(*no_javascript_access));

	return true;
}

void Handler::SendToJSLoggingMediator(std::string message)
{
	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("SendToLoggingMediator");
	msg->GetArgumentList()->SetString(0, message);

	// Send message to every browser
	for (const auto& browser : _browserList)
	{
		browser->SendProcessMessage(PID_RENDERER, msg);
	}
	
}

bool Handler::ForwardFaviconBytes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefImage> img)
{
	return _pMediator->ForwardFaviconBytes(browser, img);
}

bool Handler::StartFaviconImageDownload(CefRefPtr<CefBrowser> browser, CefString img_url)
{
	if (!_pMediator->IsFaviconAlreadyAvailable(browser, img_url))
		return false;
	StartImageDownload(browser, img_url);
	return true;
}

void Handler::EmulateKeyboardKey(CefRefPtr<CefBrowser> browser, int key, int scancode, int mods, bool send_char_keyevent)
{
	// Emulate pressing 'Enter' down, pressed and up
	CefKeyEvent event;
	event.is_system_key = false;
	event.modifiers = mods;

	event.windows_key_code = key;
	event.native_key_code = scancode;
	event.character = event.unmodified_character = key;

	// Down
	event.type = KEYEVENT_RAWKEYDOWN;
	browser->GetHost()->SendKeyEvent(event);

	if (send_char_keyevent)
	{
		// Character
		event.type = KEYEVENT_CHAR;
		browser->GetHost()->SendKeyEvent(event);
	}

	// Up
	event.type = KEYEVENT_KEYUP;
	browser->GetHost()->SendKeyEvent(event);
}

void Handler::EmulateKeyboardStrokes(CefRefPtr<CefBrowser> browser, std::string input)
{
	// Convert UTF8 to UTF16 (right now over eyeGUI functions, should be available through Chromium)
	std::u16string input16;
	eyegui_helper::convertUTF8ToUTF16(input, input16);

	for (int i = 0; i < input16.length(); i++)
	{
		char16 key = input16.at(i);
		uint32 mods = 0;
		if (key == u'\n')
		{
			key = 13;
			mods = EVENTFLAG_SHIFT_DOWN;
		}

		CefKeyEvent event;
		event.character = key;
		
		event.is_system_key = false;
		event.modifiers = mods;

		event.windows_key_code = key;
		//event.native_key_code = scancode;
		event.unmodified_character = key;
		event.focus_on_editable_field = true;

		// Character
		event.type = KEYEVENT_CHAR;
		browser->GetHost()->SendKeyEvent(event);
	}
}

void Handler::EmulateSelectAll(CefRefPtr<CefBrowser> browser)
{
	EmulateKeyboardKey(browser, 'A', 'A', EVENTFLAG_CONTROL_DOWN, false);
}

void Handler::EmulateEnterKey(CefRefPtr<CefBrowser> browser)
{
	EmulateKeyboardKey(browser, 13, 13, 0);
}

bool Handler::OnKeyEvent(CefRefPtr<CefBrowser> browser,
	const CefKeyEvent& event,
	CefEventHandle os_event)
{
	LogInfo("key character: ", event.character);
	
	return true;
}