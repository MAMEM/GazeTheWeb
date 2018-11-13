//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstract interface for eye gaze filters.

#ifndef FILTER_H_
#define FILTER_H_

#include "src/Input/Filters/CustomTransformationInteface.h"
#include "plugins/Eyetracker/Interface/EyetrackerSample.h"
#include <map>


// Filter class
class Filter : public CustomTransformationInterface
{
public:

	// Constructor
	Filter();

	// Destructor
	virtual ~Filter() = 0;

	// Update. Takes samples in window pixel coordinates and samplerate of eye tracking device
	void Update(const SampleQueue spSamples, float samplerate);

	// Various getters
	double GetRawGazeX() const;
	double GetRawGazeY() const;
	double GetFilteredGazeX() const;
	double GetFilteredGazeY() const;
	float GetFixationDuration() const;

	// Getter for age of last used sample
	double GetAge() const;

	// Getter which returns whether timestamp was actively set at least once (aka at least one sample received)
	bool IsTimestampSetOnce() const;

	// #######################################
	// ### CUSTOM TRANSFORMATION INTERFACE ###
	// #######################################

	// Register custom transformation of gaze before filtering (smooth pursuit fixation).
	// For each custom transformation, retrieved samples are transformed with provided transformation and stored.
	// At registration, provided transformation is applied on all existing samples after copying them
	bool RegisterCustomTransformation(std::string name, FilterTransformation transformation) override; // return whether successful

	// Set transformation for incoming gaze data for the named custom transformation
	bool ChangeCustomTransformation(std::string name, FilterTransformation transformation) override;

	// Unregister custom transformation
	bool UnregisterCustomTransformation(std::string name) override;

	// Retrieve filtered gaze with custom transformation
	double GetFilteredGazeX(std::string name) const override;
	double GetFilteredGazeY(std::string name) const override;

	// #######################################

private:

	// Actual implementation of filtering
	virtual void ApplyFilter(const SampleQueue& rSamples, double& rGazeX, double& rGazeY, float& rFixationDuration, float samplerate) const = 0;

	// Timestamp of last sample
	std::chrono::milliseconds _timestamp;

	// Bool whether timestamp was set at least once (aka at least one sample received)
	bool _timestampSetOnce = false;

	// Samples to filter
	SampleQueue _spSamples; // samples as delivered by eye tracker / EyeInput class

	// Values per custom transformations
	struct CustomTransformation
	{
		// Fields
		FilterTransformation transformation;
		SampleQueue queue;
		double gazeX = -1;
		double gazeY = -1;
		float fixationDuration = 0;
	};
	std::map<std::string, CustomTransformation> _customTransformations;

	// Filtered stuff
	double _gazeX = -1; // filtered
	double _gazeY = -1; // filtered
	float _fixationDuration = 0;
};

#endif FILTER_H_