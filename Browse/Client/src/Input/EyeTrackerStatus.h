//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef EYETRACKERSTATUS_H_
#define EYETRACKERSTATUS_H_

// Enumeration about eye tracker status
enum class EyeTrackerStatus
{
	CONNECTED, DISCONNECTED, TRYING_TO_CONNECT
};

// Enumeration about available eye trackers
enum class EyeTrackerDevice
{
	OPEN_GAZE, SMI_REDN, VI_MYGAZE, TOBII_EYEX, TOBII_PRO, NONE
};

#endif EYETRACKERSTATUS_H_