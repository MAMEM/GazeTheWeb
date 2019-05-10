//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Action to zoom to a coordinate with gaze.
// - Input: none
// - Output: vec2 coordinate in CEFPixel space

#ifndef FUTURECOORDINATEACTION_H_
#define FUTURECOORDINATEACTION_H_

#include "src/State/Web/Tab/Pipelines/Actions/Action.h"
#include "src/Utils/LerpValue.h"
#include "src/Input/Filters/CustomTransformationInteface.h"
#include <deque>
#include <vector>

class FutureCoordinateAction : public Action
{
public:

    // Constructor
	FutureCoordinateAction(TabInteractionInterface* pTab, bool doDimming = true);

    // Update returns whether finished with execution
    virtual bool Update(float tpf, const std::shared_ptr<const TabInput> spInput, std::shared_ptr<VoiceAction> spVoiceInput);

    // Draw
    virtual void Draw() const;

    // Activate
    virtual void Activate();

    // Deactivate
    virtual void Deactivate();

    // Abort
    virtual void Abort();

protected:

	// Sample data
	struct SampleData
	{
		// Constructor
		SampleData(
			float zoom,
			glm::vec2 relativeGazeCoordinate,
			glm::vec2 relativeZoomCoordinate,
			glm::vec2 relativeCenterOffset)
		{
			this->zoom = zoom;
			this->relativeGazeCoordinate = relativeGazeCoordinate;
			this->relativeZoomCoordinate = relativeZoomCoordinate;
			this->relativeCenterOffset = relativeCenterOffset;
		}

		// Values
		float zoom;
		glm::vec2 relativeGazeCoordinate;
		glm::vec2 relativeZoomCoordinate;
		glm::vec2 relativeCenterOffset;
		float lifetime = 0.5f; // intial lifetime in seconds
	};

    // Coordinate which is updated and later outputted. In relative page space
    glm::vec2 _relativeZoomCoordinate;

    // Linear zooming amout, in interval 0..1
    float _linZoom = 0.f;

	// Adapted zoom, decreasing while zooming
	float _zoom = 1.f;

    // Offset to center of web view
    glm::vec2 _relativeCenterOffset = glm::vec2(0, 0);

    // Bool to indicate first update
    bool _firstUpdate = true;

	// Deviation of zoom coordinate (not of gaze!, normalized by maximal dimension)
	// Not really in any common relative coordinates, as aspect ratio corrected...
	float _deviation = 0.f; // [0..1]

	// Dimming
	float _dimming = 0.f;

	// Do dimming
	bool _doDimming = true;

	// Sample data for drift correction
	std::vector<SampleData> _sampleData;

	// Potential fixations
	std::deque<glm::vec2> _fixations;

	// Shared pointer to custom transformation interface of input
	std::shared_ptr<CustomTransformationInterface> _spTrans;
};

#endif // FUTURECOORDINATEACTION_H_
