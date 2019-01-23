//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstract class of states like Web or Settings. Only one state is active in
// Master at the same time.

#ifndef STATE_H_
#define STATE_H_

#include "submodules/eyeGUI/include/eyeGUI.h"
#include "src/Input/Input.h"
#include "src/Input/VoiceInput.h"

// Forward declaration
class Master;

// Enumeration of states
enum class StateType
{
    WEB, SETTINGS
};

class State
{
public:

    // Constructor
    State(Master* pMaster);

    // Destructor
    virtual ~State() = 0;

    // Update. Returns which state should be active in next time step
    virtual StateType Update(float tpf, const std::shared_ptr<const Input> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject) = 0;

    // Draw
    virtual void Draw() const = 0;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

protected:

    // Pointer to master
    Master* _pMaster;

    // Activity
    bool _active = false;
};

#endif // STATE_H_
