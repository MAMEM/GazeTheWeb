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
#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"


void Mediator::SetMaster(MasterNotificationInterface* pMaster)
{
	_pMaster = pMaster;
}

void Mediator::RegisterTab(TabCEFInterface* pTab)
{
    CEF_REQUIRE_UI_THREAD();

    /*Following information might be changed and used in the future
    http://magpcss.org/ceforum/apidocs3/projects/%28default%29/_cef_browser_settings_t.html
        - webgl: enable WebGL
        - windowless_frame_rate: default 30 fps, maximum 60
    */
    CefWindowInfo window_info;				// CefBrowser relevant information
    window_info.SetAsWindowless(0, false);	// Window handle set to zero (may cause visual errors)
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

    LogDebug("Mediator: Creating new CefBrowser at Tab registration.");
    // Create new CefBrowser with given information
    CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(
        window_info, _handler.get(), "about:blank", browser_settings, NULL);

    // Fill maps with correlating Tab and CefBrowsre
    _browsers.emplace(pTab, browser);
    _tabs.emplace(browser->GetIdentifier(), pTab);
    _pendingTab = NULL;

	RefreshTab(pTab);

	// TODO: MutationObserver observes wrong document object if browser is created by CreateBrowserSync with an URL,
	// thus no mutations will be recognized on startup
	// We are currently preventing this by creating the browser with "about:blank" and calling RefreshTab afterwards
	// in order to load the 'real' URL. Then, MutationObserver observes the 'right' document
	// But why does this scenario exist? Might be interesting to investigate in the future ;)
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

void Mediator::RefreshTab(TabCEFInterface * pTab)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
    {
        // Get Tab object and load Tab's URL in CefBrowser
        _handler->LoadPage(_browsers.at(pTab), pTab->GetURL());
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

void Mediator::ReceiveIPCMessageforFavIcon(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg)
{
    const std::string& msgName = msg->GetName().ToString();

    if (msgName == "ReceiveFavIconBytes")
    {
		LogDebug("Mediator: Received Favicon Bytes...");

        CefRefPtr<CefListValue> args = msg->GetArgumentList();
        int index = 0;
        if(TabCEFInterface* pTab = GetTab(browser))
        {
            const int width = args->GetInt(index++);
            const int height = args->GetInt(index++);

            if (width <= 0 || height <= 0)
            {
                LogDebug("Mediator: Failure. Received favicon width or height less than zero!");

                if (TabCEFInterface* pInnerTab = GetTab(browser))
                {
                    // Inform Tab anyway
                    pInnerTab->ReceiveFaviconBytes(nullptr, width, height);
                }
            }
            else
            {
                LogDebug("Mediator: Success. Copying favicon bytes to Tab... (w= ", width, ", h=", height,")");

                // Extract width, height and byte information
                std::unique_ptr<std::vector<unsigned char> > upData = std::unique_ptr<std::vector<unsigned char> >(new std::vector<unsigned char>);
                for (int i = 0; i < height*width; i++)
                {
                    int four_bytes = args->GetInt(index++);

                    for (int j = 3; j >= 0; j--)
                    {
                        // Read byte for each channel (RGBA) from 4 byte integer value
                        upData->push_back((four_bytes >> (j*8)) & 0xFF);
                    }
                }

                // Pipe extracted information to Tab
                pTab->ReceiveFaviconBytes(std::move(upData), width, height);

                // Save new favicon URL in Tab
                //pTab->SetFavIconURL(iconUrl);	// TODO: With favicon callback, saving favicon URL in Tab really neccessary? I don't think so.
            }

        }
    }
}

void Mediator::ResetFavicon(CefRefPtr<CefBrowser> browser)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        pTab->ResetFaviconBytes();
    }
}

void Mediator::AddDOMNode(CefRefPtr<CefBrowser> browser, std::shared_ptr<DOMSelectField> spNode)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		// Find corresponding Tab and add DOM Node to its list of nodes
		pTab->AddDOMNode(spNode);
	}
}

void Mediator::AddDOMNode(CefRefPtr<CefBrowser> browser, std::shared_ptr<DOMNode> spNode)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        // Find corresponding Tab and add DOM Node to its list of nodes
        pTab->AddDOMNode(spNode);
    }
}

void Mediator::ClearDOMNodes(CefRefPtr<CefBrowser> browser)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        // Clear corresponding Tabs DOM Node list (implicitly destroy all DOM Node objects)
        pTab->ClearDOMNodes();
		//LogDebug("Mediator: ### DISABLED DOM CLEARING FOR LINK TESTING ###");
        LogDebug("Mediator: Clearing Tab's DOM nodes belonging to browserID = ", browser->GetIdentifier());
    }
}

bool Mediator::InputTextData(TabCEFInterface* tab, int64 frameID, int nodeID, std::string text, bool submit)
{
    if (CefRefPtr<CefBrowser> browser = GetBrowser(tab))
    {
        LogDebug("Mediator: Input text '", text, "' in frame id = ", frameID, " (nodeID = ", nodeID, ").");
        return _handler->InputTextData(browser, frameID, nodeID, text, submit);
    }
    return false;
}


// TODO: Generic CallJSFunction msg with JS function as argument name and the rest as arguments?
void Mediator::SetSelectionIndex(TabCEFInterface * tab, int nodeId, int index)
{
	if (const auto& browser = GetBrowser(tab))
	{
		auto& msg = CefProcessMessage::Create("SetSelectionIndex");
		auto& args = msg->GetArgumentList();
		args->SetInt(0, nodeId);
		args->SetInt(1, index);
		browser->SendProcessMessage(PID_RENDERER, msg);
	}
}

void Mediator::FillDOMNodeWithData(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		int index = 0;
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		int type = args->GetInt(index++);
		int nodeID = args->GetInt(index++);
		bool visible = args->GetBool(index++);

		// TODO(Daniel): Call an update of different attributes for different node types

		// Read out multiple Rects
		std::vector<Rect> rects;

		for (int i = index; i + 3 < args->GetSize(); i += 4)
		{
			rects.push_back(
				Rect(
					args->GetDouble(i),
					args->GetDouble(i + 1),
					args->GetDouble(i + 2),
					args->GetDouble(i + 3)
					)
				);
		}

		// Receive weak_ptr to target node and use it as shared_ptr targetNode
		if (auto targetNode = pTab->GetDOMNode((DOMNodeType) type, nodeID).lock())
		{
			// Update target nodes Rects
			targetNode->SetRects(std::make_shared<std::vector<Rect>>(rects));

			// Set target node's visibility
			targetNode->SetVisibility(visible);
			//if (!visible) LogDebug("Mediator: Set node's visibility to false after its creation");
		}
		else
		{
			LogDebug("Mediator: Trying to update node information but DOMNode object doesn't seem to exist!");
		}
	}
}

// TODO: Use only this method for initialization of nodes, which were created empty
void Mediator::InitializeDOMNode(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		const auto& args = msg->GetArgumentList();
		const int type = args->GetInt(0);
		const int id = args->GetInt(1);

		if (type == 2)
		{
			if (auto node = pTab->GetDOMSelectFieldNode(id).lock())
			{
				// Extract rect data
				std::vector<Rect> rects;
				CefRefPtr<CefListValue> rectList = args->GetList(2);
				for (int i = 0; i < rectList->GetSize(); i++)
				{
					CefRefPtr<CefListValue> rectEntry = rectList->GetList(i);
					std::vector<float> data;
					for (int j = 0; j < rectEntry->GetSize(); j++)
					{
						data.push_back(rectEntry->GetDouble(j));
					}
					rects.push_back(Rect(data));
				}
				node->SetRects(std::make_shared<std::vector<Rect>>(rects));

				// Extract options data
				std::vector<std::string> options;
				CefRefPtr<CefListValue> optionsList = args->GetList(3);
				for (int i = 0; i < optionsList->GetSize(); i++)
				{
					options.push_back(optionsList->GetString(i));
				}
				node->SetOptions(options);

			}
		}


	}
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



bool Mediator::SetLoadingStatus(CefRefPtr<CefBrowser> browser, int64 frameID, bool isMain, bool isLoading)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->SetLoadingStatus(frameID, isMain, isLoading);
		return true;
	}
	else
	{
		return false;
	}
	
}

void Mediator::ScrollOverflowElement(TabCEFInterface * pTab, int elemId, int x, int y, std::vector<int> fixedIds)
{
	if (CefRefPtr<CefBrowser> browser = GetBrowser(pTab))
	{
		_handler->ScrollOverflowElement(browser, elemId, x, y, fixedIds);
	}
}

void Mediator::AddOverflowElement(CefRefPtr<CefBrowser> browser, std::shared_ptr<OverflowElement> overflowElem)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->AddOverflowElement(overflowElem);
	}
}

std::weak_ptr<OverflowElement> Mediator::GetOverflowElement(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		return pTab->GetOverflowElement(id);
	}
	return std::weak_ptr<OverflowElement>();
}

void Mediator::RemoveOverflowElement(CefRefPtr<CefBrowser> browser, int id)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->RemoveOverflowElement(id);
	}
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

void Mediator::ReceivePageResolution(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg)
{
    if (TabCEFInterface* pTab = GetTab(browser))
    {
        CefRefPtr<CefListValue> args = msg->GetArgumentList();
		double pageWidth = args->GetDouble(0);
		double pageHeight = args->GetDouble(1);
        pTab->SetPageResolution(pageWidth, pageHeight);
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

void Mediator::Poll(float tpf)
{
    // TODO Daniel
}

void Mediator::OnTabTitleChange(CefRefPtr<CefBrowser> browser, std::string title)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->SetTitle(title);
	}
}

void Mediator::OpenPopupTab(CefRefPtr<CefBrowser> browser, std::string url)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
        pTab->AddTabAfter(url);
	}
}


void Mediator::RemoveDOMNode(CefRefPtr<CefBrowser> browser, DOMNodeType type, int nodeID)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		pTab->RemoveDOMNode(type, nodeID);
	}
}

std::weak_ptr<DOMNode> Mediator::GetDOMNode(CefRefPtr<CefBrowser> browser, DOMNodeType type, int nodeID)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		return pTab->GetDOMNode(type, nodeID);
	}
	return std::weak_ptr<DOMNode>();
}

std::weak_ptr<DOMSelectField> Mediator::GetDOMSelectFieldNode(CefRefPtr<CefBrowser> browser, int nodeId)
{
	if (TabCEFInterface* pTab = GetTab(browser))
	{
		return pTab->GetDOMSelectFieldNode(nodeId);
	}
	return std::weak_ptr<DOMSelectField>();
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
	_pMaster->PushNotificationByKey("notification:copied_to_clipboard");
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
