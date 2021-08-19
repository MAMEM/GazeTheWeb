//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================
// Global variables which are common to be changed for different setups. May be
// moved to some ini file or settings, soon.

#ifndef SETUP_H_
#define SETUP_H_

#include "src/Input/Filters/FilterKernel.h"
#include <string>
#include <chrono>

namespace setup
{
	// Make modes available at runtime as booleans
#ifdef CLIENT_DEBUG
	static const bool	DEBUG_MODE = true;
#else
	static const bool	DEBUG_MODE = false;
#endif
#ifdef CLIENT_DEMO
	static const bool	DEMO_MODE = true;
#else
	static const bool	DEMO_MODE = false;
#endif
#ifdef CLIENT_DEPLOYMENT
	static const bool	DEPLOYMENT = true;
#else
	static const bool	DEPLOYMENT = false;
#endif

	// Window
	static const bool	FULLSCREEN = false; // does not work in combination with eye tracker calibration
	static const bool	MAXIMIZE_WINDOW = false | DEPLOYMENT | DEMO_MODE; // only implemented for Windows
	static const bool	REMOVE_WINDOW_DECORATION = DEMO_MODE;
	static const int	INITIAL_WINDOW_WIDTH = 1280;
	static const int	INITIAL_WINDOW_HEIGHT = 720;

	// Control TODO: move connect bools to config file
	static const bool	CONNECT_OPEN_GAZE = false;
	static const bool	CONNECT_SMI_IVIEWX = false;
	static const bool	CONNECT_VI_MYGAZE = true | DEPLOYMENT;
	static const bool	CONNECT_TOBII_EYEX = true;
	static const bool	CONNECT_TOBII_PRO = false;
	static const float	DURATION_BEFORE_INPUT = 1.f; // wait one second before accepting input
	static const float	MAX_AGE_OF_USED_GAZE = 0.25f; // only accept gaze as input that is not older than one second (TODO: this is not used by filter but by master to determine when to stop taking gaze input as serious)
	static const float	DURATION_BEFORE_SUPER_CALIBRATION = 30.f; // duration until recalibration is offered after receiving no gaze samples
	static const bool	PAUSED_AT_STARTUP = false | DEMO_MODE;
	static const bool	SUPER_CALIBRATION_AT_STARTUP = false; // DEPLOYMENT && !DEMO_MODE;
	static const float	LINK_CORRECTION_MAX_PIXEL_DISTANCE = 5.f;
	static const int	TEXT_SELECTION_MARGIN = 4; // area which is selected before / after zoom coordinate in CEFPixels

												   // Gaze filtering
	static const int	FILTER_GAZE_FIXATION_PIXEL_RADIUS = 30;
	static const int	FILTER_MEMORY_SIZE = 1000; // how many samples are kept in memory of the filters
	static const FilterKernel FILTER_KERNEL = FilterKernel::GAUSSIAN;
	static const float	FILTER_WINDOW_TIME = 1.f; // in seconds, limits the fixation duration in the input structure !!!
	static const bool	FILTER_USE_OUTLIER_REMOVAL = true;

	// Distortion
	static const bool	EYEINPUT_DISTORT_GAZE = false && !(DEPLOYMENT || DEMO_MODE);
	static const float	EYEINPUT_DISTORT_GAZE_BIAS_X = 64.f; // pixels
	static const float	EYEINPUT_DISTORT_GAZE_BIAS_Y = 32.f; // pixels

															 // Experiments
	static const bool			ENABLE_EYEGUI_DRIFT_MAP_ACTIVATION = false; // !DEMO_MODE;
	static const std::string	LAB_STREAM_OUTPUT_NAME = "GazeTheWebOutput";
	static const std::string	LAB_STREAM_OUTPUT_SOURCE_ID = CLIENT_VERSION; // use client version as source id
	static const std::string	LAB_STREAM_INPUT_NAME = "GazeTheWebInput"; // may be set to same value as LAB_STREAM_OUTPUT_NAME to receive own events for debugging purposes
	static const bool			LOG_INTERACTIONS = false; // on eyeGUI level, deprecated
	static const bool			TAB_TRIGGER_SHOW_BADGE = false;
	static const std::string	DASHBOARD_URL = ""; // without slash at the end
	static const double			INACTIVITY_SHUTDOWN_TIME = 60.0*60.0*3.0; // shutting down after three hours of inactivity (determined by the time the super calibration layout is visible, in seconds)

	// Firebase
	static const bool			FIREBASE_MAILING = false; // on/off switch for sending data to Firebase
	static const std::string	FIREBASE_API_KEY = ""; // API key for our Firebase
	static const std::string	FIREBASE_PROJECT_ID = ""; // Project Id of our Firebase
	static const int			SOCIAL_RECORD_DIGIT_COUNT = 6;
	static const bool			SOCIAL_RECORD_PERSIST_UNKNOWN = true;
	static const std::string	DATE_FORMAT = "%d-%m-%Y %H-%M-%S";

	// Other
	static const bool	ENABLE_WEBGL = false; // only on Windows
	static const bool	BLUR_PERIPHERY = false;
	static const float	WEB_VIEW_RESOLUTION_SCALE = 1.f;
	static const unsigned int	HISTORY_MAX_PAGE_COUNT = 100; // maximal length of history
	static const bool	USE_DOM_NODE_POLLING = !DEBUG_MODE;
	static const float	DOM_POLLING_FREQUENCY = 1.0f; // times per second
	static const int	DOM_POLLING_PARTITION_NUMBER = 8;
	static const std::chrono::milliseconds STORING_TIME = std::chrono::milliseconds(2500); // time to store the queue of GazeCoordinates to use past values
	static const bool	PERIODICAL_VOICE_RESTART = false; // allow the voice recognition to restart before 60 seconds are expired (after 50 seconds)
	static const bool	KEYSTROKE_BMP_CREATION = false; // Creation of bmp files using the "s" key
	static const int	BMP_GAZE_RADIUS = 200; // Radius around the gaze in which you want to take a partial screenshot
}

#endif // SETUP_H_
