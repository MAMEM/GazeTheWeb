//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Web state which manages the tabs. Interactions are displayed on own layout.

#ifndef WEB_H_
#define WEB_H_

#include "src/State/State.h"
#include "src/State/Web/Tab/Tab.h"
#include "src/State/Web/Managers/BookmarkManager.h"
#include "src/State/Web/Managers/HistoryManager.h"
#include "src/State/Web/Screens/URLInput.h"
#include "src/State/Web/Screens/History.h"
#include "src/Input/VoiceInput.h"
#include <map>
#include <vector>
#include <memory>
#include <stack>
#include <regex>


// Forward declaration
class Mediator;

enum class WebPanelMode
{
	STANDARD, NO_DATA_TRANSFER
};

class Web : public State, public WebTabInterface
{
public:

    // Constructor
    Web(Master* pMaster, Mediator* pCefMediator, bool dataTransfer);

    // Destructor
    virtual ~Web();

    // Add tab and return id of it
	int AddTab(bool show = true);
    int AddTab(std::string URL, bool show = true, CefRefPtr<CefRequestContext> request_context = nullptr);

    // Add tab after another
    int AddTabAfter(Tab* other, std::string URL, bool show = true, CefRefPtr<CefRequestContext> request_context = nullptr);

    // Remove tab
    void RemoveTab(int id);

	// Remove all tabs
	void RemoveAllTabs();

    // Switch to tab. Returns whether successful
    bool SwitchToTab(int id);

    // Switch to tab by index. Returns whether successful
    bool SwitchToTabByIndex(int index);

    // Switch to next tab. Returns whether successful
    bool SwitchToNextTab();

    // Switch to previous tab. Returns whether successful
    bool SwitchToPreviousTab();

    // Opens URL in tab. Returns whether successful
    bool OpenURLInTab(int id, std::string URL);

	// Pushs back pointing evaluation pipeline in current tab
	void PushBackPointingEvaluationPipeline(PointingApproach approach);

	// Web panel mode
	void SetWebPanelMode(WebPanelMode mode);

	// Pause data transfer
	void SetDataTransfer(bool active);

	// Notify about click
	void NotifyClick(std::string tag, std::string id, float x, float y);

	// Set award
	void SetAward(Award award);

	// Demo mode reset (clear and delete history and bookmarks, reset tabs etc.)
	void DemoModeReset();

    // #############
    // ### STATE ###
    // #############

    // Update. Returns which state should be active in next time step
    virtual StateType Update(float tpf, const std::shared_ptr<const Input> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

	// #########################
	// ### WEB TAB INTERFACE ###
	// #########################

	// Add tab after that tab
    virtual void PushAddTabAfterJob(Tab* pCaller, std::string URL, CefRefPtr<CefRequestContext> request_context);

	// Update award
	virtual void PushUpdateAwardJob(Tab* pCaller, Award award);

    // Get own id in web. Returns -1 if not found
    virtual int GetIdOfTab(Tab const * pCaller) const;

	// Add history entry
	virtual std::shared_ptr<HistoryManager::Page> AddPageToHistory(std::string URL, std::string title);

private:

    // Jobs given by Tab over WebTabInterface
    class TabJob
    {
    public:

        // Constructor
        TabJob(Tab* pCaller);

        // Execute
        virtual void Execute(Web* pCallee) = 0;

    protected:

        // Members
        Tab* _pCaller;
    };

    class AddTabAfterJob : public TabJob
    {
    public:

        // Constructor
		AddTabAfterJob(Tab* pCaller, std::string URL, bool show, CefRefPtr<CefRequestContext> request_context) : TabJob(pCaller)
		{
			_URL = URL;
			_show = show;
			_request_context = request_context;
		}

        // Execute
        virtual void Execute(Web* pCallee);

    protected:

        // Members
        std::string _URL;
		bool _show;
		CefRefPtr<CefRequestContext> _request_context;
	};

	class UpdateAwardJob : public TabJob
	{
	public:

		// Constructor
		UpdateAwardJob(Tab* pCaller, Award award) : TabJob(pCaller)
		{
			_award = award;
		}

		// Execute
		virtual void Execute(Web* pCallee);

	protected:

		// Members
		Award _award;
	};

    // Give listener full access
    friend class WebButtonListener;

    // Listener for GUI
    class WebButtonListener: public eyegui::ButtonListener
    {
    public:

        WebButtonListener(Web* pWeb) { _pWeb = pWeb; }
        virtual void hit(eyegui::Layout* pLayout, std::string id) {}
        virtual void down(eyegui::Layout* pLayout, std::string id);
		virtual void up(eyegui::Layout* pLayout, std::string id);
		virtual void selected(eyegui::Layout* pLayout, std::string id) {}

    private:

        Web* _pWeb;
    };

    // Instance of listener
    std::shared_ptr<WebButtonListener> _spWebButtonListener;

    // Get index of Tab in order vector. Returns -1 if not found
    int GetIndexOfTabInOrderVector(int id) const;

    // Show tab overview
    void ShowTabOverview(bool show);

    // Update tab overview
    void UpdateTabOverview();

    // Calculate page cound for tab overview
    int CalculatePageCountOfTabOverview() const;

	// Update icon of tab overview
	void UpdateTabOverviewIcon();

	// Validate URL. Returns true if recognized as URL
	bool ValidateURL(const std::string& rURL) const;

	bool CreateBookmark();


    // Maps id to Tab
    std::map<int, std::unique_ptr<Tab> > _tabs;

    // Order of tabs, saving ids in vector
    std::vector<int> _tabIdOrder;

    // Current tab is indicated with index of vector (-1 means, that no tab is currently displayed)
    int _currentTabId = -1;

    // Layouts
    eyegui::Layout* _pWebLayout;
    eyegui::Layout* _pTabOverviewLayout;

    // Tab overview page [0..PageCount-1]
    int _tabOverviewPage = 0;

    // Pointer to mediator
    Mediator* _pCefMediator;

    // Bool to remind it should be switched to settings
    bool _goToSettings = false;

    // List of jobs which have to be executed
    std::deque<std::unique_ptr<TabJob> > _jobs;

	// Bookmark manager
	std::unique_ptr<BookmarkManager> _upBookmarkManager;

	// History manager
	std::unique_ptr<HistoryManager> _upHistoryManager;

	// History object
	std::unique_ptr<History> _upHistory;

	// URL input object
	std::unique_ptr<URLInput> _upURLInput;

	// Data transfer
	bool _dataTransfer = false;

	// Store which award the user currently has (in terms of persuasive design)
	Award _award = Award::BRONZE;

	// Regex for URL validation
	std::unique_ptr<std::regex> _upURLregex;
	const char* _pURLregexExpression =
		"(https?://)?"		// optional http or https
		"([\\da-z\\.-]+)"	// domain name (any number, dot and character from a to z)
		"\\."				// dot between name and domain
		"([a-z\\.]{2,6})"	// domain itself
		"([/\\w\\.:-]*)*"	// folder structure
		"/?";				// optional last dash
	std::unique_ptr<std::regex> _upIPregex;
	const char* _pIPregexExpression =
		"(https?://)?"		// optional http or https
		"(\\d{1,3}(\\.\\d{1,3}){3})" // ip address
		"([/\\w\\.:-]*)*"	// folder structure
		"/?";				// optional last dash
	std::unique_ptr<std::regex> _upFILEregex;
	const char* _pFILEregexExpression =
		"file:///"			// file prefix
		".*";				// followed by anything
};

#endif // WEB_H_
