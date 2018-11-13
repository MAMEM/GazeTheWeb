//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Records sensor data coming in from LSL. Requires SensorLib integration,
// otherwise does nothing.

#ifndef SENSORRECORDER_H_
#define SENSORRECORDER_H_

#include <memory>

// Forward declaration
namespace SensorLib
{
	class SensorLibrary;
}

class SensorRecorder
{
public:

	// Constructor. Starts recording
	SensorRecorder();

	// Destructor. Stops recording
	virtual ~SensorRecorder();

	// Start (called by constructor)
	void Start();

	// Stop (called by destructor)
	void Stop();

private:
#ifdef CLIENT_SENSOR_LIB_INTEGRATION
	// Members
	std::unique_ptr<SensorLib::SensorLibrary> _upSensorLib;
#endif
};

#endif // SENSORRECORDER_H_