//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

// This is an implementation
#define DLL_IMPLEMENTATION

#include "plugins/Eyetracker/Interface/Eyetracker.h"
#include "plugins/Eyetracker/Common/EyetrackerData.h"
#include "iViewXAPI.h"
#include <algorithm>

// Global variables
static bool serverOwner = false;

int __stdcall SampleCallbackFunction(SampleStruct sampleData)
{
	// Get max from both eyes (assuming, that values at failure are smaller)
	double gazeX = std::max(sampleData.leftEye.gazeX, sampleData.rightEye.gazeX);
	double gazeY = std::max(sampleData.leftEye.gazeY, sampleData.rightEye.gazeY);

	// Push back to vector
	using namespace std::chrono;
	eyetracker_global::PushBackSample(
		SampleData(
			gazeX, // x
			gazeY, // y
			SampleDataCoordinateSystem::SCREEN_PIXELS,
			duration_cast<milliseconds>(
				system_clock::now().time_since_epoch() // timestamp
				),
			!(gazeX == 0 && gazeY == 0)
		)
	);

	return 1;
}

EyetrackerInfo Connect(EyetrackerGeometry geometry)
{
	// TODO: use provided geometry similar to myGaze plugin

	// Variables
	EyetrackerInfo info;
	info.type = ET_SMI_IVIEWX;
	int ret_connect = 0;

	// Connect to iViewX server
	ret_connect = iV_ConnectLocal();

	// If server not running, try to start it
	if (ret_connect != RET_SUCCESS)
	{
		// Start iViewX server
		iV_Start(iViewNG);

		// Retry to connect to iViewX server
		ret_connect = iV_ConnectLocal();

		// Remember to shut down server
		if (ret_connect == RET_SUCCESS)
		{
			serverOwner = true;
		}
	}

	// Set sample callback
	if (ret_connect == RET_SUCCESS)
	{
		// Connection successful
		info.connected = true;

		// Get system info
		SystemInfoStruct systemInfoData;
		iV_GetSystemInfo(&systemInfoData);
		info.samplerate = systemInfoData.samplerate;

		// Setup LabStreamingLayer
		lsl::stream_info streamInfo(
			"myViewXLSL",
			"Gaze",
			2, // must match with number of samples in SampleData structure
			lsl::IRREGULAR_RATE, // otherwise will generate samples even if transmission paused (and somehow even gets the "real" samples, no idea how)
			lsl::cf_double64, // must match with type of samples in SampleData structure
			"source_id");
		streamInfo.desc().append_child_value("manufacturer", "SensoMotoric Instruments GmbH");
		lsl::xml_element channels = streamInfo.desc().append_child("channels");
		channels.append_child("channel")
			.append_child_value("label", "gazeX")
			.append_child_value("unit", "screenPixels");
		channels.append_child("channel")
			.append_child_value("label", "gazeY")
			.append_child_value("unit", "screenPixels");
		eyetracker_global::SetupLabStream(streamInfo);

		// Define a callback function for receiving samples
		iV_SetSampleCallback(SampleCallbackFunction);
	}

	// Return info structure
	return info;
}

bool IsTracking()
{
	return true;
}

bool Disconnect()
{
	// Just terminate lab stream (not necessary to have done setup)
	eyetracker_global::TerminateLabStream();

	// Disable callbacks
	iV_SetSampleCallback(NULL);

	// Disconnect
	if (serverOwner) // also shutdown server
	{
		return iV_Quit() == RET_SUCCESS;
	}
	else
	{
		return iV_Disconnect() == RET_SUCCESS;
	}
}

void FetchSamples(SampleQueue& rspSamples)
{
	eyetracker_global::FetchSamples(rspSamples);
}

CalibrationResult Calibrate(std::shared_ptr<CalibrationInfo>& rspInfo)
{
	// Start calibration
	return iV_Calibrate() == RET_SUCCESS ? CALIBRATION_OK : CALIBRATION_FAILED;
}

TrackboxInfo GetTrackboxInfo()
{
	return TrackboxInfo();
}

void ContinueLabStream()
{
	eyetracker_global::ContinueLabStream();
}

void PauseLabStream()
{
	eyetracker_global::PauseLabStream();
}