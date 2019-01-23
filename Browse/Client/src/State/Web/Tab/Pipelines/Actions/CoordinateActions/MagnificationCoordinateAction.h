//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action to magnify to a coordinate with gaze.
// - Input: none
// - Output: vec2 coordinate in CEFPixel space

#ifndef MAGNIFICATIONCOORDINATEACTION_H_
#define MAGNIFICATIONCOORDINATEACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/Utils/LerpValue.h"
#include <deque>

class MagnificationCoordinateAction : public Action
{
public:

    // Constructor
	MagnificationCoordinateAction(TabInteractionInterface* pTab, bool doDimming = true);

    // Update retuns whether finished with execution
    virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput, std::shared_ptr<VoiceInput> spVoiceInputObject);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

    // Abort
    virtual void Abort();

protected:

	// Dimming duration
	const float DIMMING_DURATION = 0.5f; // seconds until it is dimmed

	// Dimming value
	const float DIMMING_VALUE = 0.3f;

	// Level of magnification
	const float MAGNIFICATION = 0.2f;

	// Fixation duration taken as "perceived" fixation to trigger something
	const float FIXATION_DURATION = 0.75f;

	// Duration of magnification process
	const float MAGNIFICATION_ANIMATION_DURATION = 0.5f;

	// Time to wait before checking for fixation after activation and magnification
	float fixationWaitTime = FIXATION_DURATION; // to avoid instant selection of coordinate after actiation and magnification

	// Magnfication center. In relative page space
	glm::vec2 _relativeMagnificationCenter = glm::vec2(0.f,0.f);

	// Variable to indicate how much magnified (0 if not and 1 if completely magnified)
	float _magnification = 0.f; // [0..1]

	// Variable to indicate whether magnified or not
	bool _magnify = false;

	// Dimming
	float _dimming = 0.f;

	// Do dimming
	bool _doDimming = true;
};

#endif // MAGNIFICATIONCOORDINATEACTION_H_
