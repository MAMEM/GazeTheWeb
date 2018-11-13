//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "HistoryManager.h"
#include "src/Global.h"
#include "src/Setup.h"
#include "src/Utils/Logger.h"
#include "submodules/eyeGUI/externals/TinyXML2/tinyxml2.h"
#include <iterator>

const std::vector<std::string> HistoryManager::_filterURLs
{
	"about:blank",
	setup::DASHBOARD_URL
};

int HistoryManager::Page::_idCount = 0; // initialize counter

HistoryManager::HistoryManager(std::string userDirectory)
{
	// Fill members
	_fullpathHistory = userDirectory + HISTORY_FILE;
	_spPages = std::make_shared<std::deque<std::shared_ptr<Page> > >();

	// Load existing history
	if (!LoadHistory()) { LogInfo("HistoryManager: No history file found or parsing error"); }
}

HistoryManager::~HistoryManager()
{
	// Nothing to do
}

std::shared_ptr<HistoryManager::Page> HistoryManager::AddPage(std::string URL, std::string title)
{
	// Filter page
	if (FilterPage(URL))
	{
		// Page should not be added to history
		return nullptr;
	}

	// Create page
	auto spPage = std::make_shared<Page>(this, URL, title);

	// Add to deque storing pages
	_spPages->push_front(spPage);

	// Save page in history file
	SavePageInHistory(true, spPage->GetId(), spPage->GetURL(), spPage->GetTitle());

	// Return shared pointer to page, so caller can change attributes
	return spPage;
}

std::shared_ptr<const std::deque<std::shared_ptr<HistoryManager::Page> > > HistoryManager::GetHistory() const
{
	return _spPages;
}

void HistoryManager::ClearHistoryAndDeleteFile()
{
	_spPages->clear();
	std::remove(_fullpathHistory.c_str());
}

bool HistoryManager::SavePageInHistory(bool initialStoring, int id, std::string URL, std::string title)
{
	// Do not save history to file in demo mode
	if (setup::DEMO_MODE)
	{
		return true;
	}

	// Try to open XML file
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError result = doc.LoadFile(_fullpathHistory.c_str());

	// Try to create XML file on failure
	tinyxml2::XMLNode* pRoot = NULL;
	if (result != tinyxml2::XMLError::XML_SUCCESS)
	{
		// Create and insert root
		pRoot = doc.NewElement("history");
		doc.InsertFirstChild(pRoot);

		// Try to save file
		result = doc.SaveFile(_fullpathHistory.c_str());

		// Return at failure
		if (result != tinyxml2::XMLError::XML_SUCCESS)
		{
			LogInfo("HistoryManager: Failed to create history file");
			return false;
		}
	}
	else
	{
		// Fetch root node when file exists
		pRoot = doc.FirstChild();
	}

	// Check whether root exists
	if (pRoot == NULL)
	{
		LogInfo("HistoryManager: Failed to add page to history");
		return false;
	}

	// Check whether this is the initial storing or an update
	if (initialStoring)
	{
		// Create element for page
		tinyxml2::XMLElement* pElement = doc.NewElement("page");
		pElement->SetAttribute("url", URL.c_str());
		pElement->SetAttribute("title", title.c_str());
		pElement->SetAttribute("id", id); // this is used to identify the entry for later changes

		// Append to top of XML file
		pRoot->InsertFirstChild(pElement);
	}
	else
	{
		// Find the element by URL and id
		tinyxml2::XMLElement* pElement = pRoot->FirstChildElement("page");
		while (pElement != NULL)
		{
			// Check for both URL and id
			if (pElement->Attribute("url") == URL && pElement->IntAttribute("id") == id)
			{
				// Update title
				pElement->SetAttribute("title", title.c_str());
				break;
			}
			else // continue search
			{
				pElement = pElement->NextSiblingElement("page");
			}
		}
	}
	
	// Check length of vector and delete history if too many pages have been saved
	int pagesSize = (int)_spPages->size();
	if (pagesSize > (int)setup::HISTORY_MAX_PAGE_COUNT)
	{
		// Count of pages that should be deleted
		int deleteCount = pagesSize - setup::HISTORY_MAX_PAGE_COUNT;

		// Delete older ones from vector
		_spPages->erase(_spPages->end() - deleteCount, _spPages->end());

		// Delete older ones from XML file
		for (int i = 0; i < deleteCount; i++)
		{
			auto pLastChild = pRoot->LastChild();
			pRoot->DeleteChild(pLastChild);
		}
	}

	// Try to save file
	result = doc.SaveFile(_fullpathHistory.c_str());

	// Log at failure
	if (result != tinyxml2::XMLError::XML_SUCCESS)
	{
		LogInfo("HistoryManager: Failed to save history file");
		return false;
	}

	return true;
}

bool HistoryManager::LoadHistory()
{
    // Clean local history copy
	_spPages->clear();

	// Do not load history to file in demo mode
	if (setup::DEMO_MODE)
	{
		return true;
	}

	// Open document
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError result = doc.LoadFile(_fullpathHistory.c_str());

	// Check result of opening 
	if (result != tinyxml2::XMLError::XML_SUCCESS) { return false; }

	// Fetch root
	tinyxml2::XMLNode* pRoot = doc.FirstChild();
	if (pRoot == NULL) { return false; }

	// Get first child
	tinyxml2::XMLElement* pElement = pRoot->FirstChildElement("page");
	if (pElement == NULL) { return true; } // nothing found but somehow successful

    // Collect pages (not checked whether maximum number exceeded. Only done at saving)
	do
	{
		// Preparation
		std::string URL;
		std::string title;
		bool pageError = false;

		// Query URL string from attribute
		const char * pURL = NULL;
		pURL = pElement->Attribute("url");
		if ((pageError |= (pURL == NULL)) == false)
		{
			URL = pURL;
		}

		// Query title string from attribute
		const char * pTitle = NULL;
		pTitle = pElement->Attribute("title");
		if ((pageError |= (pTitle == NULL)) == false)
		{
			title = pTitle;
		}

		// Add page only when no error occured
		if (!pageError)
		{
			_spPages->push_back(std::make_shared<Page>(this, URL, title));
		}

	} while ((pElement = pElement->NextSiblingElement("page")) != NULL);

	// When you came to here no real errors occured
	return true;
}

bool HistoryManager::FilterPage(std::string URL) const
{
	// Go over filter list and test for substring
	for (const std::string& rURL : _filterURLs)
	{
		if (URL.find(rURL) != std::string::npos)
		{
			// URL is filtered
			return true;
		}
	}

	// URL is ok
	return false;
}
