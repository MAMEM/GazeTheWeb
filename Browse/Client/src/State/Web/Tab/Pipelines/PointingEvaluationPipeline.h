//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Pipeline for evaluation of different pointing approaches.

#ifndef POINTINGEVALUATIONPIPELINE_H_
#define POINTINGEVALUATIONPIPELINE_H_

#include "Pipeline.h"

enum PointingApproach
{
	MAGNIFICATION, FUTURE, ZOOM, DRIFT_CORRECTION, DYNAMIC_DRIFT_CORRECTION
};

class PointingEvaluationPipeline : public Pipeline
{
public:

	// Constructor
	PointingEvaluationPipeline(TabInteractionInterface* pTab, PointingApproach approach);
};

#endif // POINTINGEVALUATIONPIPELINE_H_
