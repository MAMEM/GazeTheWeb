//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#include "EyeInput.h"
#include "src/Utils/Logger.h"
#include "src/Setup.h"
#include "src/Input/Filters/WeightedAverageFilter.h"
#include <cmath>
#include <functional>

EyeInput::EyeInput(MasterThreadsafeInterface* _pMasterThreadsafeInterface, EyetrackerGeometry geometry) :
	_spFilter(std::shared_ptr<Filter>(
		new WeightedAverageFilter(
			setup::FILTER_KERNEL,
			setup::FILTER_WINDOW_TIME,
			setup::FILTER_USE_OUTLIER_REMOVAL)))
{
	// Create thread for connection to eye tracker
	_upConnectionThread = std::unique_ptr<std::thread>(new std::thread([this, _pMasterThreadsafeInterface, geometry]()
	{
#ifdef _WIN32

		// Define procedure signature for connection
		typedef EyetrackerInfo(__cdecl *CONNECT)(EyetrackerGeometry);

		// Variable about device
		EyeTrackerDevice device = EyeTrackerDevice::NONE;

		// Trying to connect
		_pMasterThreadsafeInterface->threadsafe_NotifyEyeTrackerStatus(EyeTrackerStatus::TRYING_TO_CONNECT, EyeTrackerDevice::NONE); // just give general indication about trying to connect

		// Function to connect to eye tracker via plugin
		std::function<void(std::string)> ConnectEyeTracker = [&](std::string plugin)
		{
			std::string dllName = plugin + ".dll";
			_pluginHandle = LoadLibraryA(dllName.c_str());

			// Try to connect to eye tracker
			if (_pluginHandle != NULL)
			{
				LogInfo("EyeInput: Loaded " + plugin + ".");

				// Fetch procedure for connecting
				CONNECT procConnect = (CONNECT)GetProcAddress(_pluginHandle, "Connect");

				// Fetch procedure for fetching gaze data
				_procFetchGazeSamples = (FETCH_SAMPLES)GetProcAddress(_pluginHandle, "FetchSamples");

				// Fetch procedure to check tracking
				_procIsTracking = (IS_TRACKING)GetProcAddress(_pluginHandle, "IsTracking");

				// Fetch procedure to calibrate
				_procCalibrate = (CALIBRATE)GetProcAddress(_pluginHandle, "Calibrate");

				// Fetch procedure to get trackbox info
				_procGetTrackboxInfo = (GET_TRACKBOX_INFO)GetProcAddress(_pluginHandle, "GetTrackboxInfo");

				// Fetch procedure to continue lab stream
				_procContinueLabStream = (CONTINUE_LAB_STREAM)GetProcAddress(_pluginHandle, "ContinueLabStream");

				// Fetch procedure to pause lab stream
				_procPauseLabStream = (PAUSE_LAB_STREAM)GetProcAddress(_pluginHandle, "PauseLabStream");

				// Check whether procedures could be loaded
				if (procConnect != NULL
					&& _procFetchGazeSamples != NULL
					&& _procIsTracking != NULL
					&& _procCalibrate != NULL
					&& _procGetTrackboxInfo != NULL
					&& _procContinueLabStream != NULL
					&& _procPauseLabStream != NULL)
				{
					/*
					EyetrackerGeometry geometry;
					geometry.monitorWidth = 276;
					geometry.monitorHeight = 155;
					geometry.mountingAngle = 20;
					geometry.relativeDistanceDepth = 18;
					geometry.relativeDistanceHeight = -5;
					*/
					EyetrackerInfo info = procConnect(geometry);
					if (info.connected)
					{
						LogInfo("EyeInput: Connecting eye tracker successful.");
						LogDebug("EyeInput: Eyetracker geometry setup feedback is ", info.geometrySetupSuccessful);

						// Set member about eyetracker info
						_info = info;

						return; // direct return from thread
					}
					else
					{
						// Reset handles when connection to eye tracker failed
						_procFetchGazeSamples = NULL;
						_procIsTracking = NULL;
						_procCalibrate = NULL;
						_procGetTrackboxInfo = NULL;
						_procContinueLabStream = NULL;
						_procPauseLabStream = NULL;
					}
				}
			}
			else
			{
				LogInfo("EyeInput: Failed to load " + plugin + ".");
			}
		};

		// Try to load OpenGaze API plugin
		if (!_info.connected && setup::CONNECT_OPEN_GAZE)
		{
			device = EyeTrackerDevice::OPEN_GAZE;
			ConnectEyeTracker("OpenGazePlugin");
		}

		// Try to load SMI iViewX plugin
		if (!_info.connected && setup::CONNECT_SMI_IVIEWX)
		{
			device = EyeTrackerDevice::SMI_REDN;
			ConnectEyeTracker("SMIiViewXPlugin");
		}

		// Try to load Visual Interaction myGaze plugin
		if (!_info.connected && setup::CONNECT_VI_MYGAZE)
		{
			device = EyeTrackerDevice::VI_MYGAZE;
			ConnectEyeTracker("VImyGazePlugin");
		}

		// Try to load Tobii EyeX plugin
		if (!_info.connected && setup::CONNECT_TOBII_EYEX)
		{
			device = EyeTrackerDevice::TOBII_EYEX;
			ConnectEyeTracker("TobiiEyeXPlugin");
		}

		// Try to load Tobii Pro plugin
		if (!_info.connected && setup::CONNECT_TOBII_PRO)
		{
			device = EyeTrackerDevice::TOBII_PRO;
			ConnectEyeTracker("TobiiProPlugin");
		}

		// If not connected to any eye tracker and provide feedback
		if (!_info.connected)
		{
			LogInfo("EyeInput: No eye tracker connected. Input emulated by mouse.");
			_pMasterThreadsafeInterface->threadsafe_NotifyEyeTrackerStatus(EyeTrackerStatus::DISCONNECTED, EyeTrackerDevice::NONE);
		}
		else // if connected, setup current policy about data transfer
		{
			_pMasterThreadsafeInterface->threadsafe_NotifyEyeTrackerStatus(EyeTrackerStatus::CONNECTED, device);
			if (_pMasterThreadsafeInterface->threadsafe_MayTransferData())
			{
				_procContinueLabStream(); // should be already started, but for the sake of completeness
			}
			else
			{
				_procPauseLabStream(); // pause streaming immediatelly
			}
		}
#endif // _WIN32
	}));
}

EyeInput::~EyeInput()
{
	// First, wait for eye tracker connection thread to join
	LogInfo("EyeInput: Make sure that eye tracker connection thread is joined.");
	_upConnectionThread->join();

#ifdef _WIN32

	// Check whether necessary to disconnect
	if (_pluginHandle != NULL)
	{
		// Disconnect eye tracker if necessary
		if (_info.connected)
		{
			typedef bool(__cdecl *DISCONNECT)();
			DISCONNECT procDisconnect = (DISCONNECT)GetProcAddress(_pluginHandle, "Disconnect");

			// Disconnect eye tracker when procedure available
			if (procDisconnect != NULL)
			{
				LogInfo("EyeInput: About to disconnect eye tracker.");
				bool result = procDisconnect();

				// Check whether disconnection has been successful
				if (result)
				{
					LogInfo("EyeInput: Disconnecting eye tracker successful.");
				}
				else
				{
					LogInfo("EyeInput: Disconnecting eye tracker failed.");
				}

				// Reset eyetracker info
				_info = EyetrackerInfo();
			}
		}

		// Unload plugin
		FreeLibrary(_pluginHandle);
	}

#endif // _WIN32

}

std::shared_ptr<Input> EyeInput::Update(
	bool windowFocused,
	float tpf,
	double mouseX,
	double mouseY,
	int windowX,
	int windowY,
	int windowWidth,
	int windowHeight,
	int monitorWidth,
	int monitorHeight)
{
	// ### UPDATE GAZE INPUT ###

	// Bool whether eye tracker is tracking
	bool isTracking = false;

#ifdef _WIN32

	if (_info.connected && _procFetchGazeSamples != NULL && _procIsTracking != NULL)
	{
		
		// Prepare queue to fill
		SampleQueue spSamples = SampleQueue(new std::deque<SampleData>);

		// Fetch samples
		_procFetchGazeSamples(spSamples); // shared pointered vector is filled by fetch procedure

		// Expecting in screen pixel space
		for (auto& sample : *spSamples)
		{
			switch (sample.system)
			{
			case SampleDataCoordinateSystem::SCREEN_PIXELS:
				// everything ok
				break;
			case SampleDataCoordinateSystem::SCREEN_RELATIVE:
				// bring samples into pixel space of monitor (TODO: right now, this does only work for primary monitor)
				sample.x = sample.x * monitorWidth;
				sample.y = sample.y * monitorHeight;
				sample.system = SampleDataCoordinateSystem::SCREEN_PIXELS;
				LogInfo("Gaze: ", sample.x, ", ", sample.y);
				break;
			}
		}

		// Convert parameters to double (use same values for all samples,
		double windowXDouble = (double)windowX;
		double windowYDouble = (double)windowY;
		double windowWidthDouble = (double)windowWidth;
		double windowHeightDouble = (double)windowHeight;

		// Go over available samples and bring into window space
		for (auto& sample : *spSamples)
		{
			// Do some clamping according to window coordinates for gaze x
			sample.x = sample.x - windowXDouble;
			sample.x = sample.x > 0.0 ? sample.x : 0.0;
			sample.x = sample.x < windowWidthDouble ? sample.x : windowWidthDouble;

			// Do some clamping according to window coordinates for gaze y
			sample.y = sample.y - windowYDouble;
			sample.y = sample.y > 0.0 ? sample.y : 0.0;
			sample.y = sample.y < windowHeightDouble ? sample.y : windowHeightDouble;
		}

		// Update filter algorithm and provide local variables as reference
		_spFilter->Update(spSamples, _info.samplerate);

		// Check, whether eye tracker is tracking
		isTracking = _procIsTracking();
	}

#endif // _WIN32

	// ### MOUSE INPUT ###

	// Mouse override of eye tracker
	if (_mouseOverride)
	{
		// Check whether override should stop
		if (mouseX == _mouseX && mouseY == _mouseY)
		{
			// Check whether override is over
			_mouseOverrideTime -= tpf;
			if (_mouseOverrideTime <= 0)
			{
				// Deactivate override
				_mouseOverride = false;
			}
		}
		else
		{
			// Since mouse cursor was moved, reset override (stop) time
			_mouseOverrideTime = EYEINPUT_MOUSE_OVERRIDE_STOP_DURATION;
		}
	}
	else
	{
		// Check whether override should start
		if (_mouseOverrideInitFrame)
		{
			// Already inside frame of initialization
			_mouseOverrideTime -= tpf;
			if (_mouseOverrideTime <= 0)
			{
				// Check whether mouse cursor movement was enough to trigger override
				float x = (float)(mouseX - _mouseOverrideX);
				float y = (float)(mouseY - _mouseOverrideY);
				float distance = std::sqrt(x*x + y*y);
				if (distance >= EYEINPUT_MOUSE_OVERRIDE_INIT_DISTANCE)
				{
					// Activate override
					_mouseOverride = true;
				}

				// Cleanup
				_mouseOverrideInitFrame = false;

				// Time is used while override to determine when to stop it
				_mouseOverrideTime = EYEINPUT_MOUSE_OVERRIDE_STOP_DURATION;
			}
		}
		else
		{
			// Check whether there was initial movement
			if (mouseX != _mouseX || mouseY != _mouseY)
			{
				// Start override initialization frame
				_mouseOverrideInitFrame = true;
				_mouseOverrideX = mouseX;
				_mouseOverrideY = mouseY;
				_mouseOverrideTime = EYEINPUT_MOUSE_OVERRIDE_INIT_FRAME_DURATION;
			}
		}
	}

	// ### GAZE EMULATION ###

	// Bool to indicate mouse usage for gaze coordinates
	bool gazeEmulated =
		!_info.connected // eye tracker not connected
		|| _mouseOverride // eye tracker overriden by mouse
		|| !isTracking; // eye tracker not available

	// Save mouse cursor coordinate in members for calculating mouse override
	_mouseX = mouseX;
	_mouseY = mouseY;

	// Get data from filter
	double filteredGazeX = _spFilter->GetFilteredGazeX();
	double filteredGazeY = _spFilter->GetFilteredGazeY();

	// Get raw data
	double rawGazeX = _spFilter->GetRawGazeX();
	double rawGazeY = _spFilter->GetRawGazeY();

	// Use mouse when gaze is emulated
	if (gazeEmulated)
	{
		filteredGazeX = mouseX;
		filteredGazeY = mouseY;
		rawGazeX = mouseX;
		rawGazeY = mouseY;
	}

	// ### GAZE DISTORTION ###

	// Add optional noise and bias for testing purposes
	if (setup::EYEINPUT_DISTORT_GAZE)
	{
		filteredGazeX += setup::EYEINPUT_DISTORT_GAZE_BIAS_X;
		filteredGazeY += setup::EYEINPUT_DISTORT_GAZE_BIAS_Y;
		rawGazeX += setup::EYEINPUT_DISTORT_GAZE_BIAS_X;
		rawGazeY += setup::EYEINPUT_DISTORT_GAZE_BIAS_Y;
	}

	// ### OUTPUT ###

	// Create input structure to return
	std::shared_ptr<Input> spInput = std::make_shared<Input>(
		windowFocused,
		filteredGazeX, // gazeX,
		filteredGazeY, // gazeY,
		rawGazeX, // rawGazeX
		rawGazeY, // rawGazeY
		_spFilter->GetAge(), // gazeAge
		gazeEmulated, // gazeEmulated,
		false, // gazeUponGUI,
		false, // instantInteraction,
		_spFilter->GetFixationDuration()); // fixationDuration

	// Return whether gaze coordinates comes from eye tracker
	return spInput;
}

CalibrationResult EyeInput::Calibrate(std::shared_ptr<CalibrationInfo>& rspCalibrationInfo)
{
	CalibrationResult result = CALIBRATION_NOT_SUPPORTED;
#ifdef _WIN32
	if (_info.connected && _procCalibrate != NULL)
	{
		result = _procCalibrate(rspCalibrationInfo);
	}
#endif // _WIN32
	return result;
}

TrackboxInfo EyeInput::GetTrackboxInfo()
{
	TrackboxInfo info;
#ifdef _WIN32
	if (_info.connected && _procGetTrackboxInfo != NULL)
	{
		info = _procGetTrackboxInfo();
	}
#endif // _WIN32
	return info;
}


bool EyeInput::SamplesReceived() const
{
	return _spFilter->IsTimestampSetOnce();
}

void EyeInput::ContinueLabStream()
{
#ifdef _WIN32
	if (_info.connected && _procContinueLabStream != NULL)
	{
		_procContinueLabStream();
	}
#endif // _WIN32
}

void EyeInput::PauseLabStream()
{
#ifdef _WIN32
	if (_info.connected && _procPauseLabStream != NULL)
	{
		_procPauseLabStream();
	}
#endif // _WIN32
}

std::weak_ptr<CustomTransformationInterface> EyeInput::GetCustomTransformationInterface()
{
	return _spFilter;
}
