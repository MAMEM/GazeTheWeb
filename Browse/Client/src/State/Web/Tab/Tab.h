//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Daniel Mueller (muellerd@uni-koblenz.de)
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Class for tabs. Registers itself in CefMediator. Implements all Tab
// interfaces. Has queue of pipelines which are executed in the order they are
// pushed back.

// Coordinate systems of interest:
// - Window Coordinates: Complete window as created by window manager, in pixels
// - WebViewRelative Coordinates: Relative coordinates within web view
// - WebViewPixel Coordinates: Pixel coordinates within web view in *real screen* pixels
// - CEFPixel Coordinates: Pixel coordinates as rendered by CEF (is the only one known to code in CEF folder)
// - PagePixel Coordinates: Pixel coordinates as rendered by CEF inclusive scrolling
// All coordinate system have their origin at the upper left corner!

#ifndef TAB_H_
#define TAB_H_

#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/State/Web/Tab/Interface/TabCEFInterface.h"
#include "src/State/Web/WebTabInterface.h"
#include "src/CEF/Data/DOMNode.h"
#include "src/State/Web/Tab/WebView.h"
#include "src/State/Web/Tab/Pipelines/Pipeline.h"
#include "src/State/Web/Tab/Triggers/DOMTrigger.h"
#include "src/Utils/glmWrapper.h"
#include "src/Utils/Input.h"
#include "src/Global.h"
#include "submodules/eyeGUI/include/eyeGUI.h"
#include <vector>
#include <deque>
#include <memory>
#include <string>
#include <map>
#include <set>

// Forward declaration
class Master;
class Mediator;

// Class
class Tab : public TabInteractionInterface, public TabCEFInterface
{
public:

	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Constructor
    Tab(Master* pMaster, Mediator* pCefMediator, WebTabInterface* pWeb, std::string url);

    // Destructor
    virtual ~Tab();

    // Update
    void Update(float tpf, Input& rInput);

    // Draw
    void Draw() const;

    // Activate
    void Activate();

    // Deactivate
    void Deactivate();

    // Open URL. Does load it
    void OpenURL(std::string URL);

    // Abort pipeline
    void AbortAndClearPipelines();

    // Open next page
    void GoForward();

    // Open previous page
    void GoBack();

    // Reload page
    void Reload();

    // Can go back / forward
    bool CanGoBack() const { return _canGoBack; }
    bool CanGoForward() const { return _canGoForward; }

    // #################################
    // ### TAB INTERACTIVE INTERFACE ###
    // #################################
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabInteractionImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	// Nothing extra here

    // #############################
    // ### TAB OVERLAY INTERFACE ###
    // #############################
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabOverlayImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Add floating frame from overlay (not visible after creation)
    virtual int AddFloatingFrameToOverlay(
        std::string brickFilepath,
        float relativePositionX,
        float relativePositionY,
        float relativeSizeX,
        float relativeSizeY,
        std::map<std::string, std::string> idMapper);

    // Move floating frame in overlay
    virtual void SetPositionOfFloatingFrameInOverlay(
        int index,
        float relativePositionX,
        float relativePositionY);

    // Set visibility of floating frame in overlay
    virtual void SetVisibilityOfFloatingFrameInOverlay(int index, bool visible);

    // Remove floating frame from overlay
    virtual void RemoveFloatingFrameFromOverlay(int index);

    // Register button listener in overlay
    virtual void RegisterButtonListenerInOverlay(std::string id, std::function<void(void)> downCallback, std::function<void(void)> upCallback);

    // Unregister button listener callback in overlay
    virtual void UnregisterButtonListenerInOverlay(std::string id);

    // Register keyboard listener in overlay
    virtual void RegisterKeyboardListenerInOverlay(std::string id, std::function<void(std::string)> selectCallback, std::function<void(std::u16string)> pressCallback);

    // Unregister keyboard listener callback in overlay
    virtual void UnregisterKeyboardListenerInOverlay(std::string id);

    // Set case of keyboard letters
    virtual void SetCaseOfKeyboardLetters(std::string id, bool accept);

	// Classify currently selected key
	virtual void ClassifyKey(std::string id, bool accept);

    // Register word suggest listener in overlay
    virtual void RegisterWordSuggestListenerInOverlay(std::string id, std::function<void(std::u16string)> callback);

    // Unregister word suggest listener callback in overlay
    virtual void UnregisterWordSuggestListenerInOverlay(std::string id);

    // Use word suggest to display suggestions. Chosen one can be received by callback. Empty input clears suggestions
    virtual void DisplaySuggestionsInWordSuggest(std::string id, std::u16string input);

    // Get scrolling offset
    virtual void GetScrollingOffset(double& rScrollingOffsetX, double& rScrollingOffsetY) const;

	// Set content of text block directly or by key
	virtual void SetContentOfTextBlock(std::string id, std::u16string content);
	virtual void SetContentOfTextBlock(std::string id, std::string key);

	// Add content in text edit
	virtual void AddContentAtCursorInTextEdit(std::string id, std::u16string content);

	// Delete content in text edit
	virtual void DeleteContentAtCursorInTextEdit(std::string id, int letterCount);

	// Get content in active entity
	virtual std::u16string GetActiveEntityContentInTextEdit(std::string id) const;

	// Set content in active entity
	virtual void SetActiveEntityContentInTextEdit(std::string id, std::u16string content);

	// Get content of text edit
	virtual std::u16string GetContentOfTextEdit(std::string id);

	// Move cursor over letters in text edit. Positive letter count means rightward movement, else leftward
	virtual void MoveCursorOverLettersInTextEdit(std::string id, int letterCount);

	// Move cursor over words in text edit. Positive word count means rightward movement, else leftward
	virtual void MoveCursorOverWordsInTextEdit(std::string id, int wordCount);

	// Set activity of element
	virtual void SetElementActivity(std::string id, bool active, bool fade);

	// Getter for values of interest
	virtual int GetWebViewX() const;
	virtual int GetWebViewY() const;
	virtual int GetWebViewWidth() const;
	virtual int GetWebViewHeight() const;
	virtual int GetWebViewResolutionX() const;
	virtual int GetWebViewResolutionY() const;
	virtual int GetWindowWidth() const;
	virtual int GetWindowHeight() const;

    // ############################
    // ### TAB ACTION INTERFACE ###
    // ############################
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabActionImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Push back an pipeline
    virtual void PushBackPipeline(std::unique_ptr<Pipeline> upPipeline);

	// Emulate click in tab. Optionally converts WebViewPixel position to CEFpixel position before calling CEF method
	virtual void EmulateLeftMouseButtonClick(double x, double y, bool visualize = true, bool isWebViewPixelCoordinate = true);

	// Emulate mouse cursor in tab. Optionally converts WebViewPixel position to CEFpixel position before calling CEF method. Optional offset in rendered pixels
	virtual void EmulateMouseCursor(double x, double y, bool leftButtonPressed = false, bool isWebViewPixelCoordinate = true, double xOffset = 0, double yOffset = 0) ;

	// Emulate mouse wheel scrolling
	virtual void EmulateMouseWheelScrolling(double deltaX, double deltaY);

	// Emulate left mouse button down. Can be used to start text selection. Optional offset in rendered pixels
	virtual void EmulateLeftMouseButtonDown(double x, double y, bool isWebViewPixelCoordinate = true, double xOffset = 0, double yOffset = 0);

	// Emulate left mouse button up. Can be used to end text selection. Optional offset in rendered pixels
	virtual void EmulateLeftMouseButtonUp(double x, double y, bool isWebViewPixelCoordinate = true, double xOffset = 0, double yOffset = 0);

	// Asynchronous javascript call
	virtual void PutTextSelectionToClipboardAsync();

	// Get text out of global clipboard in mediator
	virtual std::string GetClipboardText() const;

	// Set text in text input field
	virtual void InputTextData(int64 frameID, int nodeID, std::string text, bool submit);

    // Get distance to next link and weak pointer to it. Returns empty weak pointer if no link available. Distance in page pixels
    virtual std::weak_ptr<const DOMNode> GetNearestLink(glm::vec2 pagePixelCoordinate, float& rDistance) const;

	// Execute scrolling in determined Overflow Element with elemId, x and y are delta values for each dimension
	virtual void ScrollOverflowElement(int elemId, int x, int y);

	// Convert WebViewPixel coordinate to CEFPixel coordinate
	void ConvertToCEFPixel(double& rWebViewPixelX, double& rWebViewPixelY) const;

	// Convert CEFPixel coordinate to WebViewPixel coordinate
	void ConvertToWebViewPixel(double& rCEFPixelX, double& rCEFPixelY) const;

	// Reply JavaScript dialog callback
	virtual void ReplyJSDialog(bool clickedOk, std::string userInput);

	// Set WebViewParameters for WebView
	virtual void SetWebViewParameters(WebViewParameters parameters) { _webViewParameters = parameters; }

    // #########################
    // ### TAB CEF INTERFACE ###
    // #########################
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabCEFImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Tell CEF callback which resolution web view texture should have
    virtual void GetWebRenderResolution(int& rWidth, int& rHeight) const;

    // Getter and setter for favicon URL
    virtual std::string GetFavIconURL() const { return _favIconUrl; }
    virtual void SetFavIconURL(std::string url) { _favIconUrl = url; }

    // Setter of URL. Does not load it. Should be called by CefMediator only
	virtual void SetURL(std::string URL) { _url = URL; }

    // Setter for can go back / go forward
    virtual void SetCanGoBack(bool canGoBack) { _canGoBack = canGoBack;	}
    virtual void SetCanGoForward(bool canGoForward) { _canGoForward = canGoForward; }

    // Receive favicon bytes as char vector ordered in RGBA. Accepts also NULL for upData!
    virtual void ReceiveFaviconBytes(std::unique_ptr< std::vector<unsigned char> > upData, int width, int height);
    virtual void ResetFaviconBytes(); // TODO

    // Get weak pointer to texture of web view
    virtual std::weak_ptr<Texture> GetWebViewTexture() { return _upWebView->GetTexture(); }

    // Add, remove and update Tab's current DOMNodes
    virtual void AddDOMNode(std::shared_ptr<DOMNode> spNode);
	virtual std::weak_ptr<DOMNode> GetDOMNode(DOMNodeType type, int nodeID);
    virtual void ClearDOMNodes();
	virtual void RemoveDOMNode(DOMNodeType type, int nodeID);

    // Receive callbacks from CefMediator upon scrolling offset changes
    virtual void SetScrollingOffset(double x, double y);

    // Getter for URL
    virtual std::string GetURL() const { return _url; }

    // Getter for current zoom level of corresponding browser
    virtual double GetZoomLevel() const { return _zoomLevel; }

    // Set page resolution from Cef Mediator
    virtual void SetPageResolution(double width, double height);

    virtual void AddFixedElementsCoordinates(int id, std::vector<Rect> elements);
    virtual void RemoveFixedElement(int id);

    // Set Tab's title text
	virtual void SetTitle(std::string title) { _title = title; }

    // Add new Tab after that one
	virtual void AddTabAfter(std::string URL) { _pWeb->PushAddTabAfterJob(this, URL); }

	// Receive current loading status of each frame
	virtual void SetLoadingStatus(int64 frameID, bool isMain, bool isLoading);
	
	// Overflow elements
	virtual void AddOverflowElement(std::shared_ptr<OverflowElement> overflowElem);
	virtual std::shared_ptr<OverflowElement> GetOverflowElement(int id);
	virtual void RemoveOverflowElement(int id);

	// Tell about JavaScript dialog
	virtual void RequestJSDialog(JavaScriptDialogType type, std::string message);

private:

	// Enumeration for icon state of tab
	enum class IconState { LOADING, ICON_NOT_FOUND, FAVICON };

	// Struct for click visulizations
	struct ClickVisualization
	{
		float x; // x pixel position of click (in web view coordinates)
		float y; // y pixel position of click (in web view coordinates)
		unsigned int frameIndex;
		float fading; // at start this is CLICK_VISUALIZATION_DURATION and decrements to zero
	};

    // Give listener full access
    friend class TabButtonListener;
    friend class TabSensorListener;
    friend class TabOverlayButtonListener;

	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabGUIImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Listener for GUI
    class TabButtonListener: public eyegui::ButtonListener
    {
    public:

        TabButtonListener(Tab* pTab) { _pTab = pTab; }
        virtual void hit(eyegui::Layout* pLayout, std::string id) {}
        virtual void down(eyegui::Layout* pLayout, std::string id);
        virtual void up(eyegui::Layout* pLayout, std::string id);

    private:

        Tab* _pTab;
    };

    class TabSensorListener: public eyegui::SensorListener
    {
    public:

        TabSensorListener(Tab* pTab) { _pTab = pTab; }
        virtual void penetrated(eyegui::Layout* pLayout, std::string id, float amount);

    private:

        Tab* _pTab;
    };

    class TabOverlayButtonListener : public eyegui::ButtonListener
    {
    public:

        TabOverlayButtonListener(Tab* pTab) { _pTab = pTab; }
        virtual void hit(eyegui::Layout* pLayout, std::string id) {}
        virtual void down(eyegui::Layout* pLayout, std::string id);
        virtual void up(eyegui::Layout* pLayout, std::string id);

    private:

        Tab* _pTab;
    };

    class TabOverlayKeyboardListener : public eyegui::KeyboardListener
    {
    public:

        TabOverlayKeyboardListener(Tab* pTab) { _pTab = pTab; }
		virtual void keySelected(eyegui::Layout* pLayout, std::string id, std::u16string value) {}
		virtual void keySelected(eyegui::Layout* pLayout, std::string id, std::string value);
        virtual void keyPressed(eyegui::Layout* pLayout, std::string id, std::u16string value);
        virtual void keyPressed(eyegui::Layout* pLayout, std::string id, std::string value) {}

    private:

        Tab* _pTab;
    };

    class TabOverlayWordSuggestListener : public eyegui::WordSuggestListener
    {
    public:

        TabOverlayWordSuggestListener(Tab* pTab) { _pTab = pTab; }
        virtual void chosen(eyegui::Layout* pLayout, std::string id, std::u16string value);
        virtual void chosen(eyegui::Layout* pLayout, std::string id, std::string value) {}

    private:

        Tab* _pTab;
    };

    // Instance of listener
    std::shared_ptr<TabButtonListener> _spTabButtonListener;
    std::shared_ptr<TabSensorListener> _spTabSensorListener;
    std::shared_ptr<TabOverlayButtonListener> _spTabOverlayButtonListener;
    std::shared_ptr<TabOverlayKeyboardListener> _spTabOverlayKeyboardListener;
    std::shared_ptr<TabOverlayWordSuggestListener> _spTabOverlayWordSuggestListener;

	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Set pipeline activity (indicates whether there is some pipeline to work on)
    void SetPipelineActivity(bool active);

    // Method to update and pipe accent color to eyeGUI
    void UpdateAccentColor(float tpf);

	// Draw debugging overlay
	void DrawDebuggingOverlay() const;

    // Pushes back click visualization which fades out. X and y are in pixels
    void PushBackClickVisualization(double x, double y);

	// Unique name for favicon which is stored in eyeGUI
	std::string GetFaviconIdentifier() const;

	// ###############
	// ### MEMBERS ###
	// ###############

    // Current URL
    std::string _url = "";

    // Vector with DOMTriggers (take DOM input node as input...)
    std::vector<std::unique_ptr<DOMTrigger> >_DOMTriggers;

	// Vector with DOMTextLinks
	std::vector<std::shared_ptr<DOMNode> >_DOMTextLinks;


	// Vector with DOMSelectFields
	std::vector<std::shared_ptr<DOMNode> > _DOMSelectFields;

	// Map nodeID to node itself, in order to access it when it has to be updated
	std::map<int, std::shared_ptr<DOMNode> > _TextLinkMap;
	std::map<int, std::shared_ptr<DOMNode> > _TextInputMap;
	std::map<int, std::shared_ptr<DOMNode> > _SelectFieldMap;

    // Web view in which website is rendered and displayed
    std::unique_ptr<WebView> _upWebView;

    // Pointer to master
    Master* _pMaster;

	// Pointer to web (that object whom this Tab belongs)
	WebTabInterface* _pWeb;

    // Pipelines which shall be executed
    std::deque<std::unique_ptr<Pipeline> > _pipelines;

    // Layouts
    eyegui::Layout* _pPanelLayout;
    eyegui::Layout* _pPipelineAbortLayout;
    eyegui::Layout* _pOverlayLayout;
    eyegui::Layout* _pScrollingOverlayLayout;
    eyegui::Layout* _pDebugLayout;

    // Parameters for WebView
    WebViewParameters _webViewParameters;

    // Layout color accent
    glm::vec4 _targetColorAccent = TAB_DEFAULT_COLOR_ACCENT;
    glm::vec4 _currentColorAccent = TAB_DEFAULT_COLOR_ACCENT;
    float _colorInterpolation = 1.f;

    // Bool to indicate whether some pipeline is active
    bool _pipelineActive = false;

    // Automatic scrolling
    bool _autoScrolling = false;
    float _autoScrollingValue = 0; // [-1..1]

    // Gaze mouse
    bool _gazeMouse = true;

    // Level of zooming
    double _zoomLevel = 1;

    // Pointer to mediator
    Mediator* _pCefMediator;

	// RenderItem used for debug rendering
	std::unique_ptr<RenderItem> _upDebugRenderItem;

    // Frame indices of scroll up and down overlays
    unsigned int _scrollUpProgressFrameIndex = 0;
    unsigned int _scrollDownProgressFrameIndex = 0;
    unsigned int _scrollUpSensorFrameIndex = 0;
    unsigned int _scrollDownSensorFrameIndex = 0;

    // Ids of elements in overlay (added / removed by triggers or actions)
    std::map<std::string, std::function<void(void)> > _overlayButtonDownCallbacks;
    std::map<std::string, std::function<void(void)> > _overlayButtonUpCallbacks;
	std::map<std::string, std::function<void(std::string)> > _overlayKeyboardSelectCallbacks;
    std::map<std::string, std::function<void(std::u16string)> > _overlayKeyboardPressCallbacks;
    std::map<std::string, std::function<void(std::u16string)> > _overlayWordSuggestCallbacks;

    // Time until Cef is asked for page resolution
    float _timeUntilGetPageResolution = TAB_GET_PAGE_RES_INTERVAL;

	// Vector with click visualizations, holding pairs of frame index and fading
	std::vector<ClickVisualization> _clickVisualizations;

    // Current page size
    double _pageWidth = 0, _pageHeight = 0;

    // Coordinates of current fixed elements on site
	std::vector<std::vector<Rect> > _fixedElements;

    // URL of current favIcon
    std::string _favIconUrl;

    // Scroll offset
    double _scrollingOffsetX = 0;
    double _scrollingOffsetY = 0;

    // Can go back / forward
    bool _canGoBack = false;
    bool _canGoForward = false;

    // Title of current website
    std::string _title;

	// Used for current loading status
	std::set<int64> _loadingFrames;

	// Boolean which indicates whether (at least some) favicon is loaded to eyeGUI
	bool _faviconLoaded = false;

	// Saves current state of icon
	IconState _iconState;

	// There are multiple icon for a loading tab. This it time until next one is displayed
	float _timeUntilNextLoadingIconFrame = 0;

	// Current loading icon frame
	int _loadingIconFrame = 0;

	std::vector<std::shared_ptr<OverflowElement> > _overflowElements;
};

#endif // TAB_H_
