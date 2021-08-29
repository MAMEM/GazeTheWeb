//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

// This is an implementation
#define DLL_IMPLEMENTATION

#include "plugins/Eyetracker/Interface/Eyetracker.h"
#include "plugins/Eyetracker/Common/EyetrackerData.h"
#include "eyex/EyeX.h"

// Global variables
TX_CONTEXTHANDLE Context = TX_EMPTY_HANDLE;
const TX_STRING InteractorId = "GazeTheWeb-Browse"; // id or interactor which covers complete screen and we just listen to gaze point behavior
TX_HANDLE GlobalInteractorSnapshot = TX_EMPTY_HANDLE; // snapshot containing interactor
TX_TICKET ConnectionStateChangedTicket = TX_INVALID_TICKET;
TX_TICKET TrackingStateChangedTicket = TX_INVALID_TICKET;
TX_TICKET EventHandlerTicket = TX_INVALID_TICKET;
bool Tracking = false;

// Callback function invoked when state changes
void TX_CALLCONVENTION OnEngineStateChanged(TX_CONSTHANDLE hState, TX_USERPARAM userParam)
{
	// Preparation
	TX_HANDLE hStateBag = TX_EMPTY_HANDLE;
	TX_INTEGER eyeTrackingState;
	bool success;
	
	// Fetch and process data
	txGetAsyncDataContent(hState, &hStateBag);
	success = (txGetStateValueAsInteger(hStateBag, TX_STATEPATH_EYETRACKINGSTATE, &eyeTrackingState) == TX_RESULT_OK);
	if (success)
	{
		if (eyeTrackingState == TX_EYETRACKINGDEVICESTATUS_NOTAVAILABLE
			|| eyeTrackingState == TX_EYETRACKINGDEVICESTATUS_INVALIDCONFIGURATION
			|| eyeTrackingState == TX_EYETRACKINGDEVICESTATUS_DEVICENOTCONNECTED
			|| eyeTrackingState == TX_EYETRACKINGDEVICESTATUS_TRACKINGPAUSED
			|| eyeTrackingState == TX_EYETRACKINGDEVICESTATUS_UNKNOWNERROR
			|| eyeTrackingState == TX_EYETRACKINGDEVICESTATUS_CONNECTIONERROR)
		{
			Tracking = false;
		}
		else
		{
			Tracking = true;
		}
	}
	txReleaseObject(&hStateBag);
}

// Callback function invoked when a gaze data behavior is received
void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK)
	{
		// Push back to vector
		using namespace std::chrono;
		eyetracker_global::PushBackSample(
			SampleData(
				eventParams.X, // x
				eventParams.Y, // y
				SampleDataCoordinateSystem::SCREEN_PIXELS,
				duration_cast<milliseconds>(
					system_clock::now().time_since_epoch() // timestamp
					),
				true // EyeX only delivers valid samples
			)
		);

		// When gaze samples are received, tracking seems to work...
		Tracking = true; // kinda hack: initially, no onEngineStateChanged is triggered. So initial state is unkown and even txGetState does not work. This workaround sets Tracking at first gaze sample to true
	}
}

// Callback function invoked when an event has been received from the EyeX Engine
void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	// Preparation
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

	// Fetch and process data
	txGetAsyncDataContent(hAsyncData, &hEvent);
	if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK)
	{
		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}
	txReleaseObject(&hEvent);
}

// Callback function invoked when snapshot has been commited
void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
}

// Callback function invoked when engine connection state changes
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState)
	{
	case TX_CONNECTIONSTATE_CONNECTED:
		{
			txCommitSnapshotAsync(GlobalInteractorSnapshot, OnSnapshotCommitted, NULL);
		}
		break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		// TODO
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		// TODO
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		// TODO
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		// TODO
		break;
	}
}

// Intialization of global interactor snapshot with unfiltered gaze
bool InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_UNFILTERED };
	bool success;
	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&GlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateGazePointDataBehavior(hInteractor, &params) == TX_RESULT_OK; // behavior for gaze point
	txReleaseObject(&hInteractor);
	return success;
}

EyetrackerInfo Connect(EyetrackerGeometry geometry)
{
	EyetrackerInfo info;
	info.type = ET_TOBII_EYEX;

	// Check for EyeX engine
	TX_EYEXAVAILABILITY availability;
	if (txGetEyeXAvailability(&availability) == TX_RESULT_OK)
	{
		if (availability == TX_EYEXAVAILABILITY_NOTAVAILABLE)
		{
			return info;
		}
		else if (availability == TX_EYEXAVAILABILITY_NOTRUNNING)
		{
			return info;
		}
	}

	bool success = false;

	// Initialize and enable the context that is the link to the EyeX Engine
	success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK; // initialize library
	success &= txCreateContext(&Context, TX_FALSE) == TX_RESULT_OK; // context used over complete lifetime
	success &= InitializeGlobalInteractorSnapshot(Context); // initialize snapshot and handle it over at connection
	success &= txRegisterConnectionStateChangedHandler(Context, &ConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK; // register handler for connection state change
	success &= txRegisterStateChangedHandler(Context, &TrackingStateChangedTicket, TX_STATEPATH_EYETRACKINGSTATE, OnEngineStateChanged, NULL) == TX_RESULT_OK; // register handler for eyetracking state change
	success &= txRegisterEventHandler(Context, &EventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK; // register handler for events
	success &= txEnableConnection(Context) == TX_RESULT_OK; // do connect to EyeX engine

	// Info about eye tracker
	if (success)
	{
		info.connected = true;
		info.samplerate = 90; // 60 for EyeX and 90 for 4C
	}

	// Return success
	return info;
}

bool IsTracking()
{
	return Tracking;
}

bool Disconnect()
{
	bool success;
	txUnregisterConnectionStateChangedHandler(Context, ConnectionStateChangedTicket); // unregister connection state handler
	txUnregisterStateChangedHandler(Context, TrackingStateChangedTicket); // unregister eyetracking state change handler
	txUnregisterEventHandler(Context, EventHandlerTicket); // unregister event handler (which receives behavior)
	txDisableConnection(Context); // disable connection to EyeX engine
	txReleaseObject(&GlobalInteractorSnapshot); // release snapshot containing interactor
	success = txShutdownContext(Context, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK; // shutdown context
	success &= txReleaseContext(&Context) == TX_RESULT_OK; // release context object
	success &= txUninitializeEyeX() == TX_RESULT_OK; // uninitialize library
	return success;
}

void FetchSamples(SampleQueue& rspSamples)
{
	eyetracker_global::FetchSamples(rspSamples);
}

CalibrationResult Calibrate(std::shared_ptr<CalibrationInfo>& rspInfo)
{
	// Not supported
	return CALIBRATION_NOT_SUPPORTED;
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