//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "History.h"
#include "src/Global.h"
#include "src//Master/Master.h"
#include "src/Utils/Helper.h"


History::History(Master* pMaster, HistoryManager const * pHistoryManager)
{
	// Fill members
	_pMaster = pMaster;
	_pHistoryManager = pHistoryManager;

	// Create layouts
	_pLayout = _pMaster->AddLayout("layouts/History.xeyegui", EYEGUI_WEB_HISTORY_LAYER, false);

	// Create listener
	_spHistoryButtonListener = std::shared_ptr<HistoryButtonListener>(new HistoryButtonListener(this));
	eyegui::registerButtonListener(_pLayout, "close", _spHistoryButtonListener);
}

History::~History()
{
	_pMaster->RemoveLayout(_pLayout);
}

bool History::Update()
{
	return _finished;
}

void History::Activate(int tabId)
{
	if (!_active)
	{
		// Make layout visible
		eyegui::setVisibilityOfLayout(_pLayout, true, true, true);

		// Reset collected URL
		_collectedURL = u"";

		// Finished, not yet
		_finished = false;

		// Remember id of tab which url shall be changed
		_currentTabId = tabId;

		// Remember activation
		_active = true;

		// Fetch history
		auto spPages = _pHistoryManager->GetHistory();

		// Replace stack in layout with fresh one
		eyegui::replaceElementWithBrick(
			_pLayout,
			"history_flow", // same id used in brick, so no replacement necessary
			"bricks/HistoryFlow.beyegui",
			false);

		// Prepare adding of history pages to layout
		std::string brickId;
		std::map<std::string, std::string> idMapper;

		// Page count
		int pageCount = glm::min(HISTORY_DISPLAY_COUNT, (int)spPages->size());

		// Set space of flow
		float space = ((float)(pageCount + 1) / (float)HISTORY_ROWS_ON_SCREEN); // size + 1 for title
		space = space < 1.f ? 1.f : space;
		eyegui::setSpaceOfFlow(_pLayout, "history_flow", space);

		// Go over available pages but maximum the desired count to display
		for (int i = 0; i < pageCount; i++)
		{
			// Brick id
			brickId = "history_" + std::to_string(i);

			// URL id
			std::string URLId = "url_" + std::to_string(i);

			// Title id
			std::string titleId = "title_" + std::to_string(i);

			// Select id
			std::string selectId = "select_" + std::to_string(i);

			// Id mapper
			idMapper.clear();
			idMapper.emplace("grid", brickId);
			idMapper.emplace("url", URLId);
			idMapper.emplace("title", titleId);
			idMapper.emplace("select", selectId);

			// Add brick to stack, which was added by flow brick
			eyegui::addBrickToStack(
				_pLayout,
				"history_stack",
				"bricks/History.beyegui",
				idMapper);

			// Set content of displayed URL
			eyegui::setContentOfTextBlock(_pLayout, URLId, spPages->at(i)->GetURL());

			// Set content of display title
			eyegui::setContentOfTextBlock(_pLayout, titleId, spPages->at(i)->GetTitle());

			// Register button listener for select buttons
			eyegui::registerButtonListener(_pLayout, selectId, _spHistoryButtonListener);
		}
	}
}

void History::Deactivate()
{
	if (_active)
	{
		// Make layouts invisible
		eyegui::setVisibilityOfLayout(_pLayout, false, false, true);

		// Remember it
		_active = false;
	}
}

std::string History::GetURL() const
{
	std::string url8;
	eyegui_helper::convertUTF16ToUTF8(_collectedURL, url8);
	return url8;
}

void History::HistoryButtonListener::down(eyegui::Layout* pLayout, std::string id)
{
	// Standard layout
	if (id == "close")
	{
		// Reset URL to empty to indicate abortion
		_pHistory->_collectedURL = u"";
		_pHistory->_finished = true;
	}
	else // some history page has been selected
	{
		// Split id by underscore
		std::string delimiter = "_";
		size_t pos = 0;

		// Check for keyword "select"
		if (((pos = id.find(delimiter)) != std::string::npos) && id.substr(0, pos) == "select")
		{
			// Get URL of selected history entry
			auto spPages = _pHistory->_pHistoryManager->GetHistory();
			std::string URL = spPages->at((int)(StringToFloat(id.substr(pos + 1, id.length() - 1))))->GetURL();
			std::u16string URL16;
			eyegui_helper::convertUTF8ToUTF16(URL, URL16);
			_pHistory->_collectedURL = URL16;
			_pHistory->_finished = true;
			nlohmann::json record = { { "url", URL } };
			_pHistory->_pMaster->SimplePushBackAsyncJob(FirebaseIntegerKey::GENERAL_HISTORY_USAGE_COUNT, FirebaseJSONKey::GENERAL_HISTORY_USAGE, record);
		}
	}
}