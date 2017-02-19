//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef CEF_MEDIATOR_H_
#define CEF_MEDIATOR_H_

#include "src/MasterNotificationInterface.h"
#include "include/cef_browser.h"
#include "src/CEF/Data/DOMNodeType.h"
#include "src/CEF/Handler.h"
#include "src/CEF/DevToolsHandler.h"
#include "src/CEF/JavaScriptDialogType.h"
#include <set>
#include <map>
#include <memory>
#include <queue>
#include <functional>

/**
*	Expand CefApp by methods and attributes used to communicate with Master and
*   Tab classes.
*
*/

// Forward declarations
class TabCEFInterface;
class Texture;
class DOMNode;
class OverflowElement;

typedef int BrowserID;

class Mediator
{
public:

	// Setter for master pointer (MUST be called before usage)
	void SetMaster(MasterNotificationInterface* pMaster);

    // Receive tab specific commands
    void RegisterTab(TabCEFInterface* pTab);
    void UnregisterTab(TabCEFInterface* pClosing);
    void RefreshTab(TabCEFInterface* pTab);	// Daniel: "This name is weird.. have been searching for method to load URL and this is the answer? :D
    void ReloadTab(TabCEFInterface* pTab);
    void GoBack(TabCEFInterface* pTab);
    void GoForward(TabCEFInterface* pTab);

	// Request / Reply JavaScript dialog callback
	void RequestJSDialog(CefRefPtr<CefBrowser> browser, JavaScriptDialogType type, std::string message);
	void ReplyJSDialog(TabCEFInterface* pTab, bool clicked_ok, std::string user_input);

    // Renderer::OnPaint calls this method in order to receive corresponding Texture
    std::weak_ptr<Texture> GetTexture(CefRefPtr<CefBrowser> browser);

    // Equally named Renderer's callback pipes data to CefMediator, who sets offset in correlating tab
    void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y);

    // Get resolution of rendering
    void GetResolution(CefRefPtr<CefBrowser> browser, int& width, int& height) const;

    // Called by Master when window resize happens
    void ResizeTabs();

    // Call from Master to do message loop work
    void DoMessageLoopWork();

    // Emulation of left mouse button press and release in specific Tab
    void EmulateMouseCursor(TabCEFInterface* pTab, double x, double y, bool leftButtonPressed); // leftButtonPressed seems necessary
																								// between mouse button down and up during text selection
    void EmulateLeftMouseButtonClick(TabCEFInterface* pTab, double x, double y);
    void EmulateMouseWheelScrolling(TabCEFInterface* pTab, double deltaX, double deltaY);

    void ResetScrolling(TabCEFInterface* pTab);

    // Sets Tab's URL attribute, called by Handler when main frame starts loading a page
    void SetURL(CefRefPtr<CefBrowser> browser);

    void ReceiveIPCMessageforFavIcon(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg);
    void ResetFavicon(CefRefPtr<CefBrowser> browser);

    void SetCanGoBack(CefRefPtr<CefBrowser> browser, bool canGoBack);
    void SetCanGoForward(CefRefPtr<CefBrowser> browser, bool canGoForward);

    // External zoom level request
    void SetZoomLevel(TabCEFInterface* pTab);
    // Called by Handler OnLoadStart
    double GetZoomLevel(CefRefPtr<CefBrowser> browser);

    void ReceivePageResolution(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg);

    // Called when Tab realizes that it might reach end of page while scrolling
    void GetPageResolution(TabCEFInterface* pTab);

    void ReceiveFixedElements(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg);
    void RemoveFixedElement(CefRefPtr<CefBrowser> browser, int id);

    // Called by master, only. Quite similar to a update method
    void Poll(float tpf);

	// Update Tab's title when title change callback is received
	void OnTabTitleChange(CefRefPtr<CefBrowser> browser, std::string title);

	// Add new Tab with given URL at the position after the current Tab (in context of Tab overview)
	void OpenPopupTab(CefRefPtr<CefBrowser> browser, std::string url);

	bool SetLoadingStatus(CefRefPtr<CefBrowser> browser, int64 frameID, bool isMain, bool isLoading);

	// Execute scrolling request from Tab in determined Overflow Element with elemId
	void ScrollOverflowElement(TabCEFInterface* pTab, int elemId, int x, int y, std::vector<int> fixedIds = {});
	// Add OverflowElement to corresponding Tab
	void AddOverflowElement(CefRefPtr<CefBrowser> browser, std::shared_ptr<OverflowElement> overflowElem);
	std::weak_ptr<OverflowElement> GetOverflowElement(CefRefPtr<CefBrowser> browser, int id);
	void RemoveOverflowElement(CefRefPtr<CefBrowser> browser, int id);

	/* DOM relevant methods */

	// Used by BrowserMsgRouter to pass blank DOMNodes to Tab
	void AddDOMNode(CefRefPtr<CefBrowser> browser, std::shared_ptr<DOMNode> spNode);

	void ClearDOMNodes(CefRefPtr<CefBrowser> browser);

	void RemoveDOMNode(CefRefPtr<CefBrowser> browser, DOMNodeType type, int nodeID);

	// Used by BrowserMsgRouter
	std::weak_ptr<DOMNode> GetDOMNode(CefRefPtr<CefBrowser> browser, DOMNodeType type, int nodeID);

	// TODO: Call Javascript function instead of injecting more JS code
	bool InputTextData(TabCEFInterface* tab, int64 frameID, int nodeID, std::string text, bool submit = false);

	void FillDOMNodeWithData(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg);

	// Activate rendering in given Tab and deactivate it for all other Tabs
	void SetActiveTab(TabCEFInterface* pTab);	// TODO Raphael: Call this method when Tab is changed via GUI
	
	// Master calls this method upon GLFW keyboard input in order to open new window with DevTools (for active Tab)
	void ShowDevTools();

	void RegisterJavascriptCallback(std::string prefix, std::function<void(std::string)> callbackFunction);

	// ### MOUSE INTERACTION ###

	// Can be used as start and end of text selection
	void EmulateLeftMouseButtonDown(TabCEFInterface* pTab, double x, double y);
	void EmulateLeftMouseButtonUp(TabCEFInterface* pTab, double x, double y);

	// Copy and paste functionality
	void InvokeCopy(TabCEFInterface* pTab);
	void InvokePaste(TabCEFInterface* pTab, double x, double y);

	// ### GLOBAL CLIPBOARD ###
	void PutTextSelectionToClipboardAsync(TabCEFInterface* pTab); // asynchronous javascript call
	void SetClipboardText(std::string text); // should be called by browser msg router
	std::string GetClipboardText() const; // should be called by tab
	void ClearClipboardText(); // should be called by mediator before asking Javascript to extract selected string from page

protected:

    // Members
    CefRefPtr<Handler> _handler;
	CefRefPtr<DevToolsHandler> _devToolsHandler;

    // Save corresponding (Tab, CefBrowser)-pairs in two maps
    std::map<TabCEFInterface*, CefRefPtr<CefBrowser>> _browsers;
    std::map<BrowserID, TabCEFInterface*> _tabs;

	TabCEFInterface* _activeTab = NULL;

    TabCEFInterface* _pendingTab = NULL; // Used in Tab registration progress (at first, Renderer works to fast for map access)

	// Simple internal clipboard
	std::string _clipboard = "";

    // Use these methods for less coding overhead by checking if key exists in map
    TabCEFInterface* GetTab(CefRefPtr<CefBrowser> browser) const;
    CefRefPtr<CefBrowser> GetBrowser(TabCEFInterface* pTab) const;

	// Pointer to master (but only functions exposed through the interface)
	MasterNotificationInterface* _pMaster = NULL;
};


#endif // CEF_MEDIATOR_H_
