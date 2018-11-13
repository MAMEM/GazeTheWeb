//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Weighted average filtering. For fixation recogniction, samples are compared
// with each other and not with potential fixation. Outlier detection intro-
// duces delay of one sample, regardless whether that sample would fit into
// a fixation or a saccade.

#ifndef WEIGHTEDAVERAGEFILTER_H_
#define WEIGHTEDAVERAGEFILTER_H_

#include "src/Input/Filters/Filter.h"
#include "src/Input/Filters/FilterKernel.h"

class WeightedAverageFilter : public Filter
{
public:

	// Constructor
	WeightedAverageFilter(
		FilterKernel kernel, // type of weights used
		float windowTime, // time of window from which samples are taken for filtering
		bool outlierRemoval); // whether outlier detection is used, delays input by one sample

private:

	// Actual implementation of filtering
	void ApplyFilter(const SampleQueue& rSamples, double& rGazeX, double& rGazeY, float& rFixationDuration, float samplerate) const override;

	// Calulcate weight for a sample. Takes "oldness" of sample.
	// Interval must be [0..windowSize-1]
	double CalculateWeight(unsigned int i, int windowSize) const;

	// Members
	FilterKernel _kernel;
	float _windowTime;
	bool _outlierRemoval;
};

#endif WEIGHTEDAVERAGEFILTER_H_