//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Manager of history.
// TODO: Id is not necessary for loaded entries, they are only used for pages
// that are still active and which title might change.

#ifndef HISTORYMANAGER_H_
#define HISTORYMANAGER_H_

#include <string>
#include <deque>
#include <vector>
#include <functional>
#include <memory>

class HistoryManager
{
public:

	class Page; // forward declaration

	// Constructor
	HistoryManager(std::string userDirectory);

	// Destructor
	virtual ~HistoryManager();

	// Add page and returns preliminary entry, which can be changed by the caller. Might return nullptr if URL is filtered
	std::shared_ptr<Page> AddPage(std::string URL, std::string title);

	// Get history
	std::shared_ptr<const std::deque<std::shared_ptr<Page> > > GetHistory() const;

	// Clear history and delete history file
	void ClearHistoryAndDeleteFile();

private:

	// Friend class
	friend class Page;

	// List of filtered pages which will not be added to history
	static const std::vector<std::string> _filterURLs;

	// Save history to hard disk. Returns whether successful
	bool SavePageInHistory(bool initialStoring, int id, std::string URL, std::string title);

	// Load history from hard disk. Returns whether successful
	bool LoadHistory();

	// Filter pages like about:blank. Returns true when page should be NOT added
	bool FilterPage(std::string URL) const;

	// Deque of pages
	std::shared_ptr<std::deque<std::shared_ptr<Page> > > _spPages;

	// Fullpath to history file
	std::string _fullpathHistory;

public:

	class Page
	{
	public:

		// Constructor
		Page(HistoryManager* pHistoryManager, std::string URL, std::string title)
			: _id(_idCount++), _pHistoryManager(pHistoryManager), _URL(URL), _title(title) {}

		// Set title
		void SetTitle(std::string title) { _title = title; _pHistoryManager->SavePageInHistory(false, _id, _URL, _title); }

		// Read attributes
		int GetId() const { return _id; }
		std::string GetURL() const { return _URL; }
		std::string GetTitle() const { return _title; }

	private:

		// Members
		int _id; // unique identifier of history entry
		HistoryManager* _pHistoryManager;
		std::string _URL;
		std::string _title;
		static int _idCount;
	};
};

#endif // HISTORYMANAGER_H_
