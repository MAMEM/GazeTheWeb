//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "src/CEF/Mediator.h"
#include "src/CEF/Handler.h"
#include "src/CEF/DevToolsHandler.h"
#include "src/CEF/JSCode.h"
#include "src/Setup.h"
#include "src/State/Web/Tab/Interface/TabCEFInterface.h"
#include "src/CEF/Data/DOMNode.h"
#include "src/Utils/Logger.h"
#include "src/Global.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"


void Mediator::SetMaster(MasterNotificationInterface* pMaster)
{
	_pMaster = pMaster;
}

void Mediator::RegisterTab(TabCEFInterface* pTab, std::string URL, CefRefPtr<CefRequestContext> request_context)
{
    CEF_REQUIRE_UI_THREAD();

    /*Following information might be changed and used in the future
    http://magpcss.org/ceforum/apidocs3/projects/%28default%29/_cef_browser_settings_t.html
        - webgl: enable WebGL
        - windowless_frame_rate: default 30 fps, maximum 60
    */
    CefWindowInfo window_info;				// CefBrowser relevant information
    window_info.SetAsWindowless(0);			// Window handle set to zero (may cause visual errors)
    CefBrowserSettings browser_settings;	// Browser settings

    // Enable WebGL part 1 (other is in App.cpp)
    if(setup::ENABLE_WEBGL)
    {
        browser_settings.webgl = STATE_ENABLED;
        browser_settings.windowless_frame_rate = 60;
	
    }

	browser_settings.application_cache = STATE_ENABLED; // Goal: Accepting & saving cookies enabled

    // TODO: Delay Renderer OR save Tab & Texture as default if key(browser) not in map, reset after map insertion
    //			OR add maps to CreateBrowserSync and fill them before Renderer starts (if possible)

    _pendingTab = pTab;

	// Decide on url
	if (URL.empty()) { URL = BLANK_PAGE_URL; }

    LogDebug("Mediator: Creating new CefBrowser at Tab registration.");
    // Create new CefBrowser with given information
    CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(
        window_info, _handler.get(), URL, browser_settings, request_context);

	LogDebug("Mediator::RegisterTab: request_context == nullptr? ", request_context == nullptr);

    // Fill maps with correlating Tab and CefBrowsre
    _browsers.emplace(pTab, browser);
    _tabs.emplace(browser->GetIdentifier(), pTab);
    _pendingTab = NULL;
}

void Mediator::UnregisterTab(TabCEFInterface* pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        // Close corresponding CefBrowser in Handler
        _handler->CloseBrowser(browser);

        // Delete corresponding key-value-pair
        BrowserID browserID = browser->GetIdentifier();
        LogDebug("Mediator: Unregistering Tab corresponding to browserID = ", browserID);
        _tabs.erase(browserID);
        _browsers.erase(pTab);
    }
}

void Mediator::LoadURLInTab(TabCEFInterface * pTab, std::string URL)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        // Get Tab object and load Tab's URL in CefBrowser
        _handler->LoadPage(_browsers.at(pTab), URL);
    }
}

void Mediator::ReloadTab(TabCEFInterface * pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->Reload(browser);
    }
}

void Mediator::GoBack(TabCEFInterface * pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->GoBack(browser);
    }
}

void Mediator::GoForward(TabCEFInterface * pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->GoForward(browser);
    }
}

void Mediator::RequestJSDialog(CefRefPtr<CefBrowser> browser, JavaScriptDialogType type, std::string message)
{
	if (auto pTab = GetTab(browser))
	{
		pTab->RequestJSDialog(type, message);
	}
}

void Mediator::ReplyJSDialog(TabCEFInterface* pTab, bool clicked_ok, std::string user_input)
{
	if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		_handler->ReplyJSDialog(browser, clicked_ok, user_input);
	}
}

void Mediator::DoMessageLoopWork()
{
    CefDoMessageLoopWork();
}

void Mediator::EmulateMouseCursor(TabCEFInterface* pTab, double x, double y, bool leftButtonPressed)
{
    if(CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->EmulateMouseCursor(browser, x, y, leftButtonPressed);
    }
}

void Mediator::EmulateLeftMouseButtonClick(TabCEFInterface * pTab, double x, double y)
{
    if(CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        LogDebug("Mediator: Emulating left mouse button click on position (x, y) = (", x, ", ", y, ").");

        // Get Tab information and do browser work in Handler
        _handler->EmulateLeftMouseButtonClick(browser, x, y);
    }
}

void Mediator::EmulateMouseWheelScrolling(TabCEFInterface * pTab, double deltaX, double deltaY)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->EmulateMouseWheelScrolling(browser, deltaX, deltaY);
    }
}

void Mediator::ResetFavicon(CefRefPtr<CefBrowser> browser)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        pTab->ResetFaviconBytes();
    }
}

void Mediator::AddDOMTextInput(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->AddDOMTextInput(id);
	}
}

void Mediator::AddDOMLink(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->AddDOMLink(id);
	}
}

void Mediator::AddDOMSelectField(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->AddDOMSelectField(id);
	}
}

void Mediator::AddDOMOverflowElement(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->AddDOMOverflowElement(id);
	}
}

void Mediator::AddDOMVideo(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->AddDOMVideo(id);
	}
}

void Mediator::AddDOMCheckbox(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->AddDOMCheckbox(id);
	}
}


void Mediator::ClearDOMNodes(CefRefPtr<CefBrowser> browser)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        // Clear corresponding Tabs DOM Node list (implicitly destroy all DOM Node objects)
        pTab->ClearDOMNodes();
        LogDebug("Mediator: Clearing Tab's DOM nodes belonging to browserID = ", browser->GetIdentifier());
    }
}

void Mediator::RemoveDOMTextInput(CefRefPtr<CefBrowser> browser, int id)
{
	if (auto pTab = GetTab(browser))
	{
		pTab->RemoveDOMTextInput(id);
	}
}

void Mediator::RemoveDOMLink(CefRefPtr<CefBrowser> browser, int id)
{
	if (auto pTab = GetTab(browser))
	{
		pTab->RemoveDOMLink(id);
	}
}

void Mediator::RemoveDOMSelectField(CefRefPtr<CefBrowser> browser, int id)
{
	if (auto pTab = GetTab(browser))
	{
		pTab->RemoveDOMSelectField(id);
	}
}

void Mediator::RemoveDOMCheckbox(CefRefPtr<CefBrowser> browser, int id)
{
	if (auto pTab = GetTab(browser))
	{
		pTab->RemoveDOMCheckbox(id);
	}
}

bool Mediator::SendProcessMessageToRenderer(CefRefPtr<CefProcessMessage> msg, TabCEFInterface* pTab)
{
	if (auto browser = GetBrowser(pTab))
	{
		return browser->SendProcessMessage(PID_RENDERER, msg);
	}
	return false;
}

std::weak_ptr<DOMTextInput> Mediator::GetDOMTextInput(CefRefPtr<CefBrowser> browser, int id)
{
	if (auto pTab = GetTab(browser))
	{
		return pTab->GetDOMTextInput(id);
	}
	return std::weak_ptr<DOMTextInput>();
}

std::weak_ptr<DOMLink> Mediator::GetDOMLink(CefRefPtr<CefBrowser> browser, int id)
{
	if (auto pTab = GetTab(browser))
	{
		return pTab->GetDOMLink(id);
	}
	return std::weak_ptr<DOMLink>();
}

std::weak_ptr<DOMSelectField> Mediator::GetDOMSelectField(CefRefPtr<CefBrowser> browser, int id)
{
	if (auto pTab = GetTab(browser))
	{
		return pTab->GetDOMSelectField(id);
	}
	return std::weak_ptr<DOMSelectField>();
}

void Mediator::SetActiveTab(TabCEFInterface * pTab)
{
	// Remember currently active Tab
	_activeTab = pTab;

	for (const auto& key : _browsers)
	{
		CefRefPtr<CefBrowser> browser = key.second;
		if (key.first == pTab)
		{
			// Activate the given Tab's rendering
			browser->GetHost()->WasHidden(false);
		}
		else
		{
			// Deactivate rendering of all other Tabs
			browser->GetHost()->WasHidden(true);
		}
	}
}



bool Mediator::SetLoadingStatus(CefRefPtr<CefBrowser> browser, bool isLoading, bool isMainFrame)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->SetLoadingStatus(isLoading, isMainFrame);
		return true;
	}
	return false;	
}

std::weak_ptr<DOMOverflowElement> Mediator::GetDOMOverflowElement(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		return pTab->GetDOMOverflowElement(id);
	}
	return std::weak_ptr<DOMOverflowElement>();
}

std::weak_ptr<DOMVideo> Mediator::GetDOMVideo(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		return pTab->GetDOMVideo(id);
	}
	return std::weak_ptr<DOMVideo>();
}

std::weak_ptr<DOMCheckbox> Mediator::GetDOMCheckbox(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		return pTab->GetDOMCheckbox(id);
	}
	return std::weak_ptr<DOMCheckbox>();
}

void Mediator::RemoveDOMOverflowElement(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->RemoveDOMOverflowElement(id);
	}
}

void Mediator::RemoveDOMVideo(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->RemoveDOMVideo(id);
	}
}

void Mediator::EmulateKeyboardKey(int key, int scancode, int action, int mods)
{
	for (const auto& kvpair : _tabs)
	{
		_handler->EmulateKeyboardKey(GetBrowser(kvpair.second), key, scancode, mods);
		break;
	}
}

bool Mediator::EmulateKeyboardStrokes(TabCEFInterface* pTab, std::string input)
{
	if(const auto& browser = GetBrowser(pTab))
	{
		_handler->EmulateKeyboardStrokes(browser, input);
		return true;
	}
	return false;
}

bool Mediator::EmulateEnterKey(TabCEFInterface* pTab)
{
	if(const auto& browser = GetBrowser(pTab))
	{
		_handler->EmulateEnterKey(browser);
		return true;
	}
	return false;
}

bool Mediator::EmulateSelectAll(TabCEFInterface* pTab)
{
	if (const auto& browser = GetBrowser(pTab))
	{
		_handler->EmulateSelectAll(browser);
		return true;
	}
	return false;
}

void Mediator::ResetScrolling(TabCEFInterface * pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->ResetMainFramesScrolling(browser);
    }
}

void Mediator::SetURL(CefRefPtr<CefBrowser> browser)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        pTab->SetURL(browser->GetMainFrame()->GetURL());
    }
}

void Mediator::SetCanGoBack(CefRefPtr<CefBrowser> browser, bool canGoBack)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        pTab->SetCanGoBack(canGoBack);
    }
}

void Mediator::SetCanGoForward(CefRefPtr<CefBrowser> browser, bool canGoForward)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        pTab->SetCanGoForward(canGoForward);
    }
}

bool Mediator::ForwardFaviconBytes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefImage> img)
{
	if (const auto pTab = GetTab(browser))
	{
		if (!img)
			return true;

		int width, height;

		auto binary_value = img->GetAsBitmap(1.0, CEF_COLOR_TYPE_RGBA_8888, CEF_ALPHA_TYPE_PREMULTIPLIED, width, height);
		if (!binary_value)
		{
			LogInfo("Mediator: Favicon CefImage conversion to bitmap failed.");
			return true;
		}

		const size_t byte_size = binary_value->GetSize();
		if (byte_size == 0)
		{
			LogInfo("Mediator: Favicon CefImage conversion failed due to byte stream being empty.");
			return true;
		}
		if (width * height * sizeof(unsigned char) > byte_size)
		{
			LogInfo("Mediator: Something went wrong when retrieving image's resolution. Aborting...");
			return true;
		}

		// Write image bytes to unique_ptr
		auto upData = std::unique_ptr< std::vector<unsigned char> >(new std::vector<unsigned char>());
		upData->resize(byte_size / sizeof(unsigned char));
		binary_value->GetData(static_cast<void*>(upData->data()), byte_size, 0);

		pTab->ReceiveFaviconBytes(std::move(upData), width, height);
		return true;
	}
	LogInfo("Mediator: Forwarding favicon bytes to Tab failed. It might not exist anymore.");
	return false;
}

bool Mediator::IsFaviconAlreadyAvailable(CefRefPtr<CefBrowser> browser, CefString img_url)
{
	if (auto pTab = GetTab(browser))
	{
		return !pTab->IsFaviconAlreadyAvailable(img_url.ToString());
	}
	// Don't load image if Tab isn't available anymore
	return true;
}

bool Mediator::SetMetaKeywords(CefRefPtr<CefBrowser> browser, std::string content)
{
	if (auto pTab = GetTab(browser))
	{
		pTab->SetMetaKeywords(content);
	}
	return false;
}

void Mediator::SetZoomLevel(TabCEFInterface * pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->SetZoomLevel(browser);
    }
}

double Mediator::GetZoomLevel(CefRefPtr<CefBrowser> browser)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        return pTab->GetZoomLevel();
    }
    return NAN;
}

void Mediator::ReceivePageResolution(CefRefPtr<CefBrowser> browser, double width, double height)
{
    if (TabCEFInterface* pTab = GetTab(browser))
	{
        pTab->SetPageResolution(width, height);
    }
}

void Mediator::GetPageResolution(TabCEFInterface * pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        _handler->UpdatePageResolution(browser);
    }
}

void Mediator::ReceiveFixedElements(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg)
{
    CefRefPtr<CefListValue> args = msg->GetArgumentList();
    int id = args->GetInt(0);
    std::vector<Rect> fixedCoords = {};
    for (int i = 1; i < (int)args->GetSize(); i+=4)
    {
        fixedCoords.push_back(
            Rect(
                args->GetDouble(i), // top
                args->GetDouble(i + 1), // left
                args->GetDouble(i + 2), // bottom
                args->GetDouble(i + 3) // right
                )
            );
    }
    if (TabCEFInterface* pTab = GetTab(browser))
    {
      /*  LogDebug("Mediator: Sending ", fixedCoords.size(), " fixed element coordinate tupel(s) to Tab for fixedID=", id, ".");
        for (int i = 0; i < (int)fixedCoords.size(); i++)
        {
             LogDebug("\t-->", fixedCoords[i].top, ", ", fixedCoords[i].left, ", ", fixedCoords[i].bottom, ", ", fixedCoords[i].right);
        }*/
        pTab->AddFixedElementsCoordinates(id, fixedCoords);
    }
}

void Mediator::RemoveFixedElement(CefRefPtr<CefBrowser> browser, int id)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        //LogDebug("Mediator: Removing fixed element with id=", id, " from Tab.");
        pTab->RemoveFixedElement(id);
    }
}

void Mediator::Poll(TabCEFInterface* pTab, unsigned int numPartitions, unsigned int updatePartition)
{
	const std::string& polling_code = (numPartitions == 0) ? "CefPoll();" : 
		"CefPoll(" + std::to_string(numPartitions) + "," + std::to_string(updatePartition) + ");";
	if (pTab == NULL)
	{
		LogInfo("Mediator: Polling every tab...");
		for (const auto& pair: _browsers)
		{
			pair.second->GetMainFrame()->ExecuteJavaScript(polling_code, "CefPolling", 0);
		}
		return;
	}

	if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		browser->GetMainFrame()->ExecuteJavaScript(polling_code, "CefPolling", 0);
	}	
}

void Mediator::OnTabTitleChange(CefRefPtr<CefBrowser> browser, std::string title)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->SetURL(browser->GetMainFrame()->GetURL()); // guarantees that URL is set before the title
		pTab->SetTitle(title);
	}
}

void Mediator::OpenPopupTab(CefRefPtr<CefBrowser> browser, std::string url, bool javascript_access)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		CefRefPtr<CefRequestContext> request_context = (javascript_access) ? browser->GetHost()->GetRequestContext() : nullptr;
        pTab->AddTabAfter(url, request_context);
	}
}


void Mediator::ShowDevTools()
{
	LogInfo("Mediator: Showing DevTools...");

	// Setup
	CefWindowInfo windowInfo;
	CefBrowserSettings settings;

#if defined(OS_WIN)
	// Set title one Windows
	windowInfo.SetAsPopup(NULL, "DevTools");
#endif

	// Show dev tools for active tab
	if (_activeTab != nullptr)
	{
		auto browser = GetBrowser(_activeTab);
		if (browser != nullptr)
		{
			// Display dev tools of tab with handler of dev tools within extra window
			browser->GetHost()->ShowDevTools(windowInfo, _devToolsHandler, settings, CefPoint());
		}
	}
}

void Mediator::RegisterJavascriptCallback(std::string prefix, std::function<void(std::string)> callbackFunction)
{
	_handler->RegisterJavascriptCallback(prefix, callbackFunction);
}

void Mediator::EmulateLeftMouseButtonDown(TabCEFInterface* pTab, double x, double y)
{
	if (const CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		// Create mouse event
		CefMouseEvent event;
		event.x = x;
		event.y = y;

		// Send mouse down event to tab
		browser->GetHost()->SendMouseClickEvent(event, MBT_LEFT, false, 1);
	}
}

void Mediator::EmulateLeftMouseButtonUp(TabCEFInterface* pTab, double x, double y)
{
	if (const CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		// Create mouse event
		CefMouseEvent event;
		event.x = x;
		event.y = y;

		// Send mouse up event to tab
		browser->GetHost()->SendMouseClickEvent(event, MBT_LEFT, true, 1);
	}
}

void Mediator::InvokeCopy(TabCEFInterface * pTab)
{
	LogDebug("Mediator: InvokeCopy called.");

	if (const CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		browser->GetFocusedFrame()->Copy();
	}
}

void Mediator::InvokePaste(TabCEFInterface * pTab, double x, double y)
{
	LogDebug("Mediator: InvokePaste called on position (", x, ", ", y, ").");

	if (const CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		CefMouseEvent event;
		event.x = x;
		event.y = y;

		// Click at position where text should be pasted
		browser->GetHost()->SendMouseClickEvent(event, MBT_LEFT, false, 1);

		browser->GetFocusedFrame()->Paste();
	}
}

void Mediator::PutTextSelectionToClipboardAsync(TabCEFInterface* pTab)
{
	if (const CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		// Matter of taste whether to clear the clipboard or keep the current string until new one is set
		ClearClipboardText();

		// Asynchronous javascript call. See BrowserMsgRouter #select# in onQuery
		browser->GetMainFrame()->ExecuteJavaScript("GetTextSelection();", "", 0);
	}
}

void Mediator::SetClipboardText(std::string text)
{
	// Set clipboard
	_clipboard = text;

	// Notify user about the new content of the clipboard
	_pMaster->PushNotificationByKey("notification:copied_to_clipboard", MasterNotificationInterface::Type::NEUTRAL, false);
}

std::string Mediator::GetClipboardText() const
{
	return _clipboard;
}

void Mediator::ClearClipboardText()
{
	_clipboard = "";
}

TabCEFInterface* Mediator::GetTab(CefRefPtr<CefBrowser> browser) const
{
    int browserID = browser->GetIdentifier();
    if (_tabs.find(browserID) != _tabs.end())
    {
        return _tabs.at(browserID);
    }
    LogDebug("Mediator: The given CefBrowser pointer is not contained in key in Browser->Tab map (anymore).");
    return nullptr;
}

CefRefPtr<CefBrowser> Mediator::GetBrowser(TabCEFInterface * pTab) const
{
    if (_browsers.find(pTab) != _browsers.end())
    {
        return _browsers.at(pTab);
    }
    LogDebug("Mediator: The given Tab pointer is not contained in key in Tab->CefBrowser map (anymore).");
    return nullptr;
}


std::weak_ptr<Texture> Mediator::GetTexture(CefRefPtr<CefBrowser> browser)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        return pTab->GetWebViewTexture();
    }
    else if (_pendingTab)
    {
        LogDebug("Mediator: Pending Tab used for GetTexture.");
        return _pendingTab->GetWebViewTexture();
    }
    else
    {
        LogDebug("Mediator: GetTexture did not find corresponding texture.");
        // Return empty weak pointer
        return std::weak_ptr<Texture>();
    }
}

void Mediator::OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        // Set scrolling offset of correlating Tab
        pTab->SetScrollingOffset(x, y);
    }
}

void Mediator::GetResolution(CefRefPtr<CefBrowser> browser, int& width, int& height) const
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        pTab->GetWebRenderResolution(width, height);
    }
    else if (_pendingTab)
    {
        LogDebug("Mediator: Getting resolution from Tab _pendingTab.");
        _pendingTab->GetWebRenderResolution(width, height);
    }
    else
    {
        width = 0;
        height = 0;
        LogDebug("Mediator: GetResolution returned (0,0).");
    }
}

void Mediator::ResizeTabs()
{
    _handler->ResizeBrowsers();
}
