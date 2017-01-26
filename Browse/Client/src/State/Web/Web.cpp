//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Web.h"
#include "src/State/Web/Tab/Tab.h"
#include "src/Master.h"
#include "src/Global.h"
#include "src/Utils/Helper.h"
#include "src/Utils/Texture.h"
#include "src/Utils/MakeUnique.h"
#include <algorithm>

// Include singleton for mailing to JavaScript
#include "src/Singletons/JSMailer.h"

Web::Web(Master* pMaster, Mediator* pCefMediator) : State(pMaster)
{
    // Save member
    _pCefMediator = pCefMediator;

	// Create bookmark manager
	_upBookmarkManager = std::unique_ptr<BookmarkManager>(new BookmarkManager(pMaster->GetUserDirectory()));

	// Create hisotry manager
	_upHistoryManager = std::unique_ptr<HistoryManager>(new HistoryManager(pMaster->GetUserDirectory()));

	// Create History
	_upHistory = std::unique_ptr<History>(new History(_pMaster, _upHistoryManager.get()));

	// Create URL input
	_upURLInput = std::unique_ptr<URLInput>(new URLInput(_pMaster, _upBookmarkManager.get()));

    // Create own layout
    _pWebLayout = _pMaster->AddLayout("layouts/Web.xeyegui", EYEGUI_WEB_LAYER, false);
    _pTabOverviewLayout = _pMaster->AddLayout("layouts/WebTabOverview.xeyegui", EYEGUI_WEB_LAYER, false);

    // Register listener which do not change over usage
    _spWebButtonListener = std::shared_ptr<WebButtonListener>(new WebButtonListener(this));
    eyegui::registerButtonListener(_pWebLayout, "tab_overview", _spWebButtonListener);
    eyegui::registerButtonListener(_pWebLayout, "settings", _spWebButtonListener);
    eyegui::registerButtonListener(_pWebLayout, "back", _spWebButtonListener);
    eyegui::registerButtonListener(_pWebLayout, "forward", _spWebButtonListener);
    eyegui::registerButtonListener(_pTabOverviewLayout, "close", _spWebButtonListener);
	eyegui::registerButtonListener(_pTabOverviewLayout, "history", _spWebButtonListener);
    eyegui::registerButtonListener(_pTabOverviewLayout, "back", _spWebButtonListener);
    eyegui::registerButtonListener(_pTabOverviewLayout, "forward", _spWebButtonListener);
    eyegui::registerButtonListener(_pTabOverviewLayout, "close_tab", _spWebButtonListener);
    eyegui::registerButtonListener(_pTabOverviewLayout, "reload_tab", _spWebButtonListener);
    eyegui::registerButtonListener(_pTabOverviewLayout, "edit_url", _spWebButtonListener);
	eyegui::registerButtonListener(_pTabOverviewLayout, "bookmark_tab", _spWebButtonListener);

	// Regular expression for URL validation
	_upURLregex = std::make_unique<std::regex>(
		_pURLregexExpression,
		std::regex_constants::icase);
	_upIPregex = std::make_unique<std::regex>(
		_pIPregexExpression,
		std::regex_constants::icase);
}

Web::~Web()
{
    // TODO: Delete layouts?
}

int Web::AddTab(std::string URL, bool show)
{
    // Go over existing pairs and determine first free id
    int id = 0;
    for(; id < (int)_tabs.size(); id++)
    {
        // Go over all entries and try id
        bool check = true;
        for(const auto& tabId : _tabs)
        {
            if(tabId.first == id)
            {
                check = false;
                break;
            }
        }

        // Id is free, use it
        if(check) { break; }
    }

    // Create tab
    std::unique_ptr<Tab> upTab =
        std::unique_ptr<Tab>(new Tab(_pMaster, _pCefMediator, this, URL));

    // Put tab in map
    _tabs.emplace(id, std::move(upTab));

    // Push back at order
    _tabIdOrder.push_back(id);

    // Decide currently displayed tab
    if(show || _currentTabId < 0)
    {
        // Switch to tab
        SwitchToTab(id);
    }

	// Update icon of tab overview button
	UpdateTabOverviewIcon();

    // Retun id of tab
    return id;
}

int Web::AddTabAfter(Tab *other, std::string URL, bool show)
{
    // Find other tab
    int otherId = -1;
    int i = 0;
    for(const auto& rPair : _tabs)
    {
        if(rPair.second.get() == other)
        {
            otherId = i;
            break;
        }
        i++;
    }

    // Add new tab
    int id = AddTab(URL, show);

    // If the other tab exists, move created one after that
    if(otherId >= 0)
    {
        // Find tab id of other in order
        int otherOrderIndex = -1;
        for(int j = 0; j < (int)_tabIdOrder.size(); j++) { if(_tabIdOrder.at(j) == otherId) { otherOrderIndex = j; } }

        // Find tab id of new in order
        int orderIndex = -1;
        for(int j = 0; j < (int)_tabIdOrder.size(); j++) { if(_tabIdOrder.at(j) == id) { orderIndex = j; } }

        // Create new order
        auto newTabIdOrder = std::vector<int>();
        newTabIdOrder.reserve(_tabIdOrder.size());

        if(otherOrderIndex >= 0 && orderIndex >= 0)
        {
            // Add all tabs including other to order
            int oldIndex = 0;
            for(int j = 0; j <= otherOrderIndex; j++)
            {
                int currentId = _tabIdOrder.at(j);
                if(currentId != orderIndex)
                {
                    // Do not push back the new tab's id
                    newTabIdOrder.push_back(currentId);
                    oldIndex++;
                }
            }

            // Add new tab's id
            newTabIdOrder.push_back(orderIndex);

            // Add tail
            for(; oldIndex < (int)_tabIdOrder.size(); oldIndex++)
            {
                int currentId = _tabIdOrder.at(oldIndex);
                if(currentId != orderIndex)
                {
                    // Do not push back the new tab's id
                    newTabIdOrder.push_back(currentId);
                }
            }

            // Remember the vector
            _tabIdOrder = newTabIdOrder;
        }
    }

    return id;
}

void Web::RemoveTab(int id)
{
    // Verify that id exists
    auto iter = _tabs.find(id);
	if (iter != _tabs.end())
	{
		// Remove from order vector
		_tabIdOrder.erase(std::remove(_tabIdOrder.begin(), _tabIdOrder.end(), id), _tabIdOrder.end());

		// Check whether it is current tab
		if (id == _currentTabId)
		{
			// Try to switch to next tab
			if (!SwitchToNextTab())
			{
				// No next was available, try first in order
				if (_tabIdOrder.empty() || !SwitchToTab(_tabIdOrder.at(0)))
				{
					// Nothing helps, do not display anything
					_currentTabId = -1;
				}
			}
		}

		// Deactivate and remove from map
		_tabs.at(id)->Deactivate(); // should be already done but second time should not hurt
		_tabs.erase(id);

		// Update icon of tab overview button
		UpdateTabOverviewIcon();
	}
}

void Web::RemoveAllTabs()
{
	// Push back ids of tabs
	std::vector<int> ids(_tabs.size());
	for (const auto& tab : _tabs)
	{
		ids.push_back(tab.first);
	}

	// Delegate work to remove tab method
	for (int i = 0; i < (int)ids.size(); i++)
	{
		RemoveTab(i);
	}
}

bool Web::SwitchToTab(int id)
{
	bool success = false;

    // Simple case
    if(id == _currentTabId)
    {
        // Success!
		success = true;
    }

    // Verify that id exists
    auto iter = _tabs.find(id);
    if(iter != _tabs.end())
    {
        // Deactivate old tab
        if(_currentTabId >= 0 && _active)
        {
            // No check necessary
            _tabs.at(_currentTabId)->Deactivate();
        }

        // Set new tab as current
        _currentTabId = id;

        // Activate tab
        if(_active)
        {
            _tabs.at(_currentTabId)->Activate();
        }

		success = true;
    }

	// Tell Mediator and return
	if (_currentTabId >= 0)
	{
		// Set active Tab in Mediator
		_pCefMediator->SetActiveTab(_tabs.at(_currentTabId).get()); 
	}
    return success;
}

bool Web::SwitchToTabByIndex(int index)
{
    if(index >= 0 && index < (int)_tabIdOrder.size())
    {
        return SwitchToTab(_tabIdOrder[index]);
    }
    return false;
}

bool Web::SwitchToNextTab()
{
    // Get index of current tab in order vector
    int index = GetIndexOfTabInOrderVector(_currentTabId);

    // Switch to next
    if(index >= 0)
    {
        int nextIndex = index + 1;
        if(nextIndex >= 0 && nextIndex < (int)_tabIdOrder.size())
        {
            return SwitchToTab(_tabIdOrder[nextIndex]);
        }
    }

    return false;
}

bool Web::SwitchToPreviousTab()
{
    // Get index of current tab in order vector
    int index = GetIndexOfTabInOrderVector(_currentTabId);

    // Switch to next
    if(index >= 0)
    {
        int previousIndex = index - 1;
        if(previousIndex >= 0 && previousIndex < (int)_tabIdOrder.size())
        {
            return SwitchToTab(_tabIdOrder[previousIndex]);
        }
    }

    return false;
}

bool Web::OpenURLInTab(int id, std::string URL)
{
    // Verify that id exists
    auto iter = _tabs.find(id);
    if (iter != _tabs.end())
    {
        // Set URL
        iter->second->OpenURL(URL);
        return true;
    }

    return false;
}

StateType Web::Update(float tpf, Input& rInput)
{
    // Process jobs first
    while(!_jobs.empty())
    {
        auto upJob = std::move(_jobs.top());
        _jobs.pop();
        upJob->Execute(this);
    }

	// History
	if (_upHistory->IsActive())
	{
		// Update it and wait for it to finish
		if (_upHistory->Update())
		{
			// Update it and wait for finish of URL input
			if (_upHistory->Update())
			{
				// Get input and decide
				std::string URL = _upHistory->GetURL();
				if (!URL.empty())
				{
					int tabId = _upHistory->GetCurrentTabId();

					// Check whether tab id is valid
					auto iter = _tabs.find(tabId);
					if (iter != _tabs.end())
					{
						// Open URL in current tab
						_tabs.at(tabId)->OpenURL(URL);
					}
					else
					{
						// Since there is no tab, create one
						AddTab(URL, true);
					}
				}
			}

			// Input is done, deactivate it
			_upHistory->Deactivate();
		}
	}

    // URL input
    if (_upURLInput->IsActive())
    {
        // Update it and wait for finish of URL input
        if (_upURLInput->Update())
        {
            // Get input and decide
            std::string URL = _upURLInput->GetURL();
            if (!URL.empty())
            {
				// Validate URL
				if (!ValidateURL(URL))
				{
					URL = SEARCH_PREFIX + URL;
				}

				// Fetch tab id from URL input object
                int tabId = _upURLInput->GetCurrentTabId();

                // Check whether tab id is valid
                auto iter = _tabs.find(tabId);
                if (iter != _tabs.end())
                {
                    // Open URL in current tab
                    _tabs.at(tabId)->OpenURL(URL);
                }
                else
                {
                    // Since there is no tab, create one (just fallback...)
                    AddTab(URL, true);
                }
            }

            // Input is done, deactivate it
            _upURLInput->Deactivate();
        }
    }

    // Only do it if there is some tab to update
    if(_currentTabId >= 0)
    {
        // Check whether tab can go back or forward and tell it eyeGUI
        eyegui::setElementActivity(_pWebLayout, "back", _tabs.at(_currentTabId)->CanGoBack(), true);
        eyegui::setElementActivity(_pWebLayout, "forward", _tabs.at(_currentTabId)->CanGoForward(), true);

        _tabs.at(_currentTabId)->Update(tpf, rInput);
    }

    // Decide what to do next
    if (_goToSettings)
    {
        return StateType::SETTINGS;
    }
    else
    {
        return StateType::WEB;
    }

}

void Web::Draw() const
{
    // Only do it if there is some tab to draw
    if(_currentTabId >= 0)
    {
        _tabs.at(_currentTabId)->Draw();
    }
}

void Web::Activate()
{
    // Super
    State::Activate();

    // Layout
    eyegui::setVisibilityOfLayout(_pWebLayout, true, true, false);

    // Reset stuff
    _goToSettings = false;

    if(_currentTabId >= 0)
    {
        _tabs.at(_currentTabId)->Activate();
    }
}

void Web::Deactivate()
{
    // Super
    State::Deactivate();

    // Layout
    eyegui::setVisibilityOfLayout(_pWebLayout, false, true, false);

    if(_currentTabId >= 0)
    {
        _tabs.at(_currentTabId)->Deactivate();
    }

    ShowTabOverview(false);
}

void Web::PushAddTabAfterJob(Tab* pCaller, std::string URL)
{
    _jobs.push(std::unique_ptr<TabJob>(new AddTabAfterJob(pCaller, URL, true)));
}

void Web::PushAddPageToHistoryJob(Tab* pCaller, HistoryManager::Page page)
{
	_jobs.push(std::unique_ptr<TabJob>(new AddPageToHistoryJob(pCaller, page)));
}

int Web::GetIdOfTab(Tab const * pCaller) const
{
    // Go through map and find tab
    for(const auto& rPair : _tabs)
    {
        // Check whether tab is the same
        if(rPair.second.get() == pCaller)
        {
            return rPair.first;
        }
    }

    return -1;
}

int Web::GetIndexOfTabInOrderVector(int id) const
{
    // Search tab in order
    bool check = false;
    int i = 0;
    for(; i < (int)_tabIdOrder.size(); i++)
    {
        if(_tabIdOrder.at(i) == id)
        {
            check = true;
            break;
        }
    }

    // Return result
    if(check)
    {
        return i;
    }
    else
    {
        return -1;
    }
}

void Web::ShowTabOverview(bool show)
{
    if(show)
    {
        // Set visibility
        eyegui::setVisibilityOfLayout(_pTabOverviewLayout, true, true, true);

        // Update tab overview before displaying
        UpdateTabOverview();
    }
    else
    {
        // Set visibility
        eyegui::setVisibilityOfLayout(_pTabOverviewLayout, false, false, true);
    }
}

void Web::UpdateTabOverview()
{
    // Reserve some variables
    std::string brickId;
    std::map<std::string, std::string> idMapper;

    // Collect open tabs on that page
    int slotOffset = SLOTS_PER_TAB_OVERVIEW_PAGE * _tabOverviewPage;
    int slotsOnPage = std::min((int)_tabs.size() - slotOffset, SLOTS_PER_TAB_OVERVIEW_PAGE);

    // Iterate over slots in brick
    for(int i = 0; i < slotsOnPage; i++)
    {
        // Index of tab
        int indexOfTab = i + slotOffset;

        // Get id of tab
        int tabId = _tabIdOrder.at(indexOfTab);

        // Brick id
        brickId = "tab_" + std::to_string(i);
        std::string buttonId = "tab_button_" + std::to_string(i);
        std::string textblockId = "tab_textblock_" + std::to_string(i);

        // Id mapper
        idMapper.clear();
        idMapper.emplace("grid", brickId);
        idMapper.emplace("button", buttonId);
        idMapper.emplace("textblock", textblockId);

        // Put brick into layout
        eyegui::replaceElementWithBrick(
            _pTabOverviewLayout,
            brickId,
            "bricks/WebTabPreview.beyegui",
            idMapper,
            true);

        // Register listener
        eyegui::registerButtonListener(_pTabOverviewLayout, buttonId, _spWebButtonListener);

        // Tell textblock the URL (but shorter version)
        std::string shortURL = ShortenURL(_tabs.at(tabId)->GetURL());
        eyegui::setContentOfTextBlock(_pTabOverviewLayout, textblockId, shortURL);

        // Set webpage rendering as icon of button
        auto wpTexture = _tabs.at(tabId)->GetWebViewTexture();
        if(auto spTexture = wpTexture.lock())
        {
            // Fetch pixel data of tab in higher mip map level
            std::vector<unsigned char> tabPreviewData;
            int tabPreviewWidth;
            int tabPreviewHeight;
            if (spTexture->GetPixelsFromMipMap(
                WEB_TAB_OVERVIEW_MINI_PREVIEW_MIP_MAP_LEVEL,
                tabPreviewWidth,
                tabPreviewHeight,
                tabPreviewData))
            {
                // Pipe it to eyeGUI
                eyegui::setIconOfIconElement(
                    _pTabOverviewLayout,
                    buttonId,
                    buttonId + "_preview",
                    tabPreviewWidth,
                    tabPreviewHeight,
                    eyegui::ColorFormat::RGBA,
                    tabPreviewData.data(),
                    true);
            }
        }

		// Styling
        if (tabId == _currentTabId)
        {
			_pMaster->SetStylePropertyValue("tab_preview_" + std::to_string(i), eyegui::StylePropertyVec4::BackgroundColor, "0x50BAA6FF");
        }
        else
        {
			_pMaster->SetStylePropertyValue("tab_preview_" + std::to_string(i), eyegui::StylePropertyVec4::BackgroundColor, "0x607d8bFF");
        }
    }

    // Fill the rest with empty blanks before adding new tab button (otherwise there would be two of them for a moment and id would conflict)
    for (int i = slotsOnPage + 1; i < SLOTS_PER_TAB_OVERVIEW_PAGE; i++)
    {
        // Brick id
        brickId = "tab_" + std::to_string(i);

        // Id mapper
        idMapper.clear();
        idMapper.emplace("blank", brickId);

        // Put brick into layout
        eyegui::replaceElementWithBrick(
            _pTabOverviewLayout,
            brickId,
            "bricks/WebTabEmptyPreview.beyegui",
            idMapper,
            true);
    }

    // Add "new" button to last position if space left
    if (slotsOnPage < SLOTS_PER_TAB_OVERVIEW_PAGE)
    {
        // Brick id
        brickId = "tab_" + std::to_string(slotsOnPage);

        // Id mapper
        idMapper.clear();
        idMapper.emplace("stack", brickId);

        // Put brick into layout
        eyegui::replaceElementWithBrick(
            _pTabOverviewLayout,
            brickId,
            "bricks/WebTabNew.beyegui",
            idMapper,
            true);

        // Register listener
        eyegui::registerButtonListener(_pTabOverviewLayout, "new_tab", _spWebButtonListener);
    }

    // Get count of pages
    int pageCount = CalculatePageCountOfTabOverview();

    // Set page count in textblock (no more in layout)
    // eyegui::setContentOfTextBlock(_pTabOverviewLayout, "page_count", std::to_string(_tabOverviewPage + 1) + "/" + std::to_string(pageCount));

    // Activate or deactivate back and forward button
    eyegui::setElementActivity(_pTabOverviewLayout, "back", _tabOverviewPage > 0, true);
    eyegui::setElementActivity(_pTabOverviewLayout, "forward", _tabOverviewPage < (pageCount - 1), true);

    // Current tab view
    if (_currentTabId >= 0)
    {
        // Show URL
        eyegui::setContentOfTextBlock(_pTabOverviewLayout, "url", _tabs.at(_currentTabId)->GetURL());

        // Show current tab's page
        auto wpTexture = _tabs.at(_currentTabId)->GetWebViewTexture();
        if (auto spTexture = wpTexture.lock())
        {
            // Fetch pixel data of tab in higher mip map level
            std::vector<unsigned char> tabPreviewData;
            int tabPreviewWidth;
            int tabPreviewHeight;
            if (spTexture->GetPixelsFromMipMap(
                WEB_TAB_OVERVIEW_PREVIEW_MIP_MAP_LEVEL,
                tabPreviewWidth,
                tabPreviewHeight,
                tabPreviewData))
            {
                // Pipe it to eyeGUI
                eyegui::setImageOfPicture(
                    _pTabOverviewLayout,
                    "preview",
                    "current_tab_preview",
                    tabPreviewWidth,
                    tabPreviewHeight,
                    eyegui::ColorFormat::RGBA,
                    tabPreviewData.data(),
                    true);
            }
        }

        // Activate buttons
        eyegui::setElementActivity(_pTabOverviewLayout, "edit_url", true, true);
		eyegui::setElementActivity(_pTabOverviewLayout, "bookmark_tab", true, true);
        eyegui::setElementActivity(_pTabOverviewLayout, "reload_tab", true, true);
        eyegui::setElementActivity(_pTabOverviewLayout, "close_tab", true, true);

		// Set icon of bookmark button
		if (_upBookmarkManager->IsBookmark(_tabs.at(_currentTabId)->GetURL()))
		{
			eyegui::setIconOfIconElement(_pTabOverviewLayout, "bookmark_tab", "icons/BookmarkTab_true.png");
		}
		else
		{
			eyegui::setIconOfIconElement(_pTabOverviewLayout, "bookmark_tab", "icons/BookmarkTab_false.png");
		}
    }
    else
    {
        // Show no URL
        eyegui::setContentOfTextBlock(_pTabOverviewLayout, "url", " ");

        // Show placeholder in preview
		eyegui::replaceElementWithBrick(_pTabOverviewLayout, "preview", "bricks/Nothing.beyegui", true);

		// Reset icon of bookmark tab button
		eyegui::setIconOfIconElement(_pTabOverviewLayout, "bookmark_tab", "icons/BookmarkTab_false.png");

        // Deactivate buttons
		eyegui::setElementActivity(_pTabOverviewLayout, "edit_url", false, true);
		eyegui::setElementActivity(_pTabOverviewLayout, "bookmark_tab", false, true);
        eyegui::setElementActivity(_pTabOverviewLayout, "reload_tab", false, true);
        eyegui::setElementActivity(_pTabOverviewLayout, "close_tab", false, true);
    }
}

int Web::CalculatePageCountOfTabOverview() const
{
    return ((int)_tabs.size() / SLOTS_PER_TAB_OVERVIEW_PAGE) + 1;
}

void Web::UpdateTabOverviewIcon()
{
	unsigned int tabCount = _tabs.size();
	std::string iconFilepath = "";
	if (tabCount < 10)
	{
		iconFilepath = "icons/TabOverview_" + std::to_string(tabCount) + ".png";
	}
	else
	{
		iconFilepath = "icons/TabOverview_9+.png";
	}
	eyegui::setIconOfIconElement(_pWebLayout, "tab_overview", iconFilepath);
}

bool Web::ValidateURL(const std::string& rURL) const
{
	bool valid = false;

	// Check for URL
	try
	{
		valid |= std::regex_match(rURL, *(_upURLregex.get()));
	}
	catch (...) { valid = true; } // in case of failed validation, assume it is a URL since it seems to be complicated

	// Check for IP
	try
	{
		valid |= std::regex_match(rURL, *(_upIPregex.get()));
	}
	catch (...) { valid = true; } // in case of failed validation, assume it is a IP since it seems to be complicated
	
	// Return result
	return valid;
}

Web::TabJob::TabJob(Tab* pCaller)
{
    _pCaller = pCaller;
}

void Web::AddTabAfterJob::Execute(Web* pCallee)
{
	// Add tab after caller
    pCallee->AddTabAfter(_pCaller, _URL, _show);

	// Flash tab overview button to indicate, that new tab was created by application
	eyegui::flash(pCallee->_pWebLayout, "tab_overview");
}

void Web::AddPageToHistoryJob::Execute(Web* pCallee)
{
	// Add page to history
	pCallee->_upHistoryManager->AddPage(_page);
}

void Web::WebButtonListener::down(eyegui::Layout* pLayout, std::string id)
{
    if(pLayout == _pWeb->_pWebLayout)
    {
        // ### Web layout ###
        if (id == "tab_overview")
        {
            _pWeb->ShowTabOverview(true);
			JSMailer::instance().Send("tabs");
        }
        else if (id == "settings")
        {
            _pWeb->_goToSettings = true;
			JSMailer::instance().Send("settings");
        }
        else if (id == "back")
        {
            int tabId = _pWeb->_currentTabId;
            if (tabId >= 0)
            {
                _pWeb->_tabs[tabId]->GoBack();
            }
        }
        else if (id == "forward")
        {
            int tabId = _pWeb->_currentTabId;
            if (tabId >= 0)
            {
                _pWeb->_tabs[tabId]->GoForward();
            }
        }
    }
    else
    {
        // ### Tab overview layout ###
        if(id == "close")
        {
            _pWeb->ShowTabOverview(false);
			JSMailer::instance().Send("close");
        }
		else if (id == "history")
		{
			_pWeb->ShowTabOverview(false);
			_pWeb->_upHistory->Activate(_pWeb->_currentTabId);
		}
        else if (id == "edit_url")
        {
            _pWeb->ShowTabOverview(false);
            _pWeb->_upURLInput->Activate(_pWeb->_currentTabId);
			JSMailer::instance().Send("edit");
        }
		else if (id == "bookmark_tab")
		{
			int currentTab = _pWeb->_currentTabId;
			if (currentTab >= 0)
			{
				// Add as bookmark
				bool success = _pWeb->_upBookmarkManager->AddBookmark(_pWeb->_tabs.at(currentTab)->GetURL());

				// Display it on icon. Even if not successful, because that means it was already a bookmark
				eyegui::setIconOfIconElement(_pWeb->_pTabOverviewLayout, "bookmark_tab", "icons/BookmarkTab_true.png");

				// Display notification
				if (success)
				{
					_pWeb->_pMaster->PushNotificationByKey("notification:bookmark_added_success");
				}
				else
				{
					_pWeb->_pMaster->PushNotificationByKey("notification:bookmark_added_existing");
				}
			}

			JSMailer::instance().Send("bookmark_add");
		}
        else if (id == "reload_tab")
        {
            int tabId = _pWeb->_currentTabId;
            if (tabId >= 0)
            {
                _pWeb->_tabs[tabId]->Reload();
                _pWeb->ShowTabOverview(false);
            }
        }
        else if (id == "close_tab")
        {
            if (!_pWeb->_tabs.empty())
            {
                _pWeb->RemoveTab(_pWeb->_currentTabId);
                _pWeb->UpdateTabOverview();
            }
        }
        else if (id == "back")
        {
            // Go to back to previous page but not under zero
            _pWeb->_tabOverviewPage--;
            _pWeb->_tabOverviewPage = std::max(0, _pWeb->_tabOverviewPage);
            _pWeb->UpdateTabOverview();
        }
        else if (id == "forward")
        {
            // Go forward to next page but not over maximum count
            _pWeb->_tabOverviewPage++;
            _pWeb->_tabOverviewPage = std::min(_pWeb->CalculatePageCountOfTabOverview() - 1, _pWeb->_tabOverviewPage);
            _pWeb->UpdateTabOverview();
        }
        else if(id == "new_tab")
        {
            // Add tab
            int tabId = _pWeb->AddTab(BLANK_PAGE_URL, true);

            // Close tab overview
            _pWeb->ShowTabOverview(false);

            // Open URLInput to type in URL which should be loaded in new tab
            _pWeb->_upURLInput->Activate(tabId);

			JSMailer::instance().Send("new_tab");
        }
        else if(id == "tab_button_0")
        {
            if(_pWeb->SwitchToTabByIndex(0 + (_pWeb->_tabOverviewPage * SLOTS_PER_TAB_OVERVIEW_PAGE)))
            {
                _pWeb->ShowTabOverview(false);
				JSMailer::instance().Send("tab0");
            }
        }
        else if(id == "tab_button_1")
        {
            if(_pWeb->SwitchToTabByIndex(1 + (_pWeb->_tabOverviewPage * SLOTS_PER_TAB_OVERVIEW_PAGE)))
            {
                _pWeb->ShowTabOverview(false);
            }
        }
        else if(id == "tab_button_2")
        {
            if(_pWeb->SwitchToTabByIndex(2 + (_pWeb->_tabOverviewPage * SLOTS_PER_TAB_OVERVIEW_PAGE)))
            {
                _pWeb->ShowTabOverview(false);
            }
        }
        else if (id == "tab_button_3")
        {
            if(_pWeb->SwitchToTabByIndex(3 + (_pWeb->_tabOverviewPage * SLOTS_PER_TAB_OVERVIEW_PAGE)))
            {
                _pWeb->ShowTabOverview(false);
            }
        }
        else if (id == "tab_button_4")
        {
            if(_pWeb->SwitchToTabByIndex(4 + (_pWeb->_tabOverviewPage * SLOTS_PER_TAB_OVERVIEW_PAGE)))
            {
                _pWeb->ShowTabOverview(false);
            }
        }
    }
}
