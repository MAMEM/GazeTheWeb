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
// TODO: y-axis always from top to down?
// TODO: define coordinate system for unscrolled (like defined in webpage code)

#ifndef TAB_H_
#define TAB_H_

#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"
#include "src/State/Web/Tab/Interface/TabCEFInterface.h"
#include "src/State/Web/Tab/Interface/TabDOMNodeInterface.h"
#include "src/State/Web/WebTabInterface.h"
#include "src/CEF/Data/DOMNode.h"
#include "src/State/Web/Tab/WebView.h"
#include "src/State/Web/Tab/Pipelines/Pipeline.h"
#include "src/State/Web/Tab/Triggers/TextInputTrigger.h"
#include "src/State/Web/Tab/Triggers/SelectFieldTrigger.h"
#include "src/State/Web/Tab/Triggers/VideoModeTrigger.h"
#include "src/Utils/glmWrapper.h"
#include "src/Input/Input.h"
#include "src/Input/VoiceInput.h"
#include "src/Global.h"
#include "src/State/Web/Tab/Pipelines/PointingEvaluationPipeline.h"
#include "submodules/eyeGUI/include/eyeGUI.h"
#include <vector>
#include <deque>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <future>

// Forward declaration
class Master;
class Mediator;
class SocialRecord;
enum class SocialPlatform;

// Class
class Tab : public TabInteractionInterface, 
			public TabCEFInterface,
			public TabDOMNodeInterface
{
public:

	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Constructor
    Tab(Master* pMaster, Mediator* pCefMediator, WebTabInterface* pWeb, std::string url, bool dataTransfer, 
	CefRefPtr<CefRequestContext> request_context = nullptr);

    // Destructor
    virtual ~Tab();

    // Update
    void Update(float tpf, const std::shared_ptr<const Input> spInput, std::shared_ptr<VoiceAction> spVoiceInput);

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

	// Pushs back pointing evaluation pipeline
	void PushBackPointingEvaluationPipeline(PointingApproach approach);

	// Get last time per frame
	float GetLastTimePerFrame() const { return _lastTimePerFrame; }

	// Pause data transfer
	void SetDataTransfer(bool active);

	// Notify about click
	void NotifyClick(std::string tag, std::string id, float x, float y);

	// Trigger text input trigger
	void ScheduleTextInputTrigger(int id);

	// Trigger select field trigger
	void ScheduleSelectFieldTrigger(int id);

	// Trigger video trigger
	void ScheduleVideoModeTrigger(int id);

	// Retrieve all text links, their id and rects
	struct DOMLinkInfo
	{
		DOMLinkInfo(std::vector<Rect> rects, std::string text) : rects(rects), text(text) {}
		std::vector<Rect> rects;
		std::string text;
	};
	std::vector<DOMLinkInfo> RetrieveDOMLinkInfos() const;

//	struct DOMCheckboxInfo
//	{
//		DOMCheckboxInfo(std::vector<Rect> rects) : rects(rects) {}
//		std::vector<Rect> rects;
//	};
//	std::vector<DOMCheckboxInfo> RetrieveDOMCheckboxInfos() const;

	// Getter for URL
	std::string GetURL() const { return _url; }

	// Getter for title
	std::string GetTitle() const { return _title; }

	// Get color accent
	glm::vec4 GetColorAccent() const { return _targetColorAccent; }

	// Update award icon
	void SetAwardIcon(Award award); 

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
	virtual int AddFloatingFrameToOverlay(
		std::string brickFilepath,
		float relativePositionX,
		float relativePositionY,
		float relativeSizeX,
		float relativeSizeY);

    // Move floating frame in overlay
    virtual void SetPositionOfFloatingFrameInOverlay(
        int index,
        float relativePositionX,
        float relativePositionY);

	// Size floating frame in overlay
	virtual void SetSizeOfFloatingFrameInOverlay(
		int index,
		float relativeWidth,
		float relativeHeight);

    // Set visibility of floating frame in overlay
    virtual void SetVisibilityOfFloatingFrameInOverlay(int index, bool visible);

    // Remove floating frame from overlay
    virtual void RemoveFloatingFrameFromOverlay(int index);

    // Register button listener in overlay
    virtual void RegisterButtonListenerInOverlay(std::string id, std::function<void(void)> downCallback, std::function<void(void)> upCallback, std::function<void(void)> selectedCallback);

    // Unregister button listener callback in overlay
    virtual void UnregisterButtonListenerInOverlay(std::string id);

	// Classify button hit
	virtual void ClassifyButton(std::string id, bool accept);

    // Register keyboard listener in overlay
    virtual void RegisterKeyboardListenerInOverlay(std::string id, std::function<void(std::string)> selectCallback, std::function<void(std::u16string)> pressCallback);

    // Unregister keyboard listener callback in overlay
    virtual void UnregisterKeyboardListenerInOverlay(std::string id);

    // Set case of keyboard letters
    virtual void SetCaseOfKeyboardLetters(std::string id, bool accept);

	// Set keyboard 
	virtual void SetKeymapOfKeyboard(std::string id, unsigned int keymap);

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

	// Delete all content in text edit
	virtual void DeleteContentInTextEdit(std::string id);

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

	// Tell button to go up
	virtual void ButtonUp(std::string id);

	// Set global keyboard layout
	virtual void SetKeyboardLayout(eyegui::KeyboardLayout keyboardLayout);

	// Set space of flow
	virtual void SetSpaceOfFlow(std::string id, float space);

	// Add brick to stack
	virtual void AddBrickToStack(
		std::string id,
		std::string brickFilepath,
		std::map<std::string, std::string> idMapper);

	// Getter for values of interest
	virtual int GetWebViewX() const;
	virtual int GetWebViewY() const;
	virtual int GetWebViewWidth() const;
	virtual int GetWebViewHeight() const;
	virtual int GetWebViewResolutionX() const;
	virtual int GetWebViewResolutionY() const;
	virtual int GetWindowWidth() const;
	virtual int GetWindowHeight() const;

	// Use eyeGUI drift map to perform drift correction. Does nothing if USE_EYEGUI_DRIFT_MAP is false
	virtual void ApplyGazeDriftCorrection(float& rPixelX, float& rPixelY) const;

    // ############################
    // ### TAB ACTION INTERFACE ###
    // ############################
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabActionImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // Push back an pipeline
    virtual void PushBackPipeline(std::unique_ptr<Pipeline> upPipeline);

	// Emulate click in tab. Optionally converts WebViewPixel position to CEFpixel position before calling CEF method
	virtual void EmulateLeftMouseButtonClick(double x, double y, bool visualize = true, bool isWebViewPixelCoordinate = true, bool userTriggered = false);

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

    // Get distance to next link and weak pointer to it. Returns empty weak pointer if no link available. Distance in page pixels
    virtual std::weak_ptr<const DOMNode> GetNearestLink(glm::vec2 pagePixelCoordinate, float& rDistance) const;

	// Convert WebViewPixel coordinate to CEFPixel coordinate
	void ConvertToCEFPixel(double& rWebViewPixelX, double& rWebViewPixelY) const;

	// Convert CEFPixel coordinate to WebViewPixel coordinate
	void ConvertToWebViewPixel(double& rCEFPixelX, double& rCEFPixelY) const;

	// Reply JavaScript dialog callback
	virtual void ReplyJSDialog(bool clickedOk, std::string userInput);

	// Play sound
	virtual void PlaySound(std::string filepath);

	// Get interface for custom transformations of input
	virtual std::weak_ptr<CustomTransformationInterface> GetCustomTransformationInterface() const;

	// Notify about text input
	virtual void NotifyTextInput(std::string tag, std::string id, int charCount, int charDistance, float x, float y, float duration);

	// Set WebViewParameters for WebView
	virtual void SetWebViewParameters(WebViewParameters parameters) { _webViewParameters = parameters; }

	// ###############################
	// ### TAB DOM NODE INTERFACE ###
	// ###############################
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabDOMNodeImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	// Emulate keyboard strokes for given Tab and string
	virtual bool EmulateKeyboardStrokes(std::string text);
	virtual bool EmulateSelectAll();
	virtual bool EmulateEnterKey();

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
	virtual bool IsFaviconAlreadyAvailable(std::string img_url);

    // Setter of URL. Does not load it. Should be called by CefMediator only
	virtual void SetURL(std::string URL);

    // Setter for can go back / go forward
    virtual void SetCanGoBack(bool canGoBack) { _canGoBack = canGoBack;	}
    virtual void SetCanGoForward(bool canGoForward) { _canGoForward = canGoForward; }

    // Receive favicon bytes as char vector ordered in RGBA. Accepts also NULL for upData!
    virtual void ReceiveFaviconBytes(std::unique_ptr< std::vector<unsigned char> > upData, int width, int height);
    virtual void ResetFaviconBytes(); // TODO

    // Get weak pointer to texture of web view
    virtual std::weak_ptr<Texture> GetWebViewTexture() { return _upWebView->GetTexture(); }

    // Add, remove and update Tab's current DOMNodes
	virtual void AddDOMTextInput(int id);
	virtual void AddDOMLink(int id);
	virtual void AddDOMSelectField(int id);
	virtual void AddDOMOverflowElement(int id);
	virtual void AddDOMVideo(int id);
	virtual void AddDOMCheckbox(int id);

	virtual std::weak_ptr<DOMTextInput> GetDOMTextInput(int id);
	virtual std::weak_ptr<DOMLink> GetDOMLink(int id);
	virtual std::weak_ptr<DOMSelectField> GetDOMSelectField(int id);
	virtual std::weak_ptr<DOMOverflowElement> GetDOMOverflowElement(int id);
	virtual std::weak_ptr<DOMVideo> GetDOMVideo(int id);
	virtual std::weak_ptr<DOMCheckbox> GetDOMCheckbox(int id);

	virtual void RemoveDOMTextInput(int id);
	virtual void RemoveDOMLink(int id);
	virtual void RemoveDOMSelectField(int id);
	virtual void RemoveDOMOverflowElement(int id);
	virtual void RemoveDOMVideo(int id);
	virtual void RemoveDOMCheckbox(int id);
	virtual void ClearDOMNodes();

    // Receive callbacks from CefMediator upon scrolling offset changes
    virtual void SetScrollingOffset(double x, double y);

    // Getter for current zoom level of corresponding browser
    virtual double GetZoomLevel() const { return _zoomLevel; }

    // Set page resolution from Cef Mediator
    virtual void SetPageResolution(double width, double height);

    virtual void AddFixedElementsCoordinates(int id, std::vector<Rect> elements);
    virtual void RemoveFixedElement(int id);

    // Set Tab's title text
	virtual void SetTitle(std::string title);

	virtual void SetMetaKeywords(std::string content);

    // Add new Tab after that one
	virtual void AddTabAfter(std::string URL, CefRefPtr<CefRequestContext> request_context) { _pWeb->PushAddTabAfterJob(this, URL, request_context); }

	// Receive current loading status of each frame
	virtual void SetLoadingStatus(bool isLoading, bool isMainFrame);
	
	// Tell about JavaScript dialog
	virtual void RequestJSDialog(JavaScriptDialogType type, std::string message);

	// ###############################
	// ### TAB DEBUGGING INTERFACE ###
	// ###############################
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabDebuggingImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	// Size in pixels, coordinate system is WebViewPixel
	virtual void Debug_DrawRectangle(glm::vec2 coordinate, glm::vec2 size, glm::vec3 color) const;

	// Coordinate system is WebViewPixel
	virtual void Debug_DrawLine(glm::vec2 originCoordinate, glm::vec2 targetCoordinate, glm::vec3 color) const;

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
		virtual void selected(eyegui::Layout* pLayout, std::string id) {}

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
		virtual void selected(eyegui::Layout* pLayout, std::string id);

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

	// Enter video mode
	void EnterVideoMode(int id); // id of video node

	// Exit video mode
	void ExitVideoMode(bool immediately = false);

	// Start social record
	void StartSocialRecord(std::string URL, SocialPlatform platform);

	// End social record and send to database
	void EndSocialRecord();

    // Method to update and pipe accent color to eyeGUI
    void UpdateAccentColor(float tpf);

    // Pushes back click visualization which fades out. X and y are in pixels
    void PushBackClickVisualization(double x, double y);

	// Unique name for favicon which is stored in eyeGUI
	std::string GetFaviconIdentifier() const;

	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabDOMNodeImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	// Used by TabDOMInterface in order to be able to execute node functions in Javascript
	bool SendProcessMessageToRenderer(CefRefPtr<CefProcessMessage> msg);

	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	// >>> Implemented in TabDebuggingImpl.cpp >>>
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	// Initialize debugging overlay rendering
	void InitDebuggingOverlay();

	// Draw debugging overlay
	void DrawDebuggingOverlay() const;

	// ###############
	// ### MEMBERS ###
	// ###############

    // Current URL
    std::string _url = "";

    // Maps of triggers. Remember to add clearing in "ClearDOMNodes"
	std::map<int, std::unique_ptr<TextInputTrigger> >_textInputTriggers;
	std::map<int, std::unique_ptr<SelectFieldTrigger> >_selectFieldTriggers;
	std::map<int, std::unique_ptr<VideoModeTrigger> >_videoModeTriggers;
	
	// Collection of all triggers
	std::vector<Trigger*> _triggers;

	// Map nodeID to node itself, in order to access it when it has to be updated
	std::map<int, std::shared_ptr<DOMLink> > _TextLinkMap;
	std::map<int, std::shared_ptr<DOMTextInput> > _TextInputMap;
	std::map<int, std::shared_ptr<DOMSelectField> > _SelectFieldMap;
	std::map<int, std::shared_ptr<DOMOverflowElement> > _OverflowElementMap;
	std::map<int, std::shared_ptr<DOMVideo> > _VideoMap;
	std::map<int, std::shared_ptr<DOMCheckbox> > _CheckboxMap;

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
	eyegui::Layout* _pVideoModeLayout;
	eyegui::Layout* _pVideoModePauseOverlayLayout;
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

	// Bool to indicate whether tab is active
	bool _active = false;

    // Bool to indicate whether some pipeline is active
    bool _pipelineActive = false;

    // Automatic scrolling
    bool _autoScrolling = false;
    float _autoScrollingValue = 0; // [-1..1]

    // Gaze mouse
    bool _gazeMouse = false;

    // Level of zooming
    double _zoomLevel = 1;

    // Pointer to mediator
    Mediator* _pCefMediator;

	// RenderItems used for debug rendering. Initialized and used in TabDebuggingImpl.cpp
	std::unique_ptr<RenderItem> _upDebugLineQuad;
	std::unique_ptr<RenderItem> _upDebugFillQuad;
	std::unique_ptr<RenderItem> _upDebugLine;

	// Save some gaze input for debugging purposes
	std::deque<glm::vec2> _gazeDebuggingQueue;

    // Frame indices of scroll up and down overlays
    unsigned int _scrollUpProgressFrameIndex = 0;
    unsigned int _scrollDownProgressFrameIndex = 0;
    unsigned int _scrollUpSensorFrameIndex = 0;
    unsigned int _scrollDownSensorFrameIndex = 0;

    // Ids of elements in overlay (added / removed by triggers or actions)
    std::map<std::string, std::function<void(void)> > _overlayButtonDownCallbacks;
    std::map<std::string, std::function<void(void)> > _overlayButtonUpCallbacks;
	std::map<std::string, std::function<void(void)> > _overlayButtonSelectedCallbacks;
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

	// Current site's meta keywords
	std::string _metaKeywords;

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

	// Last time per frame
	float _lastTimePerFrame = 1.f; // initialize with something that is not zero, as may be used for divisions

	// Active social record
	std::shared_ptr<SocialRecord> _spSocialRecord = nullptr; // can be nullptr when currently not browsing a social page

	// Social record helpers
	std::string _prevURL = ""; // for determining URL change
	double _prevScrolling = 0; // for determining scrolling delta

	// Data transfer (indicates whether social record is allowed or not, also controls whether history is filled or not)
	bool _dataTransfer = false;

	// Time until next polling
	float _timeUntilPolling = 0.f;

	// Id of DOMVideo in map that is currently shown in video mode
	int _videoModeId = -1; // -1 is indicator that currently no video mode active

	// Marker for next received click to be triggered by user (used for social records)
	bool _userTriggeredClick = false;

	// Current history entry
	std::shared_ptr<HistoryManager::Page> _spHistoryPage = nullptr;

	// Polling partition index
	int _pollingPartitionIndex = 0;
};

#endif // TAB_H_
