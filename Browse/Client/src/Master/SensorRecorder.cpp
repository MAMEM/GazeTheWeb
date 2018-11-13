//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "SensorRecorder.h"

#ifdef CLIENT_SENSOR_LIB_INTEGRATION


#include "SensorLibrary.h"

SensorRecorder::SensorRecorder()
{
	_upSensorLib = std::unique_ptr<SensorLib::SensorLibrary>(new SensorLib::SensorLibrary());
	Start();
}

SensorRecorder::~SensorRecorder()
{
	Stop();
	_upSensorLib->shutdownSensors();
}

void SensorRecorder::Start()
{
	_upSensorLib->startRecording();
}

void SensorRecorder::Stop()
{
	_upSensorLib->stopRecording();
}

#else // no SensorLib integrated

SensorRecorder::SensorRecorder()
{
	// Do nothing
}

SensorRecorder::~SensorRecorder()
{
	// Do nothing
}

void SensorRecorder::Start()
{
	// Do nothing
}

void SensorRecorder::Stop()
{
	// Do nothing
}

#endif // CLIENT_SENSOR_LIB_INTEGRATION