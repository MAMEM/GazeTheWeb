//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef CEF_MEDIATOR_H_
#define CEF_MEDIATOR_H_

#include "src/Master/MasterNotificationInterface.h"
#include "include/cef_browser.h"
#include "src/CEF/Handler.h"
#include "src/CEF/DevToolsHandler.h"
#include "src/CEF/JavaScriptDialogType.h"
#include <set>
#include <map>
#include <memory>
#include <queue>
#include <functional>
#include "include\cef_base.h"

/**
*	Expand CefApp by methods and attributes used to communicate with Master and
*   Tab classes.
*
*/

// Forward declarations
class TabCEFInterface;
class Texture;
class DOMNode;
class DOMOverflowElement;
class DOMTextInput;
class DOMLink;
class DOMSelectField;
class DOMVideo;
class DOMCheckbox;

typedef int BrowserID;

class Mediator
{
public:

	// Setter for master pointer (MUST be called before usage)
	void SetMaster(MasterNotificationInterface* pMaster);

    // Receive tab specific commands
    void RegisterTab(TabCEFInterface* pTab, std::string URL, CefRefPtr<CefRequestContext> request_context = nullptr);
    void UnregisterTab(TabCEFInterface* pClosing);
    void LoadURLInTab(TabCEFInterface* pTab, std::string URL);
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
    void EmulateMouseWheelScrolling(TabCEFInterface* pTab, double deltaX, double deltaY, int x = 0, int y = 0);
	/*
	*  @param[in] key The [keyboard key](@ref keys) that was pressed or released.
	*  @param[in] scancode The system-specific scancode of the key.
	*  @param[in] action `GLFW_PRESS`, `GLFW_RELEASE` or `GLFW_REPEAT`.
	*  @param[in] mods Bit field describing which [modifier keys](@ref mods) were
	*/
	void EmulateKeyboardKey(int key, int scancode, int action, int mods);
	bool EmulateKeyboardStrokes(TabCEFInterface* pTab, std::string input);
	bool EmulateEnterKey(TabCEFInterface* pTab);
	bool EmulateSelectAll(TabCEFInterface* pTab);

    void ResetScrolling(TabCEFInterface* pTab);

    // Sets Tab's URL attribute, called by Handler when main frame starts loading a page
    void SetURL(CefRefPtr<CefBrowser> browser);


	void StartImageDownload(CefRefPtr<CefBrowser> browser, CefString img_url) {
		_handler->StartImageDownload(browser, img_url);
	}

    void SetCanGoBack(CefRefPtr<CefBrowser> browser, bool canGoBack);
    void SetCanGoForward(CefRefPtr<CefBrowser> browser, bool canGoForward);


	// ### FAVICON SETTING ###
	void ResetFavicon(CefRefPtr<CefBrowser> browser);

	// Get byte code from CefImage and send it to corresponding Tab
	bool ForwardFaviconBytes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefImage> img);

	// Check if favicon was already loaded before new image is also loaded
	bool IsFaviconAlreadyAvailable(CefRefPtr<CefBrowser> browser, CefString img_url);



	bool SetMetaKeywords(CefRefPtr<CefBrowser> browser, std::string content);

    // External zoom level request
    void SetZoomLevel(TabCEFInterface* pTab);
    // Called by Handler OnLoadStart
    double GetZoomLevel(CefRefPtr<CefBrowser> browser);

	void ReceivePageResolution(CefRefPtr<CefBrowser> browser, double width, double height);

    // Called when Tab realizes that it might reach end of page while scrolling
    void GetPageResolution(TabCEFInterface* pTab);

    void ReceiveFixedElements(CefRefPtr<CefBrowser> browser, CefRefPtr<CefProcessMessage> msg);
    void RemoveFixedElement(CefRefPtr<CefBrowser> browser, int id);

	// If numPartitions != 0, divide polled elements into numPartitions splits and update split with 
	// updatePartition as array index (starting with 0!)
	void Poll(TabCEFInterface* pTab = NULL, unsigned int numPartitions=0, unsigned int updatePartition=0);

	// Update Tab's title when title change callback is received
	void OnTabTitleChange(CefRefPtr<CefBrowser> browser, std::string title);

	// Add new Tab with given URL at the position after the current Tab (in context of Tab overview)
	void OpenPopupTab(CefRefPtr<CefBrowser> browser, std::string url, bool javascript_access = false);


	bool SetLoadingStatus(CefRefPtr<CefBrowser> browser, bool isLoading, bool isMainFrame);


	/* DOM relevant methods */

	// Called by MsgRouter, will create blank DOMNodes in Tab
	void AddDOMTextInput(CefRefPtr<CefBrowser> browser, int id);
	void AddDOMLink(CefRefPtr<CefBrowser> browser, int id);
	void AddDOMSelectField(CefRefPtr<CefBrowser> browser, int id);
	void AddDOMOverflowElement(CefRefPtr<CefBrowser> browser, int id);
	void AddDOMVideo(CefRefPtr<CefBrowser> browser, int id);
	void AddDOMCheckbox(CefRefPtr<CefBrowser> browser, int id);
	
	void ClearDOMNodes(CefRefPtr<CefBrowser> browser);

	void RemoveDOMTextInput(CefRefPtr<CefBrowser> browser, int id);
	void RemoveDOMLink(CefRefPtr<CefBrowser> browser, int id);
	void RemoveDOMSelectField(CefRefPtr<CefBrowser> browser, int id);
	void RemoveDOMOverflowElement(CefRefPtr<CefBrowser> browser, int id);
	void RemoveDOMVideo(CefRefPtr<CefBrowser> browser, int id);
	void RemoveDOMCheckbox(CefRefPtr<CefBrowser> browser, int id);


	// Receive weak_ptr, only perform Initialize(objMsg) and Update(attr) operations
	std::weak_ptr<DOMTextInput> GetDOMTextInput(CefRefPtr<CefBrowser> browser, int id);
	std::weak_ptr<DOMLink> GetDOMLink(CefRefPtr<CefBrowser> browser, int id);
	std::weak_ptr<DOMSelectField> GetDOMSelectField(CefRefPtr<CefBrowser> browser, int id);
	std::weak_ptr<DOMOverflowElement> GetDOMOverflowElement(CefRefPtr<CefBrowser> browser, int id);
	std::weak_ptr<DOMVideo> GetDOMVideo(CefRefPtr<CefBrowser> browser, int id);
	std::weak_ptr<DOMCheckbox> GetDOMCheckbox(CefRefPtr<CefBrowser> browser, int id);

	// DOM node objects can directly send interaction messages to Renderer
	bool SendProcessMessageToRenderer(CefRefPtr<CefProcessMessage> msg, TabCEFInterface* pTab);

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

	// Decide whether to block ads
	void BlockAds(bool blockAds) { _handler->BlockAds(blockAds); }

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
