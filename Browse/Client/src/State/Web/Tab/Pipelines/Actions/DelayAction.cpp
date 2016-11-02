//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "DelayAction.h"

DelayAction::DelayAction(TabInteractionInterface *pTab, float seconds) : Action(pTab)
{
	_time = seconds;
}

bool DelayAction::Update(float tpf, TabInput tabInput)
{
	_time -= tpf;
	bool done = _time < 0;
    return done;
}

void DelayAction::Draw() const
{
	// Nothing to do
}

void DelayAction::Activate()
{
	// Nothing to do
}

void DelayAction::Deactivate()
{
	// Nothing to do
}

void DelayAction::Abort()
{
	// Nothing to do
}
