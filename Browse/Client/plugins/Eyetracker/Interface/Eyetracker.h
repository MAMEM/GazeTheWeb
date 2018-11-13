//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef EYETRACKER_H_
#define EYETRACKER_H_

// Decide about api style by define
#ifdef DLL_IMPLEMENTATION  
#define DLL_API __declspec(dllexport)   
#else  
#define DLL_API __declspec(dllimport)   
#endif  

#include "plugins/Eyetracker/Interface/EyetrackerSample.h"
#include "plugins/Eyetracker/Interface/EyetrackerInfo.h"
#include "plugins/Eyetracker/Interface/EyetrackerGeometry.h"

// Export C interface (resolved overloading etc)
#ifdef __cplusplus
extern "C" {
#endif

	// Connect eyetracker, returns whether succesfull
	DLL_API EyetrackerInfo Connect(EyetrackerGeometry geometry);

	// Check whether eyetracker is working (regardless of user presence)
	DLL_API bool IsTracking();

	// Disconnect eyetracker, returns whether succesfull
	DLL_API bool Disconnect();

	// Fetches gaze samples and clears buffer
	DLL_API void FetchSamples(SampleQueue& rspSamples);

	// Perform calibration, returns whether succesfull
	DLL_API CalibrationResult Calibrate(std::shared_ptr<CalibrationInfo>& rspInfo);

	// Retrieve trackbox informatoin
	DLL_API TrackboxInfo GetTrackboxInfo();

	// Continue lab streaming layer streaming
	DLL_API void ContinueLabStream();

	// Pause lab streaming layer streaming
	DLL_API void PauseLabStream();

#ifdef __cplusplus
}
#endif

#endif // EYETRACKER_H_