//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef EYETRACKERGEOMETRY_H_
#define EYETRACKERGEOMETRY_H_

// Struct about geometry of screen and eyetracker
struct EyetrackerGeometry
{
	int monitorWidth			= -1; // millimeter
	int monitorHeight			= -1; // millimeter
	int mountingAngle			= -1; // degree
	int relativeDistanceHeight	= -1; // millimeter distance between eyetracker and monitor in height
	int relativeDistanceDepth	= -1; // millimeter distance between eyetracker and monitor in depth
};

#endif EYETRACKERGEOMETRY_H_