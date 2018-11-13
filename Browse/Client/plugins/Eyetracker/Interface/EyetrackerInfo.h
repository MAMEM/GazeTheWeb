//============================================================================
// Distributed under the Apache License, Version 2.0.
// Author: Raphael Menges (raphaelmenges@uni-koblenz.de)
//============================================================================

#ifndef EYETRACKERINFO_H_
#define EYETRACKERINFO_H_

#include <vector>

// Struct of info
struct EyetrackerInfo
{
	// Fields
	bool connected = false;
	int samplerate = -1;
	bool geometrySetupSuccessful = false;
};

// Struct about position of eyes in track box
struct TrackboxInfo
{
	// Whether tracked or not
	bool leftTracked = false;
	bool rightTracked = false;

	// Relative eye position in trackbox -1 to 1
	float leftX = 0; // left to right
	float leftY = 0; // lower to upper
	float leftZ = 0; // near to far
	float rightX = 0; // left to right
	float rightY = 0; // lower to upper
	float rightZ = 0; // near to far
};

// Enumeration about calibration
enum CalibrationResult { CALIBRATION_NOT_SUPPORTED, CALIBRATION_OK, CALIBRATION_BAD, CALIBRATION_FAILED };

// Enumeration about calibration point
enum CalibrationPointResult { CALIBRATION_POINT_OK, CALIBRATION_POINT_BAD, CALIBRATION_POINT_FAILED };

// Struct of calibration point
struct CalibrationPoint
{
	CalibrationPoint(int positionX, int positionY, CalibrationPointResult result) : positionX(positionX), positionY(positionY), result(result) {};
	int positionX = 0;
	int positionY = 0;
	CalibrationPointResult result = CALIBRATION_POINT_FAILED;
};

// Vector with calibration info
typedef std::vector<CalibrationPoint> CalibrationInfo;

#endif EYETRACKERINFO_H_