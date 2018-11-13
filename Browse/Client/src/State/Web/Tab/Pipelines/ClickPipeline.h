//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Pipeline to emulate clicking by zooming to a screen coordinate controlled
// with gaze.

#ifndef CLICKPIPELINE_H_
#define CLICKPIPELINE_H_

#include "Pipeline.h"

class ClickPipeline : public Pipeline
{
public:

    // Constructor
    ClickPipeline(TabInteractionInterface* pTab);
};

#endif // CLICKPIPELINE_H_
