//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "Pipeline.h"
#include "src/State/Web/Tab/Interface/TabInteractionInterface.h"

Pipeline::Pipeline(TabInteractionInterface* pTab)
{
    // Save pointer to Tab interface
    _pTab = pTab;
}

Pipeline::~Pipeline()
{
    // Deactivate current action if necessary
    if((_currentActionIndex >= 0) && (_currentActionIndex < (int)_actions.size()))
    {
        _actions.at(_currentActionIndex)->Deactivate();
    }

    // Clear all actions
    _actions.clear();

    // Reset interface to tab
    _pTab->Reset();
}

bool Pipeline::Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput)
{
    if(!_actions.empty())
    {
        // Update current action
        bool finished = _actions[_currentActionIndex]->Update(tpf, spInput, spVoiceInput);

        // Check current action for finishing of execution
        if(finished)
        {
            // Deactivate action
            _actions[_currentActionIndex]->Deactivate();

            // Execute connections (copy values)
            Action const * pPreviousAction = _actions[_currentActionIndex].get();
            for(auto& rupConnector : _connectors)
            {
				if (auto spPreviousActionInConnector = rupConnector->GetPreviousAction().lock())
				{
					if (spPreviousActionInConnector.get() == pPreviousAction)
					{
						rupConnector->Execute();
					}
				}
            }

            // Next action
            _currentActionIndex++;

            // Check whether there is an action left
            if(_currentActionIndex >= (int)_actions.size())
            {
                return true;
            }
            else
            {
                // Start next action
                _actions[_currentActionIndex]->Activate();
            }
        }
        return false;
    }
    else
    {
        // Reset Tab (at least parts which can be accessed by this)
        _pTab->Reset();
        return true;
    }
}

void Pipeline::Draw() const
{
    if(!_actions.empty())
    {
        _actions[_currentActionIndex]->Draw();
    }
}

void Pipeline::Activate()
{
    if(!_actions.empty())
    {
        _currentActionIndex = 0;
        _actions[_currentActionIndex]->Activate();
    }
}

void Pipeline::Deactivate()
{
    // Nothing to do here
}

void Pipeline::Abort()
{
	// Deactivate current action if necessary
	if ((_currentActionIndex >= 0) && (_currentActionIndex < (int)_actions.size()))
	{
		_actions.at(_currentActionIndex)->Deactivate();
	}

	// Abort all actions
    for(auto& upAction : _actions)
    {
        upAction->Abort();
    }

    // Reset Tab (at least parts which can be accessed by this)
    _pTab->Reset();
}
