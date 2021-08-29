//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Abstracts input from eyetracker and mouse into general eye input. Does fallback
// to mouse cursor input provided by GLFW when no eyetracker available. Handles
// override of eyetracking input when mouse is moved.

#ifndef EYEINPUT_H_
#define EYEINPUT_H_

#include "src/Global.h"
#include "src/Master/MasterThreadsafeInterface.h"
#include "src/Input/EyeTrackerStatus.h"
#include "src/Input/Filters/Filter.h"
#include "src/Input/Input.h"
#include "plugins/Eyetracker/Interface/EyetrackerSample.h"
#include "plugins/Eyetracker/Interface/EyetrackerInfo.h"
#include "plugins/Eyetracker/Interface/EyetrackerGeometry.h"
#include <memory>
#include <vector>
#include <thread>

// Necessary for dynamic DLL loading in Windows
#ifdef _WIN32
#include <windows.h>
typedef void(__cdecl *FETCH_SAMPLES)(SampleQueue&);
typedef bool(__cdecl *IS_TRACKING)();
typedef CalibrationResult(__cdecl *CALIBRATE)(std::shared_ptr<CalibrationInfo>&);
typedef TrackboxInfo(__cdecl *GET_TRACKBOX_INFO)();
typedef void(__cdecl *CONTINUE_LAB_STREAM)();
typedef void(__cdecl *PAUSE_LAB_STREAM)();
#endif

class EyeInput
{
public:

    // Constructor, starts thread to establish eye tracker connection. Callback called from a different thread!
    EyeInput(MasterThreadsafeInterface* _pMasterThreadsafeInterface, EyetrackerGeometry geometry);

    // Destructor
    virtual ~EyeInput();

    // Update method fills Input struct and returns it
	std::shared_ptr<Input> Update(
		bool windowFocused,
		float tpf,
		double mouseX,
		double mouseY,
		int windowX,
		int windowY,
		int windowWidth,
		int windowHeight,
		int monitorWidth,
		int monitorHeight);

	// Calibrate the eye tracking device, returns result
	CalibrationResult Calibrate(std::shared_ptr<CalibrationInfo>& rspCalibrationInfo);

	// Get trackbox info
	TrackboxInfo GetTrackboxInfo();

	// Delegation of filter. Indicated whether age of input does say something...
	bool SamplesReceived() const;

	// Continue lab streaming layer streaming of eye gaze data
	void ContinueLabStream();

	// Pause lab streaming layer streaming of eye gaze data
	void PauseLabStream();

	// Get pointer to interface for custom transformation of samples before filtering
	std::weak_ptr<CustomTransformationInterface> GetCustomTransformationInterface();

	// Get eyetracker info
	EyetrackerInfo GetEyetrackerInfo() const { return _info; }

private:

	// Thread that connects to eye tracking device
	std::unique_ptr<std::thread> _upConnectionThread;

	// ###################################
	// ### Variables written by thread ###
	// ###################################
#ifdef _WIN32
	// Handle for plugin
	HINSTANCE _pluginHandle = NULL; // can be not null but still disconnected

	// Handle to fetch gaze samples
	FETCH_SAMPLES _procFetchGazeSamples = NULL;

	// Handle to check tracking
	IS_TRACKING _procIsTracking = NULL;

	// Handle to calibration
	CALIBRATE _procCalibrate = NULL;

	// Handle to get trackbox info
	GET_TRACKBOX_INFO _procGetTrackboxInfo = NULL;

	// Handle to continue lab stream
	CONTINUE_LAB_STREAM _procContinueLabStream = NULL;

	// Handle to pause lab stream
	PAUSE_LAB_STREAM _procPauseLabStream = NULL;
#endif // _WIN32

	// Info about eye tracking device
	EyetrackerInfo _info; // also indicator for successful connection
	
	// ###################################

    // Mouse cursor coordinates
    double _mouseX = 0;
    double _mouseY = 0;

    // Eye tracker overidden by mouse
    bool _mouseOverride = false;

    // To override the eye tracker, mouse must be moved within given timeframe a given distance
    int _mouseOverrideX = 0;
    int _mouseOverrideY = 0;
    float _mouseOverrideTime = EYEINPUT_MOUSE_OVERRIDE_INIT_FRAME_DURATION;
    bool _mouseOverrideInitFrame = false;

	// Filter of gaze data
	std::shared_ptr<Filter> _spFilter;
};

#endif // EYEINPUT_H_