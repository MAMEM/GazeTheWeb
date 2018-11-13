//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "BookmarkManager.h"
#include "src/Global.h"
#include "src/Utils/Logger.h"
#include "submodules/eyeGUI/externals/TinyXML2/tinyxml2.h"
#include <iterator>

BookmarkManager::BookmarkManager(std::string userDirectory)
{
	// Fill members
	_fullpathBookmarks = userDirectory + BOOKMARKS_FILE;

	// Load existing bookmarks
	if (!LoadBookmarks()) { LogInfo("BookmarkManager: No bookmarks file found or parsing error"); }
}

BookmarkManager::~BookmarkManager()
{
	// Nothing to do
}

bool BookmarkManager::AddBookmark(std::string URL)
{
	// Insert into set
	auto result = _bookmarks.insert(URL);

	// Check wether element was new for the set
	if (result.second) // second element in pair indicates whether value was new
	{
		if (!SaveBoomarks()) { LogInfo("BookmarkManager: Failed to save bookmarks"); }
		return true;
	}
	else
	{
		return false;
	}
}

bool BookmarkManager::RemoveBookmark(std::string URL)
{
	// Insert into set
	auto result = _bookmarks.erase(URL);

	// Check wether element was new for the set
	if (result > 0) // erase returns count of erased elements
	{
		if (!SaveBoomarks()) { LogInfo("BookmarkManager: Failed to save bookmarks"); }
		return true;
	}
	else
	{
		return false;
	}
}

std::set<std::string> BookmarkManager::GetBookmarks() const
{
	return _bookmarks;
}

std::vector<std::string> BookmarkManager::GetSortedBookmarks() const
{
	// TODO: sort somehow
	std::vector<std::string> result;
	std::copy(_bookmarks.begin(), _bookmarks.end(), std::back_inserter(result));
	return result;
}

bool BookmarkManager::IsBookmark(std::string URL) const
{
	return _bookmarks.find(URL) != _bookmarks.end();
}

void BookmarkManager::ClearBookmarksAndDeleteFile()
{
	_bookmarks.clear();
	std::remove(_fullpathBookmarks.c_str());
}

bool BookmarkManager::SaveBoomarks() const
{
	// Create document
	tinyxml2::XMLDocument doc;

	// Create and insert root
	tinyxml2::XMLNode* pRoot = doc.NewElement("bookmarks");
	doc.InsertFirstChild(pRoot);

	// Insert bookmarks
    tinyxml2::XMLElement* previousChild = NULL;
	for (const auto& rBookmark : _bookmarks)
	{
		// Insert bookmark
		tinyxml2::XMLElement* pElement = doc.NewElement("bookmark");
		pElement->SetAttribute("url", rBookmark.c_str());

		// Decide how to insert
        if (previousChild == NULL)
		{
			pRoot->InsertFirstChild(pElement);
		}
		else
		{
            pRoot->InsertAfterChild(previousChild, pElement);
		}

        // Remember previous child
        previousChild = pElement;
	}

	// Try to save file
	tinyxml2::XMLError result = doc.SaveFile(_fullpathBookmarks.c_str());

	// Return whether successful
	return (result == tinyxml2::XMLError::XML_SUCCESS);
}

bool BookmarkManager::LoadBookmarks()
{
    // Clear bookmarks
	_bookmarks.clear();

	// Open document
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError result = doc.LoadFile(_fullpathBookmarks.c_str());

	// Check result of opening 
	if (result != tinyxml2::XMLError::XML_SUCCESS) { return false; }

	// Fetch root
	tinyxml2::XMLNode* pRoot = doc.FirstChild();
	if (pRoot == NULL) { return false; }

	// Get first child
	tinyxml2::XMLElement* pElement = pRoot->FirstChildElement("bookmark");
	if (pElement == NULL) { return true; } // nothing found but somehow successful

	// Collect bookmarks
	do
	{
		// Query URL string from attribute
		const char * pURL = NULL;
		pURL = pElement->Attribute("url");
		if (pURL != NULL)
		{
			_bookmarks.insert(pURL);
		}
	} while ((pElement = pElement->NextSiblingElement("bookmark")) != NULL);

	// When you came to here no real errors occured
	return true;
}
