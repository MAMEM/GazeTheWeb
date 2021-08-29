//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

// Basic implementation of the connection to the Tobii Pro SDK
// Limitations:
// - License Key is not set (must be done via TobiiProEyetrackerManager)
// - Gaze data is provided in relative screen space. Conversion assumes GTW to be executed on primary monitor
// - Calibration not implemented (custom stimulus required)

// This is an implementation
#define DLL_IMPLEMENTATION

#include "plugins/Eyetracker/Interface/Eyetracker.h"
#include "plugins/Eyetracker/Common/EyetrackerData.h"
#include "tobii_research.h"
#include "tobii_research_eyetracker.h"
#include "tobii_research_streams.h"
#include <algorithm>

// Global variables
TobiiResearchEyeTracker* eyetracker = NULL;

void gaze_data_callback(TobiiResearchGazeData* gaze_data, void* user_data)
{
	// Get max from both eyes (assuming, that values at failure are smaller)
	float left_x = gaze_data->left_eye.gaze_point.position_on_display_area.x;
	float right_x = gaze_data->right_eye.gaze_point.position_on_display_area.x;
	float left_y = gaze_data->left_eye.gaze_point.position_on_display_area.y;
	float right_y = gaze_data->right_eye.gaze_point.position_on_display_area.y;
	if (left_x != left_x) { left_x = 0.f; } // check for NaN
	if (right_x != right_x) { right_x = 0.f; } // check for NaN
	if (left_y != left_y) { left_y = 0.f; } // check for NaN
	if (right_y != right_y) { right_y = 0.f; } // check for NaN
	double gazeX = std::max(left_x, right_x);
	double gazeY = std::max(left_y, right_y);

	// Push back to vector
	using namespace std::chrono;
	eyetracker_global::PushBackSample(
		SampleData(
			gazeX, // x
			gazeY, // y
			SampleDataCoordinateSystem::SCREEN_RELATIVE,
			duration_cast<milliseconds>(
				system_clock::now().time_since_epoch() // timestamp
				),
			!(gazeX == 0 && gazeY == 0)
		)
	);
}

EyetrackerInfo Connect(EyetrackerGeometry geometry)
{
	// Variables
	EyetrackerInfo info;
	info.type = ET_TOBII_PRO;

	// Find eyetracker
	TobiiResearchEyeTrackers* eyetrackers = NULL;
	TobiiResearchStatus result;
	result = tobii_research_find_all_eyetrackers(&eyetrackers);

	// Use found eye tracker
	if (result == TOBII_RESEARCH_STATUS_OK && eyetrackers->count > 0)
	{
		// Get first eyetracker
		eyetracker = eyetrackers->eyetrackers[0];

		// Setup gaze data callback
		result = tobii_research_subscribe_to_gaze_data(eyetracker, gaze_data_callback, NULL);
		if (result == TOBII_RESEARCH_STATUS_OK)
		{
			// Get output frequency
			float gaze_output_frequency;
			result = tobii_research_get_gaze_output_frequency(eyetracker, &gaze_output_frequency);

			// Check whether everything worked until here
			if (result == TOBII_RESEARCH_STATUS_OK)
			{
				// Connection successful
				info.connected = true;
				info.samplerate = (int)gaze_output_frequency;				

				// Setup LabStreamingLayer
				lsl::stream_info streamInfo(
					"TobiiProLSL",
					"Gaze",
					2, // must match with number of samples in SampleData structure
					lsl::IRREGULAR_RATE, // otherwise will generate samples even if transmission paused (and somehow even gets the "real" samples, no idea how)
					lsl::cf_double64, // must match with type of samples in SampleData structure
					"source_id");
				streamInfo.desc().append_child_value("manufacturer", "Tobii AB");
				lsl::xml_element channels = streamInfo.desc().append_child("channels");
				channels.append_child("channel")
					.append_child_value("label", "gazeX")
					.append_child_value("unit", "relativeScreen");
				channels.append_child("channel")
					.append_child_value("label", "gazeY")
					.append_child_value("unit", "relativeScreen");
				eyetracker_global::SetupLabStream(streamInfo);
			}
			else
			{
				tobii_research_unsubscribe_from_gaze_data(eyetracker, gaze_data_callback);
				eyetracker = NULL;
			}
		}
		else
		{
			eyetracker = NULL;
		}	
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
	if (eyetracker != NULL)
	{
		tobii_research_unsubscribe_from_gaze_data(eyetracker, gaze_data_callback);
	}

	// Forget eyetracker
	eyetracker = NULL;

	return true;
}

void FetchSamples(SampleQueue& rspSamples)
{
	eyetracker_global::FetchSamples(rspSamples);
}

CalibrationResult Calibrate(std::shared_ptr<CalibrationInfo>& rspInfo)
{
	// Not supported (TODO, custom stimulus required...)
	return CALIBRATION_NOT_SUPPORTED;
}

TrackboxInfo GetTrackboxInfo()
{
	// TODO
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