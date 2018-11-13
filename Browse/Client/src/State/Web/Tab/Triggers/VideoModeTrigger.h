//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Implementation of trigger generated from videos.

#ifndef VIDEOMODETRIGGER_H_
#define VIDEOMODETRIGGER_H_

#include "src/State/Web/Tab/Triggers/DOMTrigger.h"

class VideoModeTrigger : public DOMTrigger<DOMVideo>
{
public:

	// Constructor
	VideoModeTrigger(TabInteractionInterface* pTab, std::vector<Trigger*>& rTriggerCollection, std::shared_ptr<DOMVideo> spNode, std::function<void(int id)> callback);

	// Destructor
	virtual ~VideoModeTrigger();

	// Update
	virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput);

private:

	// Members
	std::function<void(int)> _callback;
};

#endif // VIDEOMODETRIGGER_H_