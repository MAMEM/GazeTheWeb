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
#include <memory>
#include <vector>

// Necessary for dynamic DLL loading in Windows
#ifdef _WIN32
#include <windows.h>
typedef void(__cdecl *FETCH_GAZE)(int, std::vector<double>&, std::vector<double>&);
#endif

class EyeInput
{
public:

    // Constructor
    EyeInput(bool useEmulation = false);

    // Destructor
    virtual ~EyeInput();

    // Update. Returns whether gaze is currently used (or mouse when false)
	// Taking information about window to enable eyetracking in windowed mode
	bool Update(
		float tpf,
		double mouseX,
		double mouseY,
		double& rGazeX,
		double& rGazeY,
		int windowX,
		int windowY,
		int windowWidth,
		int windowHeight);

private:

#ifdef _WIN32
	// Handle for plugin
	HINSTANCE _pluginHandle = NULL;

	// Handle to fetch gaze
	FETCH_GAZE _procFetchGaze = NULL;
#endif

	// Remember whether connection has been established
	bool _connected = false;

    // Mouse cursor coordinates
    double _mouseX = 0;
    double _mouseY = 0;

    // Eyetracker overidden by mouse
    bool _mouseOverride = false;

    // To override the eyetracker, mouse must be moved within given timeframe a given distance
    int _mouseOverrideX = 0;
    int _mouseOverrideY = 0;
    float _mouseOverrideTime = EYEINPUT_MOUSE_OVERRIDE_INIT_FRAME_DURATION;
    bool _mouseOverrideInitFrame = false;
};

#endif // EYEINPUT_H_
