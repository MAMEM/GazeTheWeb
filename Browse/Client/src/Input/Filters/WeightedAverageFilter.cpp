//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "WeightedAverageFilter.h"
#include "src/Utils/Helper.h"
#include "src/Setup.h"

#include <iostream>

WeightedAverageFilter::WeightedAverageFilter(FilterKernel kernel, float windowTime, bool outlierRemoval) :
	_kernel(kernel), _windowTime(windowTime), _outlierRemoval(outlierRemoval) {}

void WeightedAverageFilter::ApplyFilter(const SampleQueue& rSamples, double& rGazeX, double& rGazeY, float& rFixationDuration, float samplerate) const
{
	// Prepare variables
	double sumX = 0;
	double sumY = 0;
	double weightSum = 0;
	int windowSize = glm::ceil(_windowTime * samplerate);
	
	// Indexing
	const int size = (int)rSamples->size();
	int endIndex = glm::max(0, size - windowSize);
	int startIndex = size - 1;

	// Indices updated by loop
	int weightIndex = 0; // weight index inputted into weight calculation
	int oldestUsedIndex = -1; // used for determining fixation duration

	// Adjust indices for outlier removal (latest sample may not be used)
	if (_outlierRemoval)
	{
		// Queue is iterated from back (where the latest samples are) to the front. So move one entry towards front
		--startIndex; // is -1 for queue with one sample, so single sample is not filtered and nothing happens
		endIndex = glm::max(0, endIndex - 1); // end index must be floored to zero
	}

	// Go over samples and smooth
	for(int i = startIndex; i >= endIndex; --i) // latest to oldest means reverse order in queue
	{
		// Get current sample
		const auto& rGaze = rSamples->at(i);

		// Saccade detection
		if (i < size - 1) // only proceed when there is a previous sample to check against
		{
			// Check distance of current sample and previously filtered one
			const auto& prevGaze = rSamples->at(i + 1); // in terms of time, prevGaze is newer than gaze
			if (glm::distance(
				glm::vec2(prevGaze.x, prevGaze.y),
				glm::vec2(rGaze.x, rGaze.y))
				> setup::FILTER_GAZE_FIXATION_PIXEL_RADIUS)
			{
				if (_outlierRemoval) // check whether to ignore this as outlier or really breaking
				{
					int nextIndex = i - 1; // index of next sample to filter (which is older than current)
					if (nextIndex >= 0)
					{
						const auto& nextGaze = rSamples->at(nextIndex);
						if (glm::distance(
							glm::vec2(prevGaze.x, prevGaze.y),
							glm::vec2(nextGaze.x, nextGaze.y))
							> setup::FILTER_GAZE_FIXATION_PIXEL_RADIUS)
						{
							break; // previous and next sample do not belong to the same fixation, so this sample is start of new fixation
						}
						else
						{
							continue; // previous and next sample do belong to the same fixation, skip this outlier
						}
					}
					else
					{
						break; // no next index available, so cannot deterime whether this is an outlier
					}
					
				}
				else // no outlier removal performed
				{
					break; // just break as this sample is too far away from previous
				}
			}
		}

		// Calculate weight
		double weight = CalculateWeight(weightIndex, windowSize);

		// Sum values
		sumX += rGaze.x * weight;
		sumY += rGaze.y * weight;

		// Sum weight for later normalization
		weightSum += weight;

		// Update index of oldest sample point considered as belonging to current fixation
		oldestUsedIndex = i;

		// Update weight index
		weightIndex++;
	}

	// Update members
	float fixationDuration = 0; // fallback value of fixation duration
	if (oldestUsedIndex >= 0)
	{
		// Filter gaze
		rGazeX = sumX / weightSum;
		rGazeY = sumY / weightSum;

		// Calculate fixation duration (duration from now to receiving of oldest sample contributing to fixation)
		fixationDuration = (float)((double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch() - rSamples->at(oldestUsedIndex).timestamp).count() / 1000.0);
	}
	rFixationDuration = fixationDuration; // update fixation duration
}

double WeightedAverageFilter::CalculateWeight(unsigned int i, int windowSize) const
{
	switch (_kernel)
	{
	case FilterKernel::LINEAR:
		return 1.0;
		break;
	case FilterKernel::TRIANGULAR:
		return windowSize - (int)i;
		break;
	case FilterKernel::GAUSSIAN:
		// Calculate sigma for gaussian filter
		float sigma = glm::sqrt(-glm::pow(windowSize - 1.f, 2.f) / (2.f * glm::log(0.05f))); // determine sigma, so that no weight is below 0.05
		float gaussianDenominator = (2.f * glm::pow(sigma, 2.f));
		return glm::exp(-glm::pow(i, 2.f) / gaussianDenominator);
		break;
	}
	return 1.0;
}