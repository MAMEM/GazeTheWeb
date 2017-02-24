//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "URLInput.h"
#include "src/Global.h"
#include "src/Master.h"
#include "src/Utils/Helper.h"

// Include singleton for mailing to JavaScript
#include "src/Singletons/JSMailer.h"

URLInput::URLInput(Master* pMaster, BookmarkManager const * pBookmarkManager)
{
    // Fill members
    _pMaster = pMaster;
	_pBookmarkManager = pBookmarkManager;

    // Create layouts
    _pLayout = _pMaster->AddLayout("layouts/URLInput.xeyegui", EYEGUI_WEB_URL_INPUT_LAYER, false);
	_pBookmarksLayout = _pMaster->AddLayout("layouts/URLInputBookmarks.xeyegui", EYEGUI_WEB_URL_INPUT_LAYER, false);

    // Create listeners
    _spURLKeyboardListener = std::shared_ptr<URLKeyboardListener>(new URLKeyboardListener(this));
    eyegui::registerKeyboardListener(_pLayout, "keyboard", _spURLKeyboardListener);
    _spURLButtonListener = std::shared_ptr<URLButtonListener>(new URLButtonListener(this));
    eyegui::registerButtonListener(_pLayout, "close", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "bookmarks", _spURLButtonListener);
    eyegui::registerButtonListener(_pLayout, "delete", _spURLButtonListener);
    eyegui::registerButtonListener(_pLayout, "complete", _spURLButtonListener);
	eyegui::registerButtonListener(_pBookmarksLayout, "back", _spURLButtonListener);
}

URLInput::~URLInput()
{
    _pMaster->RemoveLayout(_pLayout);
}

bool URLInput::Update()
{
    return _finished;
}

void URLInput::Activate(int tabId)
{
    if (!_active)
    {
        // Make layout visible
        eyegui::setVisibilityOfLayout(_pLayout, true, true, true);

		// Make bookmarks layout invisble
		eyegui::setVisibilityOfLayout(_pBookmarksLayout, false, false, false);

        // Reset collected URL
        _collectedURL = u"";

        // Set initial content of text block TODO: localization
        eyegui::setContentOfTextBlock(_pLayout, "text_block", "URL will appear here");

        // Finished, not yet
        _finished = false;

        // Remember id of tab which url shall be changed
        _currentTabId = tabId;

        // Remember activation
        _active = true;

		// Get current bookmarks
		_bookmarks = _pBookmarkManager->GetSortedBookmarks();
    }
}

void URLInput::Deactivate()
{
    if (_active)
    {
        // Make layouts invisible
        eyegui::setVisibilityOfLayout(_pLayout, false, false, true);
		eyegui::setVisibilityOfLayout(_pBookmarksLayout, false, false, true);

        // Remember it
        _active = false;
    }
}

std::string URLInput::GetURL() const
{
	std::string url8;
	eyegui_helper::convertUTF16ToUTF8(_collectedURL, url8);
	return url8;
}

void URLInput::ShowBookmarks()
{
	// Prepare adding of bricks
	std::string brickId;
	std::map<std::string, std::string> idMapper;

	// Add brick to stack.
	// Necessary because layout cannot be reset and it is easier
	// to reset the bookmarks list by replacing the current stack
	// with an empty one from a brick
	eyegui::replaceElementWithBrick(
		_pBookmarksLayout,
		"bookmarks_flow", // same id used in brick, so no replacement necessary
		"bricks/URLInputBookmarksFlow.beyegui",
		false);

	// Set space of flow
	float space = ((float)(_bookmarks.size() + 1) / (float)URL_INPUT_BOOKMARKS_ROWS_ON_SCREEN); // size + 1 for title
	space = space < 1.f ? 1.f : space;
	eyegui::setSpaceOfFlow(_pBookmarksLayout, "bookmarks_flow", space);

	// Go over available bookmarks
	for (int i = 0; i < (int)_bookmarks.size(); i++)
	{
		// Brick id
		brickId = "bookmark_" + std::to_string(i);

		// URL id
		std::string URLId = "url_" + std::to_string(i);

		// Bookmark selection id
		std::string selectId = "select_" + std::to_string(i);

		// Id mapper
		idMapper.clear();
		idMapper.emplace("grid", brickId);
		idMapper.emplace("url", URLId);
		idMapper.emplace("select", selectId);

		// Add brick to stack, which was added by flow brick
		eyegui::addBrickToStack(
			_pBookmarksLayout,
			"bookmark_stack",
			"bricks/URLInputBookmark.beyegui",
			idMapper);

		// Set content of displayed URL
		eyegui::setContentOfTextBlock(_pBookmarksLayout, URLId, _bookmarks.at(i));

		// Register button listener for select buttons
		eyegui::registerButtonListener(_pBookmarksLayout, selectId, _spURLButtonListener);
	}

	// Make layout visible
	eyegui::setVisibilityOfLayout(_pBookmarksLayout, true, true, true);
}

void URLInput::URLKeyboardListener::keyPressed(eyegui::Layout* pLayout, std::string id, std::u16string value)
{
    _pURLInput->_collectedURL += value;
    eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "text_block", _pURLInput->_collectedURL + u"|");

	// Do logging about it
	JSMailer::instance().Send("keystroke");
}

void URLInput::URLButtonListener::down(eyegui::Layout* pLayout, std::string id)
{
	if (pLayout == _pURLInput->_pLayout)
	{
		// Standard layout
		if (id == "close")
		{
			// Reset URL input to empty to indicate abortion
			_pURLInput->_collectedURL = u"";
			_pURLInput->_finished = true;

			JSMailer::instance().Send("close");
			LabStreamMailer::instance().Send("Close URL input");
		}
		else if (id == "bookmarks")
		{
			// Show bookmarks
			_pURLInput->ShowBookmarks();
			JSMailer::instance().Send("bookmarks");
			LabStreamMailer::instance().Send("Display bookmarks");
		}
		else if (id == "delete")
		{
			if (_pURLInput->_collectedURL.size() > 0)
			{
				// Delete last letter
				_pURLInput->_collectedURL.pop_back();

				// Tell preview about it
				eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "text_block", _pURLInput->_collectedURL);

				// TODO: Displaying does not work for empty string aka last letter deleted
			}
		}
		else if (id == "complete")
		{
			_pURLInput->_finished = true;
			LabStreamMailer::instance().Send("URL input done");
		}
	}
	else
	{
		// Bookmarks layout
		if (id == "back")
		{
			eyegui::setVisibilityOfLayout(_pURLInput->_pBookmarksLayout, false, false, true);
			LabStreamMailer::instance().Send("Hide bookmarks");
		}
		else
		{
			// Split id by underscore
			std::string delimiter = "_";
			size_t pos = 0;

			// Check for keyword "select"
			if (((pos = id.find(delimiter)) != std::string::npos) && id.substr(0, pos) == "select")
			{
				// Extract number of bookmark which should be used
				std::string URL = _pURLInput->_bookmarks.at((int)(StringToFloat(id.substr(pos + 1, id.length() - 1))));
				std::u16string URL16;
				eyegui_helper::convertUTF8ToUTF16(URL, URL16);
				_pURLInput->_collectedURL = URL16;
				_pURLInput->_finished = true;
				LabStreamMailer::instance().Send("Open bookmark: " + URL);
			}

			JSMailer::instance().Send("open_bookmark");
		}
	}
}
