//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

// This is an implementation
#define DLL_IMPLEMENTATION

#include "plugins/Eyetracker/Interface/Eyetracker.h"
#include "plugins/Eyetracker/Common/EyetrackerData.h"
#include "myGazeAPI.h"
#include <algorithm>

// Global variables
static bool serverOwner = false;
static const int RECALIBRATION_ATTEMPTS = 3;

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
	// Variables
	EyetrackerInfo info;
	int ret_connect = 0;

	// Connect to running myGaze server
	ret_connect = iV_Connect();

	// If server not running, try to start it
	if (ret_connect != RET_SUCCESS)
	{
		// Increase connection timeout
		iV_SetConnectionTimeout(10);

		// Start myGaze server
		iV_Start();

		// Retry to connect to myGaze server
		ret_connect = iV_Connect();

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

		// Set geometry
		char buf[256] = "GazeTheWeb";
		GeometryStruct REDgeometry;
		REDgeometry.geometryType = GeometryType::myGazeGeometry;
		REDgeometry.stimX = geometry.monitorWidth;
		REDgeometry.stimY = geometry.monitorHeight;
		REDgeometry.inclAngle = geometry.mountingAngle;
		REDgeometry.stimDistHeight = geometry.relativeDistanceHeight;
		REDgeometry.stimDistDepth = geometry.relativeDistanceDepth;
		strcpy_s(REDgeometry.setupName, "GazeTheWeb");
		info.geometrySetupSuccessful = RET_SUCCESS == iV_SetREDGeometry(&REDgeometry);

		// Enable low power pick up mode
		iV_EnableLowPowerPickUpMode();

		// Set tracking (not licensed)
		// iV_SetTrackingParameter(ET_PARAM_EYE_BOTH, ET_PARAM_SMARTBINOCULAR, 0);

		// Get system info
		SystemInfoStruct systemInfoData;
		iV_GetSystemInfo(&systemInfoData);
		info.samplerate = systemInfoData.samplerate;

		// Setup LabStreamingLayer
		lsl::stream_info streamInfo(
			"myGazeLSL",
			"Gaze",
			2, // must match with number of samples in SampleData structure
			lsl::IRREGULAR_RATE, // otherwise will generate samples even if transmission paused (and somehow even gets the "real" samples, no idea how)
			lsl::cf_double64, // must match with type of samples in SampleData structure
			"source_id");
		streamInfo.desc().append_child_value("manufacturer", "Visual Interaction GmbH");
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
	// Setup calibration
	CalibrationStruct calibrationData;
	calibrationData.method = 5;
	calibrationData.speed = 0;
	calibrationData.displayDevice = 0;
	calibrationData.targetShape = 2;
	calibrationData.foregroundBrightness = 242;
	calibrationData.backgroundBrightness = 33;
	calibrationData.autoAccept = 2;
	calibrationData.targetSize = 35;
	calibrationData.visualization = 1;
	strcpy(calibrationData.targetFilename, "");
	iV_SetupCalibration(&calibrationData);

	// Start calibration
	CalibrationResult result = iV_Calibrate() == RET_SUCCESS ? CALIBRATION_OK : CALIBRATION_FAILED;

	// Check calibration points
	if (result == CALIBRATION_OK) // refine result
	{
		for (int i = 1; i <= 5; i++) // go over calibration points
		{
			int count = 0;
			bool check = true;
			while (check)
			{
				// Check calibration quality of this calibration point
				CalibrationPointQualityStruct left, right;
				iV_GetCalibrationQuality(i, &left, &right);
				bool badCalibration =
					(left.usageStatus != CalibrationPointUsageStatusEnum::calibrationPointUsed)
					|| (right.usageStatus != CalibrationPointUsageStatusEnum::calibrationPointUsed)
					|| (left.qualityIndex < 0.5f && right.qualityIndex < 0.5f);

				// Check count of attempts
				if (count >= RECALIBRATION_ATTEMPTS)
				{
					// At bad calibration of this calibration point, set calibration result to bad
					if (badCalibration)
					{
						result = CALIBRATION_BAD;
					}
					check = false;
				}
				else
				{
					// Decide how to proceed
					if (badCalibration) // bad calibration
					{
						iV_RecalibrateOnePoint(i);
					}
					else // good calibration
					{
						check = false;
					}
				}

				// Increase count of attempts
				count++;
			}
		}

		// Go over all points to provide user with info
		std::shared_ptr<CalibrationInfo> spCalibrationInfo = std::make_shared<CalibrationInfo>();
		for (int i = 1; i <= 5; i++) // go over calibration points
		{
			// Retrieve calibration point
			CalibrationPointStruct point;
			if (RET_SUCCESS == iV_GetCalibrationPoint(i, &point))
			{
				// Check calibration quality of this calibration point
				CalibrationPointQualityStruct left, right;
				iV_GetCalibrationQuality(i, &left, &right);

				// Check for quality
				if (((left.usageStatus != CalibrationPointUsageStatusEnum::calibrationPointUsed)
					&& (right.usageStatus != CalibrationPointUsageStatusEnum::calibrationPointUsed))
					|| (left.qualityIndex <= 0.0f && right.qualityIndex <= 0.0f)) // failure
				{
					spCalibrationInfo->push_back(CalibrationPoint(point.positionX, point.positionY, CALIBRATION_POINT_FAILED));
				}
				else if ((left.usageStatus != CalibrationPointUsageStatusEnum::calibrationPointUsed)
					|| (right.usageStatus != CalibrationPointUsageStatusEnum::calibrationPointUsed)
					|| (left.qualityIndex < 0.5f && right.qualityIndex < 0.5f)) // bad
				{
					spCalibrationInfo->push_back(CalibrationPoint(point.positionX, point.positionY, CALIBRATION_POINT_BAD));
				}
				else // ok
				{
					spCalibrationInfo->push_back(CalibrationPoint(point.positionX, point.positionY, CALIBRATION_POINT_OK));
				}
			}
		}
		rspInfo = spCalibrationInfo;
	}

	// Return whether successful
	return result;
}

TrackboxInfo GetTrackboxInfo()
{
	TrackingStatusStruct status;
	iV_GetTrackingStatus(&status);
	TrackboxInfo info;
	info.leftTracked = status.leftEye.validity == 1;
	info.leftX = (float)status.leftEye.relativePositionX;
	info.leftY = (float)status.leftEye.relativePositionY;
	info.leftZ = (float)status.leftEye.relativePositionZ;
	info.rightTracked = status.rightEye.validity == 1;
	info.rightX = (float)status.rightEye.relativePositionX;
	info.rightY = (float)status.rightEye.relativePositionY;
	info.rightZ = (float)status.rightEye.relativePositionZ;
	return info;
}

void ContinueLabStream()
{
	eyetracker_global::ContinueLabStream();
}

void PauseLabStream()
{
	eyetracker_global::PauseLabStream();
}