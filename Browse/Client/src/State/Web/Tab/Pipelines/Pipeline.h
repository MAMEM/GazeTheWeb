//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Pipeline is combination of connected actions. Only one action may be active
// at a frame.

// Notes
// - Coordinates are always relative in webview space in pipeline

#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/State/Web/Tab/Pipelines/Actions/ActionConnector.h"
#include "src/Input/Input.h"
#include <memory>
#include <vector>
#include "src/Input/VoiceInput.h"


// Forward declaration
class TabInteractionInterface;

class Pipeline
{
public:

    // Constructor
    Pipeline(TabInteractionInterface* pTab);

    // Destructor
    virtual ~Pipeline() = 0;

    // Update retuns whether finished with execution
    bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject);

    // Draw
    void Draw() const;

    // Activate
    void Activate();

    // Deactivate
    void Deactivate();

    // Abort
    void Abort();

protected:

    // Pointer to tab interface
    TabInteractionInterface* _pTab;

    // Vector with actions
    std::vector<std::shared_ptr<Action> > _actions;

    // Index of current action
    int _currentActionIndex = -1;

    // Vector with connections
    std::vector<std::unique_ptr<ActionConnector> > _connectors;
};

#endif // PIPELINE_H_
