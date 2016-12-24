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

HistoryManager::HistoryManager(std::string userDirectory)
{
	// Fill members
	_fullpathHistory = userDirectory + HISTORY_FILE;

	// Load existing history
	if (!LoadHistory()) { LogInfo("HistoryManager: No history file found or parsing error"); }
}

HistoryManager::~HistoryManager()
{
	// Nothing to do
}

void HistoryManager::AddPage(Page page)
{
	// Filter page
	if (FilterPage(page))
	{
		// Page should not be added to history
		return;
	}

	// Add to vector storing pages
	_pages.push_front(page);

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
			return;
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
		return;
	}

	// Create element for page
	tinyxml2::XMLElement* pElement = doc.NewElement("page");
	pElement->SetAttribute("url", page.URL.c_str());
	pElement->SetAttribute("title", page.title.c_str());

	// Append to top of XML file
	pRoot->InsertFirstChild(pElement);

	// Check length of vector and delete history if too many pages have been saved
	int pagesSize = (int)_pages.size();
    if (pagesSize > (int)setup::HISTORY_MAX_PAGE_COUNT)
	{
		// Count of pages that should be deleted
		int deleteCount = pagesSize - setup::HISTORY_MAX_PAGE_COUNT;

		// Delete older ones from vector
		_pages.erase(_pages.begin(), _pages.begin() + deleteCount);

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
	}
}

std::deque<HistoryManager::Page> HistoryManager::GetHistory() const
{
	return _pages;
}

std::deque<HistoryManager::Page> HistoryManager::GetHistory(int count) const
{
	// When count is negative or zero
	if (count <= 0)
	{
		return std::deque<HistoryManager::Page>();
	}

	// Count is limited by size of page deque
    if (count > (int)_pages.size())
	{
		count = _pages.size();
	}

	// Return sublist of pages
	return std::deque<HistoryManager::Page>(_pages.begin(), _pages.begin() + count);
}

bool HistoryManager::LoadHistory()
{
    // Clean local history copy
	_pages.clear();

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
		Page page;
		bool pageError = false;

		// Query URL string from attribute
		const char * pURL = NULL;
		pURL = pElement->Attribute("url");
		if ((pageError |= (pURL == NULL)) == false)
		{
			page.URL = pURL;
		}

		// Query title string from attribute
		const char * pTitle = NULL;
		pTitle = pElement->Attribute("title");
		if ((pageError |= (pTitle == NULL)) == false)
		{
			page.title = pTitle;
		}

		// Add page only when no error occured
		if (!pageError)
		{
			_pages.push_back(page);
		}
	} while ((pElement = pElement->NextSiblingElement("page")) != NULL);

	// When you came to here no real errors occured
	return true;
}

bool HistoryManager::FilterPage(Page page) const
{
	// Go over filter list and test for equality
	for (const std::string& rURL : _filterURLs)
	{
		if (rURL == page.URL)
		{
			// URL is filtered
			return true;
		}
	}

	// URL is ok
	return false;
}
