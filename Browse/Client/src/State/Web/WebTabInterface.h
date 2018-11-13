//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Interface how Tab can interact with Web class. All methods here should only
// add jobs to the Web object. Otherwise, there may be a cycle when Tab calls
// a method here while Web is updating the tabs. Then the iteration over the
// tabs may break and the whole program collapses.

#ifndef WEBTABINTERFACE_H_
#define WEBTABINTERFACE_H_

#include "src/State/Web/Managers/HistoryManager.h"
#include "src/Award.h"
#include <string>

// Forward declarations
class Tab;

class WebTabInterface
{
public:

	// Add tab after that tab
    virtual void PushAddTabAfterJob(Tab* pCaller, std::string URL, CefRefPtr<CefRequestContext> request_context) = 0;

	// Update award
	virtual void PushUpdateAwardJob(Tab* pCaller, Award award) = 0;

    // Get own id in web. Returns -1 if not found
    virtual int GetIdOfTab(Tab const * pCaller) const = 0;

	// Add history entry
	virtual std::shared_ptr<HistoryManager::Page> AddPageToHistory(std::string URL, std::string title) = 0;
};

#endif // WEBTABINTERFACE_H_
