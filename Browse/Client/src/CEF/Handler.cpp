//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/CEF/Handler.h"
#include "src/CEF/Mediator.h"
#include "src/CEF/RequestHandler.h"
#include "src/Utils/Logger.h"
#include "src/Singletons/JSMailer.h"
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include <sstream>
#include <string>
#include <cmath>

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

void Handler::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
{
	// Set loading status of each in Tab to loading in order to display loading icon and status
	_pMediator->SetLoadingStatus(browser, frame->GetIdentifier(), frame->IsMain(), true);

	// DEBUG: golem.de
	//LogDebug("OnLoadStart: frameId=", frame->GetIdentifier(), ", isMain?=", frame->IsMain());
	//frame->ExecuteJavaScript("document.onreadystatechange = function(){console.log('" + std::to_string(frame->GetIdentifier())
	//	+ ": '+document.readyState)}", "", 0);

    if (frame->IsMain())
    {
		//LogDebug("Handler: Started loading frame id = ", frame->GetIdentifier(), " (main = ", frame->IsMain(), "), browserID = ", browser->GetIdentifier());

		frame->ExecuteJavaScript("StartPageLoadingTimer();", "", 0);

        // Set Tab's URL when page is loading
        _pMediator->SetURL(browser);

        // Set zoom level according to Tab's settings
        SetZoomLevel(browser, false);

        // Inject Javascript to hide scrollbar
        frame->ExecuteJavaScript(_js_remove_css_scrollbar, frame->GetURL(), 0);

    }

}

void Handler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
	// Set loading status of each frame in Tab to load finished in order to display loading icon and status
	_pMediator->SetLoadingStatus(browser, frame->GetIdentifier(), frame->IsMain(), false);

	// DEBUG: golem.de
	//LogDebug("OnLoadEnd: frameId=", frame->GetIdentifier(), ", isMain?=", frame->IsMain());

    if (frame->IsMain())
    {
		//LogDebug("Handler: End of loading frame id = ", frame->GetIdentifier(), " (main = ", frame->IsMain(), ").");
		frame->ExecuteJavaScript("StopPageLoadingTimer();", "", 0);

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

    // TODO: There might be a more performant way to set those values not as regularly as with every callback
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

	if (msgName == "SubmitInput")
	{
		LogDebug("Handler: Emulating a centered click and 'Enter' being pressed, in order to submit input.");

		// Emulate left mouse click on input fields center
		bool submit = msg->GetArgumentList()->GetBool(0);
		double x = msg->GetArgumentList()->GetDouble(1);
		double y = msg->GetArgumentList()->GetDouble(2);
		EmulateLeftMouseButtonClick(browser, x, y);

		if (submit) 
		{
			// Emulate pressing 'Enter' down, pressed and up
			CefKeyEvent event;
			event.is_system_key = false;
			event.modifiers = 0;

			// Enter key. Everywhere
			event.windows_key_code = 13;
			event.native_key_code = 13;
			event.character = event.unmodified_character = 13;

			// Down
			event.type = KEYEVENT_RAWKEYDOWN;
			browser->GetHost()->SendKeyEvent(event);

			// Character
			event.type = KEYEVENT_CHAR;
			browser->GetHost()->SendKeyEvent(event);

			// Up
			event.type = KEYEVENT_KEYUP;
			browser->GetHost()->SendKeyEvent(event);
		}

	}

    if (msgName == "ReceiveDOMElements")
    {
        LogDebug("Handler: OLD APPROACH! DELETE! ... ReceiveDOMElements msg received -> redirect to CefMediator.");

        //_pMediator->ClearDOMNodes(browser);

        // Pipe IPC message to CefMediator in order to create DOM nodes
       /* _pMediator->ReceiveIPCMessageforDOM(browser, msg);*/
        return true;
    }
    if (msgName == "ReceiveFavIconBytes")
    {
        _pMediator->ReceiveIPCMessageforFavIcon(browser, msg);
    }
    if (msgName == "ReceivePageResolution")
    {
        _pMediator->ReceivePageResolution(browser, msg);
    }
    if (msgName == "ReceiveFixedElements")
    {
        _pMediator->ReceiveFixedElements(browser, msg);
    }
    if (msgName == "IPCLog")
    {
        IPCLogRenderer(browser, msg);
    }

	if (msgName == "OnContextCreated")
	{
		_pMediator->ClearDOMNodes(browser);
	}
	if (msgName == "SendDOMNodeData")
	{
		//_pMediator->HandleDOMNodeIPCMsg(browser, msg);
		LogDebug("Handler: Received old IPC msg 'SendDOMNodeData'!");
	}

	if (msgName.substr(0, 9) == "CreateDOM")
	{
		_pMediator->FillDOMNodeWithData(browser, msg);
	}

    return _msgRouter->OnProcessMessageReceived(browser, source_process, msg);
}

bool Handler::OnJSDialog(
	CefRefPtr<CefBrowser> browser,
	const CefString& origin_url,
	const CefString& accept_lang,
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

		bit->get()->GetMainFrame()->ExecuteJavaScript("UpdateDOMRects();", "", 0);

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

bool Handler::InputTextData(CefRefPtr<CefBrowser> browser, int64 frameID, int nodeID, std::string text, bool submit)
{
    //CEF_REQUIRE_UI_THREAD();

	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("ExecuteTextInput");
	const auto& args = msg->GetArgumentList();
	args->SetInt(0, nodeID);
	args->SetString(1, text);
	args->SetBool(2, submit);

	browser->SendProcessMessage(PID_RENDERER, msg);

	return true;
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
    const std::string resetScrolling = "document.body.scrollTop=0; document.body.scrollLeft=0;";
    browser->GetMainFrame()->ExecuteJavaScript(resetScrolling, browser->GetMainFrame()->GetURL(), 0);
}

void Handler::SetZoomLevel(CefRefPtr<CefBrowser> browser, bool definitelyChanged)
{

    if (double zoomLevel = _pMediator->GetZoomLevel(browser))
    {
        LogDebug("Handler: Setting zoom level = ", zoomLevel, " (browserID = ", browser->GetIdentifier(), ").");
		const std::string setZoomLevel = "if(document.body !== null && document.body !== undefined) document.body.style.zoom=" + std::to_string(zoomLevel) + ";this.blur(); UpdateDOMRects();";
        browser->GetMainFrame()->ExecuteJavaScript(setZoomLevel, "", 0); 

        if (definitelyChanged)
        {
            UpdatePageResolution(browser);				// TODO: Does a JS event exist for this?

        }
    }
}

void Handler::UpdatePageResolution(CefRefPtr<CefBrowser> browser)
{
	// TODO: Return these values by calling a function (in Renderer Process), get rid of these window variables

    // Javascript code for receiving the current page width & height
	const std::string getPageResolution = "\
            if(document.documentElement && document.body !== null && document.body !== undefined)\
			{\
			window._pageWidth = document.body.scrollWidth;\
            window._pageHeight = document.body.scrollHeight;\
			}\
			else\
				console.log('Handler::UpdatePageResolution: Could not access document.documentElement or document.body!');\
            ";

    browser->GetMainFrame()->ExecuteJavaScript(getPageResolution, browser->GetMainFrame()->GetURL(), 0);

    // Tell renderer process to get V8 values, where page sizes were written to
    CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("GetPageResolution");
    browser->SendProcessMessage(PID_RENDERER, msg);
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
    std::string highResURL;
    int currentRes = 0;
    std::string icoURL;

    for (int i = 0; i < (int)icon_urls.size(); i++)
    {
        std::string url = icon_urls[i].ToString();
        std::string format = "", res = "";

        // Read out file ending
        for (int j = url.size() - 1; j >= (int)url.size() - 7; j--)
        {
            if (url[j] == '.')
            {
                break;
            }
            format = url[j] + format;
        }

        if (format == "ico")
        {
            icoURL = url;
            continue;
        }

        // Try to read parts of resolution like "160x160" from URL's end
        for (int j = url.size() - format.size() - 7; j < (int)(url.size() - format.size()) - 1; j++)
        {
            res += url[j];
        }

        int height = 0;
        for (int j = res.size()-1; j >= 0; j--)
        {
            // char is a digit?
            if (digits.find(res[j]) != digits.end())
            {
                height += ((res[j]-'0') * pow(10, res.size()-1-j));
            }
            else
            {
                // break, when 'x' is reached
                break;
            }
        }

        if (height > currentRes)
        {
            highResURL = url;
            currentRes = height;
        }
    }

    const std::string iconURL = (currentRes > 0) ? highResURL : icoURL;

    // Trigger favIconImg.onload function by setting image src
    const std::string jscode = "favIconImg.src = '" + iconURL + "';";
    browser->GetMainFrame()->ExecuteJavaScript(jscode, "", 0); 

    // New image incoming, delete the last one
    _pMediator->ResetFavicon(browser);
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

	_pMediator->OpenPopupTab(browser, target_url);

	return true;
}

void Handler::ScrollOverflowElement(CefRefPtr<CefBrowser> browser, int elemId, int x, int y, std::vector<int> fixedIds)
{
	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("ScrollOverflowElement");
	const auto& args = msg->GetArgumentList();
	args->SetInt(0, elemId);
	args->SetInt(1, x);
	args->SetInt(2, y);

	int count = 3;
	for (const auto& id : fixedIds)
	{
		args->SetInt(count++, id);
	}

	browser->SendProcessMessage(PID_RENDERER, msg);
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
