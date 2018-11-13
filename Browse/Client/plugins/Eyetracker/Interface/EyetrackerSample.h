//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef EYETRACKERSAMPLE_H_
#define EYETRACKERSAMPLE_H_

#include <chrono>
#include <memory>
#include <deque>

// Coordinate systems for sample data coordinates
enum class SampleDataCoordinateSystem
{
	SCREEN_PIXELS, SCREEN_RELATIVE
};

// Struct of sample data
struct SampleData
{
	// Constructor
	SampleData(double x, double y, SampleDataCoordinateSystem system, std::chrono::milliseconds timestamp, bool valid) : x(x), y(y), system(system), timestamp(timestamp), valid(valid)
	{};

	// Fields
	double x;
	double y;
	SampleDataCoordinateSystem system;
	std::chrono::milliseconds timestamp; // expected to be filled initially with system_clock::now().time_since_epoch()
	bool valid;
};

// Typedef for unique pointer of sample queue
typedef std::shared_ptr<std::deque<SampleData> > SampleQueue;

#endif EYETRACKERSAMPLE_H_