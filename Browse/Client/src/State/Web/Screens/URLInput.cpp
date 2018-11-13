//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "URLInput.h"
#include "src/Global.h"
#include "src/Master/Master.h"
#include "src/Utils/Helper.h"

// Include singleton for mailing to JavaScript
#include "src/Singletons/JSMailer.h"

URLInput::URLInput(Master* pMaster, BookmarkManager* pBookmarkManager)
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
	eyegui::registerButtonListener(_pLayout, "com", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "org", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "net", _spURLButtonListener);
	// eyegui::registerButtonListener(_pLayout, "eu", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "gr", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "co_il", _spURLButtonListener);
	// eyegui::registerButtonListener(_pLayout, "de", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "space", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "extra_keys", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "layout", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "layout_us_english", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "layout_germany_german", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "layout_israel_hebrew", _spURLButtonListener);
	eyegui::registerButtonListener(_pLayout, "layout_greece_greek", _spURLButtonListener);
	eyegui::registerButtonListener(_pBookmarksLayout, "back", _spURLButtonListener);
}

URLInput::~URLInput()
{
    _pMaster->RemoveLayout(_pLayout);
}

URLInput::Status URLInput::Update()
{
    return _status;
}

void URLInput::Activate(int tabId)
{
    if (!_active)
    {
        // Make layout visible
        eyegui::setVisibilityOfLayout(_pLayout, true, true, true);

		// Make bookmarks layout invisble
		eyegui::setVisibilityOfLayout(_pBookmarksLayout, false, false, false);

		// Set keymap
		eyegui::setKeymapOfKeyboard(_pLayout, "keyboard", 0);

        // Reset collected URL
        _collectedURL = u"";

		// Set display
		eyegui::setContentOfTextBlock(_pLayout, "url_display", u"|");

        // Reset extra key button
		eyegui::buttonUp(_pLayout, "extra_keys", true);

        // Finished, not yet
		_status = Status::PENDING;

        // Remember id of tab which url shall be changed
        _currentTabId = tabId;

        // Remember activation
        _active = true;

		// Reset layout button related stuff
		eyegui::buttonUp(_pLayout, "layout", true);
		eyegui::setElementActivity(_pLayout, "keyboard", true);
		eyegui::setElementActivity(_pLayout, "bookmarks", true);
		eyegui::setElementActivity(_pLayout, "complete", true);
		eyegui::setElementActivity(_pLayout, "delete", true);
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
	// Get current bookmarks
	_bookmarks = _pBookmarkManager->GetSortedBookmarks();

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

		// Bookmark remove id
		std::string removeId = "remove_" + std::to_string(i);

		// Id mapper
		idMapper.clear();
		idMapper.emplace("grid", brickId);
		idMapper.emplace("url", URLId);
		idMapper.emplace("select", selectId);
		idMapper.emplace("remove", removeId);

		// Add brick to stack, which was added by flow brick
		eyegui::addBrickToStack(
			_pBookmarksLayout,
			"bookmark_stack",
			"bricks/URLInputBookmark.beyegui",
			idMapper);

		// Set content of displayed URL
		eyegui::setContentOfTextBlock(_pBookmarksLayout, URLId, _bookmarks.at(i));

		// Register button listener for remove and select button
		eyegui::registerButtonListener(_pBookmarksLayout, removeId, _spURLButtonListener);
		eyegui::registerButtonListener(_pBookmarksLayout, selectId, _spURLButtonListener);
	}

	// Make layout visible
	eyegui::setVisibilityOfLayout(_pBookmarksLayout, true, true, true);
}

void URLInput::URLKeyboardListener::keyPressed(eyegui::Layout* pLayout, std::string id, std::u16string value)
{
    _pURLInput->_collectedURL += value;
    eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");

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
			_pURLInput->_status = URLInput::Status::MANUAL_URL;

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
				eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
			}
		}
		else if (id == "complete")
		{
			_pURLInput->_status = URLInput::Status::MANUAL_URL;
			nlohmann::json record = { { "charCount", _pURLInput->_collectedURL.length() } };
			_pURLInput->_pMaster->SimplePushBackAsyncJob(FirebaseIntegerKey::GENERAL_URL_INPUT_COUNT, FirebaseJSONKey::GENERAL_URL_INPUT, record);
			LabStreamMailer::instance().Send("URL input done");
		}
		else if (id == "com")
		{
			_pURLInput->_collectedURL += u".com";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}
		else if (id == "org")
		{
			_pURLInput->_collectedURL += u".org";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}
		else if (id == "net")
		{
			_pURLInput->_collectedURL += u".net";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}
		/*else if (id == "eu")
		{
			_pURLInput->_collectedURL += u".eu";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}*/
		else if (id == "gr")
		{
			_pURLInput->_collectedURL += u".gr";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}
		else if (id == "co_il")
		{
			_pURLInput->_collectedURL += u".co.il";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}
		/*else if (id == "de")
		{
			_pURLInput->_collectedURL += u".de";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}*/
		else if (id == "space")
		{
			_pURLInput->_collectedURL += u" ";
			eyegui::setContentOfTextBlock(_pURLInput->_pLayout, "url_display", _pURLInput->_collectedURL + u"|");
		}
		else if (id == "extra_keys")
		{
			eyegui::setKeymapOfKeyboard(_pURLInput->_pLayout, "keyboard", 1);
		}
		else if (id == "layout")
		{
			eyegui::setElementActivity(_pURLInput->_pLayout, "keyboard", false);
			eyegui::setElementActivity(_pURLInput->_pLayout, "bookmarks", false);
			eyegui::setElementActivity(_pURLInput->_pLayout, "complete", false);
			eyegui::setElementActivity(_pURLInput->_pLayout, "delete", false);
		}
		else if (id == "layout_us_english")
		{
			_pURLInput->_pMaster->SetKeyboardLayout(eyegui::KeyboardLayout::US_ENGLISH);
			eyegui::buttonUp(_pURLInput->_pLayout, "layout");
			eyegui::buttonUp(_pURLInput->_pLayout, "extra_keys");
		}
		else if (id == "layout_germany_german")
		{
			_pURLInput->_pMaster->SetKeyboardLayout(eyegui::KeyboardLayout::GERMANY_GERMAN);
			eyegui::buttonUp(_pURLInput->_pLayout, "layout");
			eyegui::buttonUp(_pURLInput->_pLayout, "extra_keys");
		}
		else if (id == "layout_israel_hebrew")
		{
			_pURLInput->_pMaster->SetKeyboardLayout(eyegui::KeyboardLayout::ISRAEL_HEBREW);
			eyegui::buttonUp(_pURLInput->_pLayout, "layout");
			eyegui::buttonUp(_pURLInput->_pLayout, "extra_keys");
		}
		else if (id == "layout_greece_greek")
		{
			_pURLInput->_pMaster->SetKeyboardLayout(eyegui::KeyboardLayout::GREECE_GREEK);
			eyegui::buttonUp(_pURLInput->_pLayout, "layout");
			eyegui::buttonUp(_pURLInput->_pLayout, "extra_keys");
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
			if ((pos = id.find(delimiter)) != std::string::npos)
			{
				// Check for keyword "remove"
				if (id.substr(0, pos) == "remove")
				{
					// Extract URL of chosen bookmark from local cache
					std::string URL = _pURLInput->_bookmarks.at((int)(StringToFloat(id.substr(pos + 1, id.length() - 1))));

					// Remove bookmark from manager
					_pURLInput->_pBookmarkManager->RemoveBookmark(URL); // remove the bookmark

					// Log it
					nlohmann::json record = { { "url", URL } };
					_pURLInput->_pMaster->SimplePushBackAsyncJob(FirebaseIntegerKey::GENERAL_BOOKMARK_REMOVAL_COUNT, FirebaseJSONKey::GENERAL_BOOKMARK_REMOVAL, record);
					LabStreamMailer::instance().Send("Remove bookmark: " + URL);

					// Reset screen and notify user
					eyegui::setVisibilityOfLayout(_pURLInput->_pBookmarksLayout, false, false, true);
					_pURLInput->_pMaster->PushNotificationByKey("notification:bookmark_removed", MasterNotificationInterface::Type::SUCCESS, false);
				}
				else if (id.substr(0, pos) == "select") // "select"
				{
					// Extract number of bookmark which should be used
					std::string URL = _pURLInput->_bookmarks.at((int)(StringToFloat(id.substr(pos + 1, id.length() - 1))));
					std::u16string URL16;
					eyegui_helper::convertUTF8ToUTF16(URL, URL16);
					_pURLInput->_collectedURL = URL16;
					_pURLInput->_status = URLInput::Status::BOOKMARK_URL;
					nlohmann::json record = { { "url", URL } };
					_pURLInput->_pMaster->SimplePushBackAsyncJob(FirebaseIntegerKey::GENERAL_BOOKMARK_USAGE_COUNT, FirebaseJSONKey::GENERAL_BOOKMARK_USAGE, record);
					LabStreamMailer::instance().Send("Open bookmark: " + URL);
				}
			}

			JSMailer::instance().Send("open_bookmark");
		}
	}
}

void URLInput::URLButtonListener::up(eyegui::Layout* pLayout, std::string id)
{
	if (pLayout == _pURLInput->_pLayout)
	{
		if (id == "layout")
		{
			eyegui::setElementActivity(_pURLInput->_pLayout, "keyboard", true);
			eyegui::setElementActivity(_pURLInput->_pLayout, "bookmarks", true);
			eyegui::setElementActivity(_pURLInput->_pLayout, "complete", true);
			eyegui::setElementActivity(_pURLInput->_pLayout, "delete", true);
		}
		else if (id == "extra_keys")
		{
			eyegui::setKeymapOfKeyboard(_pURLInput->_pLayout, "keyboard", 0);
		}
	}
}
