//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Delegated by Web class to manage and use history.
// TODO: merge with URLInput in terms of common superclass?

#ifndef HISTORY_H_
#define HISTORY_H_

#include "src/State/Web/Managers/HistoryManager.h"
#include "submodules/eyeGUI/include/eyeGUI.h"
#include <string>
#include <vector>

// Forward declaration
class Master;

class History
{
public:

	// Constructor
	History(Master* pMaster, HistoryManager const * pHistoryManager);

	// Destructor
	virtual ~History();

	// Update while active. Returns when user leaves history screen
	bool Update();

	// Activate
	void Activate(int tabId);

	// Deactivate
	void Deactivate();

	// Get whether active
	bool IsActive() const { return _active; }

	// Get URL. Is set to an empty string if aborted
	std::string GetURL() const;

	// Get id of tab which URL is currently edited. Returned -1 if not defined
	int GetCurrentTabId() const { return _currentTabId; }

private:

	// Give listeners full access
	friend class HistoryButtonListener;

	// Listener for GUI
	class HistoryButtonListener : public eyegui::ButtonListener
	{
	public:

		HistoryButtonListener(History* pHistory) { _pHistory = pHistory; }
		void hit(eyegui::Layout* pLayout, std::string id) {}
		void down(eyegui::Layout* pLayout, std::string id);
		void up(eyegui::Layout* pLayout, std::string id) {}
		virtual void selected(eyegui::Layout* pLayout, std::string id) {}

	private:

		History* _pHistory;
	};

	// Instances of listeners
	std::shared_ptr<HistoryButtonListener> _spHistoryButtonListener;

	// Pointer to master
	Master* _pMaster;

	// Pointer to history manager
	HistoryManager const * _pHistoryManager;

	// Pointer to layout
	eyegui::Layout* _pLayout;

	// Collected url
	std::u16string _collectedURL = u"";

	// Bool whether active
	bool _active = false;

	// Bool whether history screen should be left
	bool _finished = false;

	// Id of current tab
	int _currentTabId = -1;
};

#endif // HISTORY_H_