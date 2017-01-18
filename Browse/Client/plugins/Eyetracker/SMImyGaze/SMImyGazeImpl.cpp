//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "plugins/Eyetracker/Interface/Eyetracker.h"
#include "plugins/Eyetracker/Common/EyetrackerData.h"
#include "plugins/Eyetracker/SMImyGaze/mygaze/include/myGazeAPI.h"
#include <algorithm>

int __stdcall SampleCallbackFunction(SampleStruct sampleData)
{
	// Get max from both eyes (assuming, that values at failure are smaller)
	double gazeX = std::max(sampleData.leftEye.gazeX, sampleData.rightEye.gazeX);
	double gazeY = std::max(sampleData.leftEye.gazeY, sampleData.rightEye.gazeY);

	// Push back to array
	eyetracker_global::PushBackRawData(gazeX, gazeY, gazeX != 0 && gazeY != 0);

	return 1;
}

bool Connect()
{
	// Initialize eyetracker
	SystemInfoStruct systemInfoData;
	int ret_connect = 0;

	// Connect to iViewX
	ret_connect = iV_Connect();

	// Set sample callback
	if (ret_connect == RET_SUCCESS)
	{
		iV_GetSystemInfo(&systemInfoData);
		/*
		LogInfo("iViewX ETSystem: ", systemInfoData.iV_ETDevice);
		LogInfo("iViewX iV_Version: ", systemInfoData.iV_MajorVersion, ".", systemInfoData.iV_MinorVersion, ".", systemInfoData.iV_Buildnumber);
		LogInfo("iViewX API_Version: ", systemInfoData.API_MajorVersion, ".", systemInfoData.API_MinorVersion, ".", systemInfoData.API_Buildnumber);
		LogInfo("iViewX SystemInfo Samplerate: ", systemInfoData.samplerate);
		*/

		// Define a callback function for receiving samples
		iV_SetSampleCallback(SampleCallbackFunction);
	}

	// Return success or failure
	return (ret_connect == RET_SUCCESS);
}

bool Disconnect()
{
	// Disable callbacks
	iV_SetSampleCallback(NULL);

	// Disconnect
	return iV_Disconnect() == RET_SUCCESS;
}

void FetchGaze(int maxSampleCount, std::vector<double>& rGazeX, std::vector<double>& rGazeY)
{
	eyetracker_global::GetKOrLessValidRawGazeEntries(maxSampleCount, rGazeX, rGazeY);
}