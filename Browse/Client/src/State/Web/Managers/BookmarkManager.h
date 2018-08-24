//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Manager of bookmarks.

#ifndef BOOKMARKMANAGER_H_
#define BOOKMARKMANAGER_H_

#include <string>
#include <set>
#include <vector>

class BookmarkManager
{
public:

	// Constructor
	BookmarkManager(std::string userDirectory);

	// Destructor
	virtual ~BookmarkManager();

	// Add bookmark. Returns true when successful or false when already bookmarked
	bool AddBookmark(std::string URL);

	// Remove bookmark. Returns whether successful
	bool RemoveBookmark(std::string URL);

	// Get unsorted bookmarks
	std::set<std::string> GetBookmarks() const;

	// Get sorted bookmarks
	std::vector<std::string> GetSortedBookmarks() const;

	// Check for existence
	bool IsBookmark(std::string URL) const;

	// Clear bookmarks and delete bookmark file
	void ClearBookmarksAndDeleteFile();

private:

	// Save bookmarks to hard disk. Returns whether successful
	bool SaveBoomarks() const;

	// Load bookmarks from hard disk. Returns whether successful
	bool LoadBookmarks();

	// Set of bookmarks
	std::set<std::string> _bookmarks;

	// Fullpath to bookmarks file
	std::string _fullpathBookmarks;

};

#endif // BOOKMARKMANAGER_H_
